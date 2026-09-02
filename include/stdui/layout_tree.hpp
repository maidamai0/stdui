#pragma once

#include <stdui/geometry.hpp>
#include <stdui/inspection.hpp>
#include <stdui/layout.hpp>
#include <stdui/overlay.hpp>

#include <algorithm>
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
  overlay,
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
  case layout_kind::overlay:
    return "overlay";
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
  if (kind == "overlay") {
    return layout_kind::overlay;
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
  overlay_options overlay;

  auto flex() const -> stdui::flex_policy { return policy; }

  auto measure(proposal const &proposal) const -> size {
    if (kind == layout_kind::text) {
      return measure_text(proposal);
    }
    if (kind == layout_kind::overlay) {
      return clamp_size(measure_overlay(children, proposal).extent, proposal);
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
    if (children.empty()) {
      box.bounds = {bounds.origin, {}};
      return box;
    }

    auto child_frames = arrange_children(bounds);
    box.children.reserve(children.size());
    for (std::size_t i = 0; i < children.size(); ++i) {
      box.children.push_back(children[i].arrange(child_frames[i]));
    }
    box.bounds = {bounds.origin, occupied_extent(bounds, box.children)};
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
    auto content_proposal = inset_proposal(proposal, stack.padding);
    layout_result result;
    if (axis == detail::stack_axis::horizontal) {
      result = measure_hstack(children, content_proposal, stack.spacing);
    } else {
      result = measure_vstack(children, content_proposal, stack.spacing);
    }
    return add_padding(clamp_size(result.extent, content_proposal), stack.padding);
  }

  auto arrange_children(rect const &bounds) const -> std::vector<rect> {
    if (kind == layout_kind::hstack) {
      return layout_hstack(children, bounds, stack).frames;
    }
    if (kind == layout_kind::overlay) {
      return layout_overlay(children, bounds, overlay).frames;
    }
    return layout_vstack(children, bounds, stack).frames;
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
  if (result.kind == layout_kind::text) {
    result.text_measure = text_measure;
    result.policy.grow = 0.0;
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
