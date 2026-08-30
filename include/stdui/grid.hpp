#pragma once

#include <stdui/geometry.hpp>
#include <stdui/layout.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace stdui {

/// Configuration for a grid layout.
struct grid_options {
  std::size_t columns = 1;
  double row_spacing = 0.0;
  double column_spacing = 0.0;
  layout_alignment cell_alignment = layout_alignment::stretch;
};

/// Measures children into a fixed-column grid.
template <std::ranges::input_range Range>
  requires layout_element<std::ranges::range_value_t<Range>>
auto measure_grid(Range const &children, proposal const &proposal, grid_options const &options)
    -> layout_result {
  layout_result result;
  std::vector<double> column_widths(options.columns, 0.0);
  std::vector<double> row_heights;

  std::size_t index = 0;
  for (auto &&child : children) {
    auto child_size = clamp_size(child.measure(proposal), proposal);
    std::size_t column = index % options.columns;
    std::size_t row = index / options.columns;

    if (row >= row_heights.size()) {
      row_heights.push_back(0.0);
    }

    column_widths[column] = std::max(column_widths[column], child_size.width);
    row_heights[row] = std::max(row_heights[row], child_size.height);
    result.children.push_back(child_size);
    ++index;
  }

  result.extent.width = 0.0;
  for (double width : column_widths) {
    result.extent.width += width;
  }
  if (!column_widths.empty()) {
    result.extent.width += options.column_spacing * static_cast<double>(column_widths.size() - 1);
  }

  result.extent.height = 0.0;
  for (double height : row_heights) {
    result.extent.height += height;
  }
  if (!row_heights.empty()) {
    result.extent.height += options.row_spacing * static_cast<double>(row_heights.size() - 1);
  }

  result.extent = clamp_size(result.extent, proposal);
  return result;
}

/// Places measured children into grid cells.
inline auto arrange_grid(std::span<size const> child_sizes, rect const &bounds,
                         grid_options const &options) -> std::vector<rect> {
  std::vector<rect> frames;
  frames.reserve(child_sizes.size());

  if (options.columns == 0) {
    return frames;
  }

  std::size_t rows = (child_sizes.size() + options.columns - 1) / options.columns;
  std::vector<double> column_widths(options.columns, 0.0);
  std::vector<double> row_heights(rows, 0.0);

  for (std::size_t i = 0; i < child_sizes.size(); ++i) {
    std::size_t column = i % options.columns;
    std::size_t row = i / options.columns;
    column_widths[column] = std::max(column_widths[column], child_sizes[i].width);
    row_heights[row] = std::max(row_heights[row], child_sizes[i].height);
  }

  double intrinsic_width = 0.0;
  for (double width : column_widths) {
    intrinsic_width += width;
  }
  if (!column_widths.empty()) {
    intrinsic_width += options.column_spacing * static_cast<double>(column_widths.size() - 1);
  }

  double intrinsic_height = 0.0;
  for (double height : row_heights) {
    intrinsic_height += height;
  }
  if (!row_heights.empty()) {
    intrinsic_height += options.row_spacing * static_cast<double>(row_heights.size() - 1);
  }

  double extra_width = std::max(0.0, bounds.extent.width - intrinsic_width);
  double extra_height = std::max(0.0, bounds.extent.height - intrinsic_height);

  for (double &width : column_widths) {
    width += extra_width / static_cast<double>(column_widths.size());
  }
  for (double &height : row_heights) {
    height += extra_height / static_cast<double>(row_heights.size());
  }

  std::vector<double> column_x(options.columns, bounds.origin.x);
  for (std::size_t column = 1; column < options.columns; ++column) {
    column_x[column] = column_x[column - 1] + column_widths[column - 1] + options.column_spacing;
  }

  std::vector<double> row_y(rows, bounds.origin.y);
  for (std::size_t row = 1; row < rows; ++row) {
    row_y[row] = row_y[row - 1] + row_heights[row - 1] + options.row_spacing;
  }

  for (std::size_t i = 0; i < child_sizes.size(); ++i) {
    std::size_t column = i % options.columns;
    std::size_t row = i / options.columns;
    size child_size = child_sizes[i];

    double x = column_x[column];
    if (options.cell_alignment == layout_alignment::center) {
      x += (column_widths[column] - child_size.width) * 0.5;
    } else if (options.cell_alignment == layout_alignment::end) {
      x += column_widths[column] - child_size.width;
    } else if (options.cell_alignment == layout_alignment::stretch) {
      child_size.width = column_widths[column];
    }

    double y = row_y[row];
    if (options.cell_alignment == layout_alignment::center) {
      y += (row_heights[row] - child_size.height) * 0.5;
    } else if (options.cell_alignment == layout_alignment::end) {
      y += row_heights[row] - child_size.height;
    } else if (options.cell_alignment == layout_alignment::stretch) {
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
