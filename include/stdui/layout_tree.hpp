#pragma once

#include <stdui/geometry.hpp>
#include <stdui/grid.hpp>
#include <stdui/inspection.hpp>
#include <stdui/layout.hpp>
#include <stdui/overlay.hpp>

#include <algorithm>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace stdui {

/// Measurement function for text leaves during layout materialization.
using text_measure_fn = std::function<size(std::string_view)>;

/// Layout node categories materialized from view expressions.
enum class layout_kind {
  text,
  hstack,
  vstack,
  overlay,
  grid,
  passthrough,
};

inline auto to_string(layout_kind kind) -> std::string {
  switch (kind) {
  case layout_kind::text:
    return "text";
  case layout_kind::hstack:
    return "hstack";
  case layout_kind::vstack:
    return "vstack";
  case layout_kind::overlay:
    return "overlay";
  case layout_kind::grid:
    return "grid";
  case layout_kind::passthrough:
    return "passthrough";
  }
  return "passthrough";
}

inline auto to_layout_kind(std::string_view kind) -> layout_kind {
  if (kind == "text") {
    return layout_kind::text;
  }
  if (kind == "hstack") {
    return layout_kind::hstack;
  }
  if (kind == "vstack") {
    return layout_kind::vstack;
  }
  if (kind == "overlay") {
    return layout_kind::overlay;
  }
  if (kind == "grid") {
    return layout_kind::grid;
  }
  return layout_kind::passthrough;
}

/// Positioned subtree produced by arranging a materialized layout node.
/**
 * "box" refers to a view layout rectangle, not a rendered image frame.
 */
struct layout_box {
  std::string kind;
  std::string content;
  rect bounds;
  std::vector<layout_box> children;
};

/// Persistent layout node materialized from a view expression snapshot.
struct layout_node {
  layout_kind kind = layout_kind::passthrough;
  std::string content;
  std::vector<layout_node> children;

  text_measure_fn text_measure;
  flex_policy policy{};
  stack_options stack;
  grid_options grid;
  overlay_options overlay;

  auto flex() const -> stdui::flex_policy { return policy; }

  auto measure(proposal const &proposal) const -> size {
    switch (kind) {
    case layout_kind::text:
      return clamp_size(text_measure ? text_measure(content) : size{}, proposal);
    case layout_kind::hstack:
      return measure_stack(proposal, detail::stack_axis::horizontal);
    case layout_kind::vstack:
      return measure_stack(proposal, detail::stack_axis::vertical);
    case layout_kind::overlay:
      return clamp_size(measure_overlay(children, proposal).extent, proposal);
    case layout_kind::grid:
      return clamp_size(measure_grid(children, proposal, grid).extent, proposal);
    case layout_kind::passthrough:
      return measure_passthrough(proposal);
    }
    return {};
  }

  auto arrange(rect const &bounds) const -> layout_box {
    layout_box box{to_string(kind), content, bounds, {}};

    if (kind == layout_kind::text || children.empty()) {
      auto measured = kind == layout_kind::text
                          ? clamp_size(text_measure ? text_measure(content) : size{},
                                       proposal::bounded(bounds.extent.width, bounds.extent.height))
                          : size{};
      box.bounds = {bounds.origin, measured};
      return box;
    }

    auto child_bounds = arrange_children(bounds);
    box.children.reserve(children.size());
    for (std::size_t i = 0; i < children.size(); ++i) {
      box.children.push_back(children[i].arrange(child_bounds[i]));
    }
    return box;
  }

private:
  auto measure_stack(proposal const &proposal, detail::stack_axis axis) const -> size {
    auto content_proposal = inset_proposal(proposal, stack.padding);
    layout_result result;
    if (axis == detail::stack_axis::horizontal) {
      result = measure_hstack(children, content_proposal, stack.spacing);
    } else {
      result = measure_vstack(children, content_proposal, stack.spacing);
    }
    return add_padding(clamp_size(result.extent, content_proposal), stack.padding);
  }

  auto measure_passthrough(proposal const &proposal) const -> size {
    if (children.empty()) {
      return {};
    }
    size result;
    for (auto const &child : children) {
      auto child_size = child.measure(proposal);
      result.width = std::max(result.width, child_size.width);
      result.height = std::max(result.height, child_size.height);
    }
    return result;
  }

  auto arrange_children(rect const &bounds) const -> std::vector<rect> {
    switch (kind) {
    case layout_kind::hstack:
      return layout_hstack(children, bounds, stack).frames;
    case layout_kind::vstack:
      return layout_vstack(children, bounds, stack).frames;
    case layout_kind::overlay:
      return layout_overlay(children, bounds, overlay).frames;
    case layout_kind::grid:
      return layout_grid(children, bounds, grid).frames;
    case layout_kind::text:
    case layout_kind::passthrough:
      return std::vector<rect>(children.size(), bounds);
    }
    return {};
  }

  static auto inset_proposal(proposal value, edge_insets const &insets) -> proposal {
    if (value.width.max) {
      value.width.max = std::max(0.0, *value.width.max - insets.left - insets.right);
    }
    if (value.height.max) {
      value.height.max = std::max(0.0, *value.height.max - insets.top - insets.bottom);
    }
    return value;
  }

  static auto add_padding(size value, edge_insets const &insets) -> size {
    value.width += insets.left + insets.right;
    value.height += insets.top + insets.bottom;
    return value;
  }
};

/// Creates a layout tree from a headless expression snapshot.
inline auto materialize_layout(inspection_node const &node, text_measure_fn text_measure)
    -> layout_node {
  layout_node result;
  result.kind = to_layout_kind(node.kind);
  result.content = node.content;

  switch (result.kind) {
  case layout_kind::text:
    result.text_measure = std::move(text_measure);
    result.policy.grow = 0.0;
    break;
  case layout_kind::hstack:
    result.stack.direction = layout_direction::left_to_right;
    break;
  case layout_kind::vstack:
    result.stack.direction = layout_direction::left_to_right;
    break;
  case layout_kind::overlay:
    result.overlay.alignment = layout_alignment::start;
    break;
  case layout_kind::grid:
    result.grid.columns = 1;
    break;
  case layout_kind::passthrough:
    break;
  }

  result.children.reserve(node.children.size());
  for (auto const &child : node.children) {
    result.children.push_back(materialize_layout(child, text_measure));
  }

  return result;
}

} // namespace stdui
