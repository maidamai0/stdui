#pragma once

namespace stdui {

/// Horizontal writing direction used during arrangement.
enum class layout_direction {
  left_to_right,
  right_to_left,
};

/// Cross-axis alignment used while arranging children.
enum class layout_alignment {
  start,
  center,
  end,
  stretch,
};

/// Describes how an element participates in main-axis space distribution.
struct flex_policy {
  double grow = 1.0;
  bool fill = false;
};

/// Uniform or directional padding around a layout.
struct edge_insets {
  double left = 0.0;
  double top = 0.0;
  double right = 0.0;
  double bottom = 0.0;
};

/// Complete configuration for a horizontal or vertical stack.
struct stack_options {
  layout_direction direction = layout_direction::left_to_right;
  layout_alignment alignment = layout_alignment::start;
  double spacing = 0.0;
  edge_insets padding;
};

/// Configuration for overlapping children in the same bounds.
struct overlay_options {
  layout_alignment alignment = layout_alignment::start;
};

} // namespace stdui
