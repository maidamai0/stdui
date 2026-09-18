#pragma once

#include <stdui/core/geometry.hpp>
#include <stdui/core/grid.hpp>
#include <stdui/core/inspection.hpp>
#include <stdui/core/layout.hpp>
#include <stdui/core/zstack.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
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
  zstack,
  grid,
  spacer,
  padding,
  frame,
  dynamic_list,
};

inline auto to_string(layout_kind kind) -> std::string {
  switch (kind) {
  case layout_kind::text:
    return "text";
  case layout_kind::hstack:
    return "hstack";
  case layout_kind::vstack:
    return "vstack";
  case layout_kind::zstack:
    return "zstack";
  case layout_kind::grid:
    return "grid";
  case layout_kind::spacer:
    return "spacer";
  case layout_kind::padding:
    return "padding";
  case layout_kind::frame:
    return "frame";
  case layout_kind::dynamic_list:
    return "dynamic_list";
  }
}

inline auto to_layout_kind(std::string_view kind) -> std::optional<layout_kind> {
  if (kind == "text") {
    return layout_kind::text;
  }
  if (kind == "hstack") {
    return layout_kind::hstack;
  }
  if (kind == "vstack") {
    return layout_kind::vstack;
  }
  if (kind == "zstack") {
    return layout_kind::zstack;
  }
  if (kind == "grid") {
    return layout_kind::grid;
  }
  if (kind == "spacer") {
    return layout_kind::spacer;
  }
  if (kind == "padding") {
    return layout_kind::padding;
  }
  if (kind == "frame") {
    return layout_kind::frame;
  }
  if (kind == "dynamic_list") {
    return layout_kind::dynamic_list;
  }
  return std::nullopt;
}

/// Positioned subtree produced by arranging a materialized layout node.
/**
 * "box" refers to a view layout rectangle, not a rendered image frame.
 * `bounds` is the rect the node's content occupies within its assigned frame:
 * text leaves report their measured extent, containers the union of their
 * arranged children's bounds.
 */
struct layout_box {
  std::string kind;
  std::string content;
  rect bounds;
  std::vector<layout_box> children;
};

/// Persistent layout node materialized from a view expression snapshot.
struct layout_node {
  layout_kind kind = layout_kind::vstack;
  std::string content;
  std::vector<layout_node> children;

  /// Text measurement shared by all text leaves in the tree.
  std::shared_ptr<text_measure_fn const> text_measure;
  flex_policy policy{};
  stack_options stack;
  zstack_options zstack;
  grid_options grid;
  frame_options frame;
  edge_insets padding;
  size spacer_minimum;

  auto flex() const -> stdui::flex_policy { return policy; }

  auto measure(proposal const &proposal) const -> size {
    if (kind == layout_kind::text) {
      return measure_text(proposal);
    }
    if (kind == layout_kind::spacer) {
      return clamp_size(spacer_minimum, proposal);
    }
    if (kind == layout_kind::padding) {
      return measure_padding(proposal);
    }
    if (kind == layout_kind::frame) {
      return measure_frame(proposal);
    }
    if (kind == layout_kind::grid) {
      return clamp_size(measure_grid(children, proposal, grid).extent, proposal);
    }
    if (kind == layout_kind::zstack) {
      return clamp_size(measure_zstack(children, proposal).extent, proposal);
    }
    auto axis =
        kind == layout_kind::hstack ? detail::stack_axis::horizontal : detail::stack_axis::vertical;
    return measure_stack(proposal, axis);
  }

  auto arrange(rect const &bounds) const -> layout_box {
    layout_box box{to_string(kind), content, bounds, {}};

    if (kind == layout_kind::text) {
      box.bounds = {bounds.origin,
                    measure_text(proposal::bounded(bounds.extent.width, bounds.extent.height))};
      return box;
    }
    if (kind == layout_kind::spacer) {
      box.bounds = bounds;
      return box;
    }
    if (children.empty()) {
      box.bounds = {bounds.origin, {}};
      return box;
    }

    auto child_frames = arrange_children(bounds);
    box.children.reserve(children.size());
    for (std::size_t i = 0; i < children.size(); ++i) {
      box.children.push_back(children[i].arrange(child_frames[i]));
    }
    if (kind == layout_kind::padding || kind == layout_kind::frame) {
      box.bounds = {
          bounds.origin,
          measure(proposal::bounded(bounds.extent.width, bounds.extent.height)),
      };
    } else {
      box.bounds = {bounds.origin, occupied_extent(bounds, box.children)};
    }
    return box;
  }

private:
  auto measure_text(proposal const &proposal) const -> size {
    if (text_measure == nullptr) {
      return size{};
    }
    if (!*text_measure) {
      return size{};
    }
    return clamp_size((*text_measure)(content), proposal);
  }

  auto measure_stack(proposal const &proposal, detail::stack_axis axis) const -> size {
    layout_result result;
    if (axis == detail::stack_axis::horizontal) {
      result = measure_hstack(children, proposal, stack.spacing.value_or(0.0));
    } else {
      result = measure_vstack(children, proposal, stack.spacing.value_or(0.0));
    }
    return clamp_size(result.extent, proposal);
  }

