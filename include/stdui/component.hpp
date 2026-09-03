#pragma once

#include <stdui/expressions.hpp>
#include <stdui/inspection.hpp>
#include <stdui/state.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace stdui {

/**
 * Component identity for state tracking.
 *
 * Components are identified by their type and a unique ID within that type.
 * The runtime uses this to associate state_storage instances with components.
 */
struct component_id {
  std::type_index type;
  std::size_t instance_id;

  bool operator==(component_id const &) const = default;
};

} // namespace std

namespace std {
template <> struct hash<stdui::component_id> {
  auto operator()(stdui::component_id const &id) const noexcept -> std::size_t {
    return std::hash<std::type_index>{}(id.type) ^ (std::hash<std::size_t>{}(id.instance_id) << 1);
  }
};
} // namespace std

namespace stdui {

/**
 * Base class for all components.
 *
 * Components are reusable UI building blocks with their own state and body.
 * Subclasses override body() to return the component's view tree as an
 * inspection_node (the runtime representation of expression trees).
 */
class component_base {
public:
  virtual ~component_base() = default;

  /// Returns the component's view tree. Called during evaluation.
  virtual auto body(component_context &ctx) const -> inspection_node = 0;

  /// Returns the component's type index for identity tracking.
  virtual auto type_id() const -> std::type_index = 0;

  /// Returns a unique instance ID for this component (default: based on address).
  virtual auto instance_id() const -> std::size_t { return reinterpret_cast<std::size_t>(this); }
};

/**
 * Typed component base with CRTP for automatic type_id.
 */
template <typename Derived> class typed_component : public component_base {
public:
  auto type_id() const -> std::type_index override { return typeid(Derived); }
};

/**
 * Component registry for tracking instances and their state.
 *
 * The runtime maintains one registry per application. Components are
 * registered during evaluation, and their state persists across frames.
 */
class component_registry {
public:
  /// Returns the state storage for a component, creating it if necessary.
  auto get_or_create_storage(component_id const &id) -> state_storage & {
    auto it = storage_.find(id);
    if (it == storage_.end()) {
      it = storage_.emplace(id, state_storage{}).first;
    }
    return it->second;
  }

  /// Removes state storage for a component (called when component is destroyed).
  void remove_storage(component_id const &id) { storage_.erase(id); }

  /// Clears all component state.
  void clear() { storage_.clear(); }

  /// Returns the number of registered components.
  auto component_count() const -> std::size_t { return storage_.size(); }

private:
  std::unordered_map<component_id, state_storage> storage_;
};

/**
 * Component evaluator.
 *
 * Evaluates a component tree, managing state and component lifecycles.
 */
class component_evaluator {
public:
  explicit component_evaluator(component_registry &registry) : registry_(registry) {}

  /// Evaluates a component, returning its body as an inspection_node.
  auto evaluate(component_base const &comp, std::function<void()> on_change) -> inspection_node {
    component_id id{comp.type_id(), comp.instance_id()};

    auto &storage = registry_.get_or_create_storage(id);
    component_context ctx{&storage, std::move(on_change)};

    return comp.body(ctx);
  }

private:
  component_registry &registry_;
};

/**
 * Helper: Inspects any view expression and returns inspection_node.
 * Components can use this in their body() to convert DSL expressions.
 */
template <view_expression T> auto inspect_expr(T const &expr) -> inspection_node {
  return inspect(expr);
}

} // namespace stdui
