#pragma once

#include <stdui/core/geometry.hpp>
#include <stdui/core/layout.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace stdui {

namespace detail {

inline auto resolved_columns(grid_options const &options) -> std::vector<grid_track> {
  if (options.columns.empty()) {
    return {grid_track::flexible()};
  }
  return options.columns;
}

inline auto track_width(grid_track const &track) -> double { return std::max(0.0, track.size); }

inline auto intrinsic_column_widths(std::span<size const> child_sizes,
                                    std::vector<grid_track> const &columns) -> std::vector<double> {
  std::vector<double> widths(columns.size(), 0.0);
  for (std::size_t i = 0; i < columns.size(); ++i) {
    widths[i] = track_width(columns[i]);
  }

  for (std::size_t i = 0; i < child_sizes.size(); ++i) {
    auto column = i % columns.size();
    widths[column] = std::max(widths[column], child_sizes[i].width);
  }
  return widths;
}

inline void distribute_column_width(std::vector<double> &widths,
                                    std::vector<grid_track> const &columns, double available,
                                    double spacing) {
  if (widths.empty()) {
    return;
  }

  double current = spacing * static_cast<double>(widths.size() - 1);
  for (double width : widths) {
    current += width;
  }

  double extra = available - current;
  if (extra <= 0.0) {
    return;
  }

  std::size_t flexible_count = 0;
  for (auto const &column : columns) {
    if (column.type == grid_track::kind::flexible) {
      ++flexible_count;
    }
  }
  if (flexible_count == 0) {
    return;
  }

  double share = extra / static_cast<double>(flexible_count);
  for (std::size_t i = 0; i < widths.size(); ++i) {
    if (columns[i].type == grid_track::kind::flexible) {
      widths[i] += share;
    }
  }
}

inline auto intrinsic_row_heights(std::span<size const> child_sizes, std::size_t column_count)
    -> std::vector<double> {
  std::size_t rows = column_count == 0 ? 0 : (child_sizes.size() + column_count - 1) / column_count;
  std::vector<double> heights(rows, 0.0);

  for (std::size_t i = 0; i < child_sizes.size(); ++i) {
    auto row = i / column_count;
    heights[row] = std::max(heights[row], child_sizes[i].height);
  }
  return heights;
}

inline void distribute_row_height(std::vector<double> &heights, double available, double spacing) {
  if (heights.empty()) {
    return;
  }

  double current = spacing * static_cast<double>(heights.size() - 1);
  for (double height : heights) {
    current += height;
  }

  double extra = available - current;
  if (extra <= 0.0) {
    return;
  }

  double share = extra / static_cast<double>(heights.size());
  for (double &height : heights) {
    height += share;
  }
}

} // namespace detail

/// Measures children into a fixed or flexible column grid.
template <std::ranges::input_range Range>
  requires layout_element<std::ranges::range_value_t<Range>>
auto measure_grid(Range const &children, proposal const &proposal, grid_options const &options)
    -> layout_result {
  layout_result result;
  auto columns = detail::resolved_columns(options);
  double column_spacing = options.column_spacing.value_or(0.0);
  double row_spacing = options.row_spacing.value_or(0.0);

  std::size_t index = 0;
  for (auto &&child : children) {
    auto child_size = clamp_size(child.measure(proposal), proposal);
    result.children.push_back(child_size);
    ++index;
  }

  auto column_widths = detail::intrinsic_column_widths(result.children, columns);
  auto row_heights = detail::intrinsic_row_heights(result.children, columns.size());

  if (proposal.width.max) {
    detail::distribute_column_width(column_widths, columns, *proposal.width.max, column_spacing);
  }
  if (proposal.height.max) {
    detail::distribute_row_height(row_heights, *proposal.height.max, row_spacing);
  }

  for (double width : column_widths) {
    result.extent.width += width;
  }
  if (!column_widths.empty()) {
    result.extent.width += column_spacing * static_cast<double>(column_widths.size() - 1);
  }

  for (double height : row_heights) {
    result.extent.height += height;
  }
  if (!row_heights.empty()) {
    result.extent.height += row_spacing * static_cast<double>(row_heights.size() - 1);
  }

  result.extent = clamp_size(result.extent, proposal);
  return result;
}

/// Places measured children into grid cells.
inline auto arrange_grid(std::span<size const> child_sizes, rect const &bounds,
                         grid_options const &options) -> std::vector<rect> {
  std::vector<rect> frames;
  frames.reserve(child_sizes.size());

  auto columns = detail::resolved_columns(options);
  double column_spacing = options.column_spacing.value_or(0.0);
  double row_spacing = options.row_spacing.value_or(0.0);

  auto column_widths = detail::intrinsic_column_widths(child_sizes, columns);
  auto row_heights = detail::intrinsic_row_heights(child_sizes, columns.size());
  detail::distribute_column_width(column_widths, columns, bounds.extent.width, column_spacing);
  detail::distribute_row_height(row_heights, bounds.extent.height, row_spacing);

  std::vector<double> column_x(columns.size(), bounds.origin.x);
  for (std::size_t column = 1; column < columns.size(); ++column) {
    column_x[column] = column_x[column - 1] + column_widths[column - 1] + column_spacing;
  }

  std::vector<double> row_y(row_heights.size(), bounds.origin.y);
  for (std::size_t row = 1; row < row_heights.size(); ++row) {
    row_y[row] = row_y[row - 1] + row_heights[row - 1] + row_spacing;
  }

  for (std::size_t i = 0; i < child_sizes.size(); ++i) {
    auto column = i % columns.size();
    auto row = i / columns.size();
    size child_size = child_sizes[i];

    double x = column_x[column];
    if (options.alignment == layout_alignment::center) {
      x += (column_widths[column] - child_size.width) * 0.5;
    } else if (options.alignment == layout_alignment::end) {
      x += column_widths[column] - child_size.width;
    }
    if (options.sizing == cross_axis_sizing::stretch) {
      child_size.width = column_widths[column];
    }

    double y = row_y[row];
    if (options.alignment == layout_alignment::center) {
      y += (row_heights[row] - child_size.height) * 0.5;
    } else if (options.alignment == layout_alignment::end) {
      y += row_heights[row] - child_size.height;
    }
    if (options.sizing == cross_axis_sizing::stretch) {
      child_size.height = row_heights[row];
    }

    frames.push_back({{x, y}, child_size});
  }

  return frames;
}

/// Measures and arranges children in one grid pass.
template <std::ranges::input_range Range>
  requires layout_element<std::ranges::range_value_t<Range>>
auto layout_grid(Range const &children, rect const &bounds, grid_options const &options)
    -> arranged_layout {
  auto measurement =
      measure_grid(children, proposal::bounded(bounds.extent.width, bounds.extent.height), options);
  auto frames = arrange_grid(measurement.children, bounds, options);
  return {std::move(measurement), std::move(frames)};
}

} // namespace stdui
