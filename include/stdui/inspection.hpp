#pragma once

#include <stdui/expressions.hpp>

#include <string>
#include <tuple>
#include <vector>

namespace stdui {

/// Backend-independent snapshot of an expression tree.
/**
 * The current headless runtime returns this representation, and tests inspect
 * it.
 */
struct inspection_node {
  std::string kind;
  std::string content;
  std::vector<inspection_node> children;
  flex_policy policy{1.0, false};
  stack_options stack;
  overlay_options overlay;
};

/// Converts text into its inspection representation.
inline auto inspect(text_expression const &x) {
  inspection_node n{"text", x.value, {}};
  n.policy.grow = 0.0;
  return n;
}

/// Converts a vertical stack and its children into an inspection tree.
template <class... T> auto inspect(vstack_expression<T...> const &x) {
  inspection_node n{"vstack", {}, {}};
  n.stack = x.options;
  std::apply([&](auto const &...c) { (n.children.push_back(inspect(c)), ...); }, x.children);
  return n;
}

/// Converts a horizontal stack and its children into an inspection tree.
template <class... T> auto inspect(hstack_expression<T...> const &x) {
  inspection_node n{"hstack", {}, {}};
  n.stack = x.options;
  std::apply([&](auto const &...c) { (n.children.push_back(inspect(c)), ...); }, x.children);
  return n;
}

/// Converts an overlay and its children into an inspection tree.
template <class... T> auto inspect(overlay_expression<T...> const &x) {
  inspection_node n{"overlay", {}, {}};
  n.overlay = x.options;
  std::apply([&](auto const &...c) { (n.children.push_back(inspect(c)), ...); }, x.children);
  return n;
}

/// Converts a flex-wrapped child and attaches its policy to the child node.
template <class Expression> auto inspect(flex_expression<Expression> const &x) {
  auto n = inspect(x.expression);
  n.policy = x.policy;
  return n;
}

} // namespace stdui
