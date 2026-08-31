#pragma once

#include <stdui/geometry.hpp>
#include <stdui/grid.hpp>
#include <stdui/inspection.hpp>
#include <stdui/layout.hpp>
#include <stdui/overlay.hpp>

#include <algorithm>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace stdui {

/// Measurement function for text leaves during layout materialization.
using text_measure_fn = std::function<size(std::string_view)>;

/// Fully arranged subtree produced from a materialized layout node.
struct layout_frame {
  std::string kind;
  std::string content;
  rect bounds;
  std::vector<layout_frame> children;
};

/// Persistent layout node materialized from a view expression snapshot.
struct layout_node {
  std::string kind;
  std::string content;
  std::vector<layout_node> children;

  text_measure_fn text_measure;
  flex_policy policy{};
  stack_options stack;
  grid_options grid;
  overlay_options overlay;

  auto flex() const -> stdui::flex_policy { return policy; }

  auto measure(proposal const &proposal) const -> size {
    if (text_measure) {
      return clamp_size(text_measure(content), proposal);
    }

    if (kind == "hstack") {
      auto content_proposal = inset_proposal(proposal, stack.padding);
      auto result = measure_hstack(children, content_proposal, stack.spacing);
      return add_padding(clamp_size(result.extent, content_proposal), stack.padding);
    }

    if (kind == "vstack") {
      auto content_proposal = inset_proposal(proposal, stack.padding);
      auto result = measure_vstack(children, content_proposal, stack.spacing);
      return add_padding(clamp_size(result.extent, content_proposal), stack.padding);
    }

    if (kind == "overlay") {
      auto result = measure_overlay(children, proposal);
      return clamp_size(result.extent, proposal);
    }

    if (kind == "grid") {
      auto result = measure_grid(children, proposal, grid);
      return clamp_size(result.extent, proposal);
    }

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

  auto arrange(rect const &bounds) const -> layout_frame {
    layout_frame frame{kind, content, bounds, {}};

    if (text_measure || children.empty()) {
      auto measured = text_measure
                          ? clamp_size(text_measure(content),
                                       proposal::bounded(bounds.extent.width, bounds.extent.height))
                          : size{};
      frame.bounds = {bounds.origin, measured};
      return frame;
    }

    std::vector<rect> child_bounds;
    if (kind == "hstack") {
      child_bounds = layout_hstack(children, bounds, stack).frames;
    } else if (kind == "vstack") {
      child_bounds = layout_vstack(children, bounds, stack).frames;
    } else if (kind == "overlay") {
      child_bounds = layout_overlay(children, bounds, overlay).frames;
    } else if (kind == "grid") {
      child_bounds = layout_grid(children, bounds, grid).frames;
    } else {
      for (auto const &child : children) {
        child_bounds.push_back(bounds);
      }
    }

    frame.children.reserve(children.size());
    for (std::size_t i = 0; i < children.size(); ++i) {
      frame.children.push_back(children[i].arrange(child_bounds[i]));
    }

    return frame;
  }

private:
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
  result.kind = node.kind;
  result.content = node.content;

  if (node.kind == "text") {
    result.text_measure = std::move(text_measure);
    result.policy.grow = 0.0;
  } else if (node.kind == "hstack") {
    result.stack.direction = layout_direction::left_to_right;
  } else if (node.kind == "vstack") {
    result.stack.direction = layout_direction::left_to_right;
  } else if (node.kind == "overlay") {
    result.overlay.alignment = layout_alignment::start;
  } else if (node.kind == "grid") {
    result.grid.columns = 1;
  }

  result.children.reserve(node.children.size());
  for (auto const &child : node.children) {
    result.children.push_back(materialize_layout(child, text_measure));
  }

  return result;
}

} // namespace stdui
