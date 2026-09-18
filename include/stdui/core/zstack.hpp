#pragma once

#include <stdui/core/geometry.hpp>
#include <stdui/core/layout.hpp>

#include <algorithm>
#include <concepts>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace stdui {

/// Measures overlapping children as a single composite extent.
template <std::ranges::input_range Range>
  requires layout_element<std::ranges::range_value_t<Range>>
auto measure_zstack(Range const &children, proposal const &proposal) -> layout_result {
  layout_result result;

  for (auto &&child : children) {
    auto child_size = clamp_size(child.measure(proposal), proposal);
    result.extent.width = std::max(result.extent.width, child_size.width);
    result.extent.height = std::max(result.extent.height, child_size.height);
    result.children.push_back(child_size);
  }

  result.extent = clamp_size(result.extent, proposal);
  return result;
}

/// Places measured children within the same bounds.
inline auto arrange_zstack(std::span<size const> child_sizes, rect const &bounds,
                           zstack_options const &options) -> std::vector<rect> {
  std::vector<rect> frames;
  frames.reserve(child_sizes.size());

  for (auto child_size : child_sizes) {
    double x = bounds.origin.x;
    double y = bounds.origin.y;

    if (options.alignment == layout_alignment::center) {
      x += (bounds.extent.width - child_size.width) * 0.5;
      y += (bounds.extent.height - child_size.height) * 0.5;
    } else if (options.alignment == layout_alignment::end) {
      x += bounds.extent.width - child_size.width;
      y += bounds.extent.height - child_size.height;
    }
    if (options.sizing == cross_axis_sizing::stretch) {
      child_size.width = bounds.extent.width;
      child_size.height = bounds.extent.height;
    }

    frames.push_back({{x, y}, child_size});
  }

  return frames;
}

/// Measures and arranges children in one z-stack pass.
template <std::ranges::input_range Range>
  requires layout_element<std::ranges::range_value_t<Range>>
auto layout_zstack(Range const &children, rect const &bounds, zstack_options const &options)
    -> arranged_layout {
  auto measurement =
      measure_zstack(children, proposal::bounded(bounds.extent.width, bounds.extent.height));
  auto frames = arrange_zstack(measurement.children, bounds, options);
  return {std::move(measurement), std::move(frames)};
}

} // namespace stdui
