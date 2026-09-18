#pragma once

#include <stdui/core/expressions.hpp>

#include <optional>
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
  std::optional<stack_options> stack;
  std::optional<zstack_options> zstack;
  std::optional<grid_options> grid;
  std::optional<frame_options> frame;
  std::optional<edge_insets> padding;
  std::optional<size> spacer_minimum;
};

/// Converts text into its inspection representation.
inline auto inspect(text_expression const &x) { return inspection_node{"text", x.value, {}}; }

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

/// Converts a zstack and its children into an inspection tree.
template <class... T> auto inspect(zstack_expression<T...> const &x) {
  inspection_node n{"zstack", {}, {}};
  n.zstack = x.options;
  std::apply([&](auto const &...c) { (n.children.push_back(inspect(c)), ...); }, x.children);
  return n;
}

/// Converts a grid and its children into an inspection tree.
template <class... T> auto inspect(grid_expression<T...> const &x) {
  inspection_node n{"grid", {}, {}};
  n.grid = x.options;
  std::apply([&](auto const &...c) { (n.children.push_back(inspect(c)), ...); }, x.children);
  return n;
}

/// Converts a spacer into an inspection leaf.
inline auto inspect(spacer_expression const &x) {
  inspection_node node{"spacer", {}, {}};
  node.spacer_minimum = x.minimum;
  return node;
}

/// Converts a padded child into an inspection tree.
template <class Expression> auto inspect(padding_expression<Expression> const &x) {
  return inspection_node{"padding", {}, {inspect(x.expression)}, {}, {}, {}, {}, x.insets, {}};
}

/// Converts a framed child into an inspection tree.
template <class Expression> auto inspect(frame_expression<Expression> const &x) {
  return inspection_node{"frame", {}, {inspect(x.expression)}, {}, {}, {}, x.options, {}, {}};
}

} // namespace stdui
