#pragma once

#include <stdui/expressions.hpp>
#include <stdui/inspection.hpp>

#include <any>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace stdui {

class runtime;

/// Internal key for one persistent state slot.
struct state_key {
  std::string path;
  std::type_index kind;
  std::type_index state_type;
  std::string name;

  bool operator==(state_key const &) const = default;
};

/// Hash support for `state_key`.
struct state_key_hash {
  auto operator()(state_key const &key) const noexcept -> std::size_t {
    auto seed = std::hash<std::string>{}(key.path);
    auto kind_hash = std::hash<std::type_index>{}(key.kind);
    seed ^= kind_hash + 0x9e3779b9U + (seed << 6U) + (seed >> 2U);
    seed ^=
        std::hash<std::type_index>{}(key.state_type) + 0x9e3779b9U + (seed << 6U) + (seed >> 2U);
    seed ^= std::hash<std::string>{}(key.name) + 0x9e3779b9U + (seed << 6U) + (seed >> 2U);
    return seed;
  }
};

using state_store = std::unordered_map<state_key, std::any, state_key_hash>;

/// Explicit state staging area for one evaluation.
struct evaluation_context {
  state_store &staged;
};

/**
 * Non-owning access to persistent component state.
 *
 * The handle is valid only while the owning component body is being evaluated.
 */
template <class T> class state_handle {
public:
  explicit state_handle(T *value) : value_(value) {}

  T &get() const { return *value_; }
  T &operator*() const { return get(); }
  T *operator->() const { return value_; }

private:
  T *value_;
};

/// Runtime services available while evaluating one component body.
class component_context {
public:
  component_context(runtime &owner, std::string component_path, std::type_index component_kind,
                    evaluation_context &context)
      : owner_(owner), component_path_(std::move(component_path)), component_kind_(component_kind),
        context_(context) {}

  /**
   * Obtains or reuses a named component-local state object.
   *
   * The name must be unique within one component evaluation. `T` must be
   * default-constructible when no initial value is supplied.
   */
  template <class T> state_handle<T> state(std::string name, T initial_value = T{});

private:
  runtime &owner_;
  std::string component_path_;
  std::type_index component_kind_;
  evaluation_context &context_;
  std::unordered_set<std::string> state_names_;
};

/// Owns persistent component state and reconciles expression updates.
class runtime {
public:
  /**
   * Evaluates a root expression and atomically commits its resulting state.
   *
   * On successful evaluation, the new state table replaces the previous one.
   * If evaluation throws, the previous committed state remains unchanged.
   */
  template <view_expression Expression> inspection_node reconcile(Expression const &expression) {
    state_store staged;
    evaluation_context context{staged};
    constexpr char root_path[] = "root";

    auto tree = evaluate(expression, root_path, context);
    committed_state_slots_ = std::move(staged);
    return tree;
  }

  /// Returns the number of committed state slots, primarily for tests.
  [[nodiscard]] auto state_count() const -> std::size_t { return committed_state_slots_.size(); }

private:
  friend class component_context;

  template <class T>
  auto state_at(evaluation_context &context, std::string const &component_path,
                std::type_index component_kind, std::string const &name, T initial) {
    state_key key{component_path, component_kind, typeid(T), name};
    auto &staged = context.staged;

    auto it = staged.find(key);
    if (it == staged.end()) {
      auto committed = committed_state_slots_.find(key);
      if (committed != committed_state_slots_.end()) {
        staged.emplace(key, committed->second);
      } else {
        staged.emplace(key, std::move(initial));
      }
      it = staged.find(key);
    }

    if (it->second.type() != typeid(T)) {
      throw std::logic_error("state slot type changed between evaluations");
    }

    return state_handle<T>(&std::any_cast<T &>(it->second));
  }

  auto evaluate(text_expression const &expression, std::string, evaluation_context &) {
    return inspection_node{"text", expression.value, {}};
  }

  template <class... T>
  auto evaluate(vstack_expression<T...> const &expression, std::string path,
                evaluation_context &context) {
    return evaluate_stack("vstack", expression.children, std::move(path), context);
  }

  template <class... T>
  auto evaluate(hstack_expression<T...> const &expression, std::string path,
                evaluation_context &context) {
    return evaluate_stack("hstack", expression.children, std::move(path), context);
  }

