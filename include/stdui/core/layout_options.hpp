#pragma once

#include <cstddef>

namespace stdui {

/// Order in which children are arranged along a stack's main axis.
enum class layout_direction {
  forward,
  reverse,
};

/// Cross-axis alignment for stack children.
enum class layout_alignment {
  start,
  center,
  end,
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
};

/// Configuration for stack layouts.
struct stack_options {
  /// Forward means left-to-right for hstack and top-to-bottom for vstack.
  layout_direction direction = layout_direction::forward;
  layout_alignment alignment = layout_alignment::start;
  double spacing = 0.0;
  edge_insets padding;
};

/// Configuration for zstack layouts.
struct zstack_options {
  layout_alignment alignment = layout_alignment::start;
};

/// Configuration for grid layouts.
struct grid_options {
  std::size_t columns = 1;
  double row_spacing = 0.0;
  double column_spacing = 0.0;
  layout_alignment cell_alignment = layout_alignment::stretch;
};

} // namespace stdui