  auto arrange_children(rect const &bounds) const -> std::vector<rect> {
    if (kind == layout_kind::hstack) {
      return layout_hstack(children, bounds, stack).frames;
    }
    if (kind == layout_kind::zstack) {
      return layout_zstack(children, bounds, zstack).frames;
    }
    if (kind == layout_kind::grid) {
      return layout_grid(children, bounds, grid).frames;
    }
    if (kind == layout_kind::padding) {
      return {inset_rect(bounds, padding)};
    }
    if (kind == layout_kind::frame) {
      return {align_frame(bounds)};
    }
    return layout_vstack(children, bounds, stack).frames;
  }

  auto measure_padding(proposal const &proposal) const -> size {
    if (children.empty()) {
      return {};
    }
    auto content_proposal = inset_proposal(proposal, padding);
    return add_padding(clamp_size(children.front().measure(content_proposal), content_proposal),
                       padding);
  }

  auto measure_frame(proposal const &proposal) const -> size {
    if (children.empty()) {
      return {};
    }
    auto measured = children.front().measure(proposal);
    return clamp_size({resolve_frame_axis(measured.width, proposal.width.max, frame.width,
                                          frame.min_width, frame.ideal_width, frame.max_width),
                       resolve_frame_axis(measured.height, proposal.height.max, frame.height,
                                          frame.min_height, frame.ideal_height, frame.max_height)},
                      proposal);
  }

  auto align_frame(rect const &bounds) const -> rect {
    if (children.empty()) {
      return {bounds.origin, {}};
    }

    auto frame_extent = measure(proposal::bounded(bounds.extent.width, bounds.extent.height));
    rect frame_bounds{bounds.origin, frame_extent};
    auto child_size = children.front().measure(
        proposal::bounded(frame_bounds.extent.width, frame_bounds.extent.height));

    double x = frame_bounds.origin.x;
    double y = frame_bounds.origin.y;
    if (frame.alignment == layout_alignment::center) {
      x += (frame_bounds.extent.width - child_size.width) * 0.5;
      y += (frame_bounds.extent.height - child_size.height) * 0.5;
    } else if (frame.alignment == layout_alignment::end) {
      x += frame_bounds.extent.width - child_size.width;
      y += frame_bounds.extent.height - child_size.height;
    }
    return {{x, y}, child_size};
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

  static auto resolve_frame_axis(double measured, std::optional<double> proposal_max,
                                 std::optional<double> fixed, std::optional<double> minimum,
                                 std::optional<double> ideal, std::optional<double> maximum)
      -> double {
    if (fixed) {
      return *fixed;
    }

    double result = ideal.value_or(measured);
    if (maximum && std::isinf(*maximum) && proposal_max) {
      result = *proposal_max;
    }
    if (minimum) {
      result = std::max(result, *minimum);
    }
    if (maximum) {
      result = std::min(result, *maximum);
    }
    return result;
  }

  /// Union of the arranged child boxes relative to the assigned bounds.
  static auto occupied_extent(rect const &bounds, std::span<layout_box const> boxes) -> size {
    size result;
    for (auto const &child : boxes) {
      result.width = std::max(result.width,
                              child.bounds.origin.x + child.bounds.extent.width - bounds.origin.x);
      result.height = std::max(result.height, child.bounds.origin.y + child.bounds.extent.height -
                                                  bounds.origin.y);
    }
    return result;
  }
};

/// Creates a layout tree from a headless expression snapshot.
/**
 * Unknown node kinds are rejected rather than assigned fallback geometry:
 * silently overlapping children are harder to diagnose than an exception.
 */
inline auto materialize_layout(inspection_node const &node,
                               std::shared_ptr<text_measure_fn const> text_measure) -> layout_node {
  auto kind = to_layout_kind(node.kind);
  if (!kind) {
    throw std::logic_error("unknown layout kind: " + node.kind);
  }

  layout_node result;
  result.kind = *kind;
  result.content = node.content;
  result.stack = node.stack.value_or(stack_options{});
  result.zstack = node.zstack.value_or(zstack_options{});
  result.grid = node.grid.value_or(grid_options{});
  result.frame = node.frame.value_or(frame_options{});
  result.padding = node.padding.value_or(edge_insets{});
  result.spacer_minimum = node.spacer_minimum.value_or(size{});
  if (result.kind == layout_kind::text) {
    result.text_measure = text_measure;
    result.policy.grow = 0.0;
  } else if (result.kind == layout_kind::spacer) {
    result.policy = flex_policy{.grow = 0.0, .fill = true};
  }

  result.children.reserve(node.children.size());
  for (auto const &child : node.children) {
    result.children.push_back(materialize_layout(child, text_measure));
  }

  return result;
}

inline auto materialize_layout(inspection_node const &node, text_measure_fn text_measure)
    -> layout_node {
  return materialize_layout(node, std::make_shared<text_measure_fn const>(std::move(text_measure)));
}

} // namespace stdui