  template <class Id, class Expression>
  auto evaluate(identified_expression<Id, Expression> const &expression, std::string path,
                evaluation_context &context) {
    return evaluate(expression.expression, path + "/" + explicit_id_segment(expression.id),
                    context);
  }

  template <class Range, class IdFn, class BodyFn>
  auto evaluate(dynamic_list_expression<Range, IdFn, BodyFn> const &expression, std::string path,
                evaluation_context &context) {
    inspection_node node{"dynamic_list", {}, {}};
    std::unordered_set<std::string> child_paths;

    for (auto &&item : expression.items) {
      auto raw_id = std::invoke(expression.id_fn, item);
      using storage_id_t = typename id_storage<std::decay_t<decltype(raw_id)>>::type;
      storage_id_t id{raw_id};
      auto child_path = path + "/" + explicit_id_segment(id);

      if (!child_paths.insert(child_path).second) {
        throw std::logic_error("duplicate item id");
      }

      auto child = std::invoke(expression.body_fn, item);
      node.children.push_back(evaluate(child, std::move(child_path), context));
    }

    return node;
  }

  template <class Kind, class Body>
  auto evaluate(component_expression<Kind, Body> const &expression, std::string path,
                evaluation_context &context) {
    std::type_index kind{typeid(Kind)};
    component_context component_ctx{*this, path, kind, context};
    return evaluate(std::invoke(expression.body, component_ctx), path + "/body", context);
  }

  template <class Tuple>
  auto evaluate_stack(char const *kind, Tuple const &children, std::string path,
                      evaluation_context &context) {
    inspection_node node{kind, {}, {}};
    append_children(node, children, path, context,
                    std::make_index_sequence<std::tuple_size_v<Tuple>>{});
    return node;
  }

  template <class Tuple, std::size_t... I>
  void append_children(inspection_node &node, Tuple const &children, std::string const &path,
                       evaluation_context &context, std::index_sequence<I...>) {
    std::vector<std::string> child_paths;
    std::unordered_set<std::string> sibling_paths;
    child_paths.reserve(sizeof...(I));
    (remember_child_path(child_paths, sibling_paths, std::get<I>(children), path, I), ...);
    (node.children.push_back(
         evaluate_child(std::get<I>(children), std::move(child_paths[I]), context)),
     ...);
  }

  template <view_expression Child>
  void remember_child_path(std::vector<std::string> &child_paths,
                           std::unordered_set<std::string> &sibling_paths, Child const &child,
                           std::string const &parent_path, std::size_t index) {
    auto path = child_path(parent_path, index, child);
    if (!sibling_paths.insert(path).second) {
      throw std::logic_error("duplicate sibling identity");
    }
    child_paths.push_back(std::move(path));
  }

  template <view_expression Child>
  auto evaluate_child(Child const &child, std::string path, evaluation_context &context) {
    return evaluate(child, std::move(path), context);
  }

  /// Evaluates an identified child through its explicit sibling path.
  template <class Id, class Expression>
  auto evaluate_child(identified_expression<Id, Expression> const &child, std::string path,
                      evaluation_context &context) {
    return evaluate(child.expression, std::move(path), context);
  }

  /// Builds the positional path for an ordinary child expression.
  template <view_expression Child>
  static auto child_path(std::string const &parent_path, std::size_t index, Child const &)
      -> std::string {
    return parent_path + "/" + std::to_string(index);
  }

  /// Builds the id-based path for an identified child expression.
  template <class Id, class Expression>
  static auto child_path(std::string const &parent_path, std::size_t,
                         identified_expression<Id, Expression> const &child) -> std::string {
    return parent_path + "/" + explicit_id_segment(child.id);
  }

  template <class Id> static auto explicit_id_segment(Id const &id) -> std::string {
    if constexpr (std::is_same_v<std::decay_t<Id>, std::string>) {
      return "id:" + std::to_string(id.size()) + ":" + id;
    } else if constexpr (std::integral<std::decay_t<Id>>) {
      return "id:" + std::to_string(id);
    } else {
      static_assert(std::is_same_v<Id, void>, "explicit ids must be strings or integers");
    }
  }

  state_store committed_state_slots_;
};

template <class T> state_handle<T> component_context::state(std::string name, T initial_value) {
  if (!state_names_.insert(name).second) {
    throw std::logic_error("duplicate component state name");
  }
  return owner_.state_at(context_, component_path_, component_kind_, name,
                         std::move(initial_value));
}

} // namespace stdui
