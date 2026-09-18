#pragma once

#include <cstddef>
#include <optional>
#include <vector>

namespace stdui {

/// Semantic writing direction used to resolve leading and trailing alignment.
enum class layout_direction {
  left_to_right,
  right_to_left,
};

/// Traversal order along a stack's main axis.
enum class stack_direction {
  forward,
  reverse,
};

/// Cross-axis alignment for children in a layout container.
enum class layout_alignment {
  start,
  center,
  end,
};

/// Whether a child keeps its intrinsic cross-axis size or fills available space.
enum class cross_axis_sizing {
  intrinsic,
  stretch,
};

/// Space-distribution policy for flex layout.
struct flex_policy {
  double grow = 0.0;
  bool fill = false;
};

/// Padding applied to all four edges.
struct edge_insets {
  double left = 0.0;
  double top = 0.0;
  double right = 0.0;
  double bottom = 0.0;

  static auto all(double value) -> edge_insets { return {value, value, value, value}; }
};

/// Configuration for horizontal and vertical stacks.
struct stack_options {
  /// Forward means left-to-right for hstack and top-to-bottom for vstack.
  stack_direction direction = stack_direction::forward;
  layout_alignment alignment = layout_alignment::start;
  cross_axis_sizing cross_axis = cross_axis_sizing::intrinsic;

  /// When unset, the stack uses zero spacing.
  std::optional<double> spacing;
};

/// Configuration for z-order stacks.
struct zstack_options {
  layout_alignment alignment = layout_alignment::start;
  cross_axis_sizing sizing = cross_axis_sizing::intrinsic;
};

/// Sizing strategy for one grid column or row.
struct grid_track {
  enum class kind {
    fixed,
    flexible,
  };

  kind type = kind::flexible;
  double size = 1.0;

  static auto fixed(double value) -> grid_track { return {kind::fixed, value}; }

  static auto flexible(double minimum = 0.0) -> grid_track { return {kind::flexible, minimum}; }
};

inline auto repeat_track(grid_track track, std::size_t count) -> std::vector<grid_track> {
  return std::vector<grid_track>(count, track);
}

/// Configuration for grid layouts.
struct grid_options {
  std::vector<grid_track> columns{grid_track::flexible()};
  std::optional<double> row_spacing;
  std::optional<double> column_spacing;
  layout_alignment alignment = layout_alignment::start;
  cross_axis_sizing sizing = cross_axis_sizing::intrinsic;
};

/// SwiftUI-style frame constraints applied to one child.
struct frame_options {
  std::optional<double> width;
  std::optional<double> min_width;
  std::optional<double> ideal_width;
  std::optional<double> max_width;
  std::optional<double> height;
  std::optional<double> min_height;
  std::optional<double> ideal_height;
  std::optional<double> max_height;
  layout_alignment alignment = layout_alignment::center;
};

} // namespace stdui
