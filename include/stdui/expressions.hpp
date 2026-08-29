#pragma once

#include <concepts>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace stdui {

class component_context;

/// Matches value types with an opt-in expression tag.
/**
 * Ordinary components return existing expression types, so users do not need
 * to add the tag. Framework-defined primitives add it to opt into composition.
 */
template <class T>
concept view_expression = requires { typename std::remove_cvref_t<T>::is_stdui_expression; };

/// Matches deferred callables accepted by `component`.
template <class Body>
concept component_body = std::invocable<Body, component_context &> &&
                         view_expression<std::invoke_result_t<Body, component_context &>>;

/// Matches explicit identities accepted by `identified`.
template <class T>
concept explicit_id = std::integral<std::remove_cvref_t<T>> ||
                      std::convertible_to<std::remove_cvref_t<T>, std::string> ||
                      std::same_as<std::remove_cvref_t<T>, std::string_view>;

/// Owns display text.
struct text_expression {
  using is_stdui_expression = void;

  std::string value;
};

/// Creates a leaf expression that owns @p value.
inline auto text(std::string value) { return text_expression{std::move(value)}; }

/// Orders children vertically as semantic structure, without producing geometry.
template <view_expression... T> struct vstack_expression {
  using is_stdui_expression = void;

  std::tuple<T...> children;
};

/// Creates a value expression owning the supplied children in their written order.
template <view_expression... T> auto vstack(T &&...x) {
  return vstack_expression<std::decay_t<T>...>{{std::forward<T>(x)...}};
}

/// Orders children horizontally as semantic structure, without producing geometry.
template <view_expression... T> struct hstack_expression {
  using is_stdui_expression = void;

  std::tuple<T...> children;
};

/// Creates a value expression owning the supplied children in their written order.
template <view_expression... T> auto hstack(T &&...x) {
  return hstack_expression<std::decay_t<T>...>{{std::forward<T>(x)...}};
}

/// Selects a stable storage type for an explicit identity.
template <class T> struct id_storage {
  using type =
      std::conditional_t<std::convertible_to<T, std::string>, std::string, std::decay_t<T>>;
};
template <> struct id_storage<std::string_view> {
  using type = std::string;
};

/**
 * Wraps a child expression with an explicit sibling identity.
 *
 * The id replaces the child's positional index during reconciliation. It is
 * local to the parent and distinguishes siblings only; changing it changes
 * identity and therefore resets associated persistent state.
 * This is analogous to SwiftUI's `.id(...)`, expressed as a wrapper because
 * C++ member modifiers need shared extension machinery.
 */
template <explicit_id Id, view_expression Expression> struct identified_expression {
  using is_stdui_expression = void;
  using storage_id_t = typename id_storage<std::decay_t<Id>>::type;

  storage_id_t id;
  Expression expression;
};

/// Creates an explicitly identified child expression.
template <explicit_id Id, view_expression Expression>
auto identified(Id &&id, Expression &&expression) {
  using storage_id_t = typename id_storage<std::decay_t<Id>>::type;
  return identified_expression<storage_id_t, std::decay_t<Expression>>{
      storage_id_t(std::forward<Id>(id)), std::forward<Expression>(expression)};
}

/**
 * A deferred component body plus its nominal component identity.
 *
 * `Kind` is a marker type used as compile-time component identity; it is not
 * stored or instantiated. `Body` must be callable with a
 * `component_context&` and return another view expression. The runtime invokes
 * `body` during reconciliation, after establishing the component identity and
 * state scope.
 */
template <class Kind, component_body Body> struct component_expression {
  using is_stdui_expression = void;

  Body body;
};

/// Creates a lazy, stateful component expression identified by @p Kind.
template <class Kind, component_body Body> auto component(Body &&body) {
  return component_expression<Kind, std::decay_t<Body>>{std::forward<Body>(body)};
}
} // namespace stdui
