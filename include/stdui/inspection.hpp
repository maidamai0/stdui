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
};

/// Converts text into its inspection representation.
inline auto inspect(text_expression const &x) { return inspection_node{"text", x.value, {}}; }

/// Converts a vertical stack and its children into an inspection tree.
template <class... T> auto inspect(vstack_expression<T...> const &x) {
  inspection_node n{"vstack", {}, {}};
  std::apply([&](auto const &...c) { (n.children.push_back(inspect(c)), ...); }, x.children);
  return n;
}

/// Converts a horizontal stack and its children into an inspection tree.
template <class... T> auto inspect(hstack_expression<T...> const &x) {
  inspection_node n{"hstack", {}, {}};
  std::apply([&](auto const &...c) { (n.children.push_back(inspect(c)), ...); }, x.children);
  return n;
}

/// Converts an overlay and its children into an inspection tree.
template <class... T> auto inspect(overlay_expression<T...> const &x) {
  inspection_node n{"overlay", {}, {}};
  std::apply([&](auto const &...c) { (n.children.push_back(inspect(c)), ...); }, x.children);
  return n;
}

} // namespace stdui
