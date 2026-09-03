#pragma once

#include <stdui/geometry.hpp>
#include <stdui/layout_options.hpp>

#include <algorithm>
#include <concepts>
#include <optional>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace stdui {

/// Optional minimum and maximum bound for one logical axis.
struct size_constraint {
  std::optional<double> min;
  std::optional<double> max;
};

/// Proposal containing one constraint per axis.
struct proposal {
  size_constraint width;
  size_constraint height;

  static auto unbounded() -> proposal { return {}; }

  static auto bounded(double width, double height) -> proposal {
    proposal result;
    result.width.max = width;
    result.height.max = height;
    return result;
  }
};

/// Matches values that can measure themselves against a proposal.
template <class T>
concept layout_element = requires(T const &element, proposal const &proposal) {
  { element.measure(proposal) } -> std::convertible_to<size>;
};

/// Result of measuring a stack and its children.
struct layout_result {
  size extent;
  std::vector<size> children;
};

/// Fully arranged stack result.
struct arranged_layout {
  layout_result measurement;
  std::vector<rect> frames;
};

/// Returns a rectangle inset by the supplied padding.
inline auto inset_rect(rect const &bounds, edge_insets const &insets) -> rect {
  return {{bounds.origin.x + insets.left, bounds.origin.y + insets.top},
          {std::max(0.0, bounds.extent.width - insets.left - insets.right),
           std::max(0.0, bounds.extent.height - insets.top - insets.bottom)}};
}

template <layout_element T> auto layout_flex(T const &element) -> flex_policy;
inline auto clamp_size(size value, proposal const &proposal) -> size;

namespace detail {

enum class stack_axis {
  horizontal,
  vertical,
};

template <stack_axis Axis> struct stack_traits;

template <> struct stack_traits<stack_axis::horizontal> {
  static auto main(size const &value) -> double { return value.width; }
  static auto cross(size const &value) -> double { return value.height; }
  static auto main(size &value) -> double & { return value.width; }
  static auto cross(size &value) -> double & { return value.height; }
  static auto main_constraint(proposal const &value) -> size_constraint const & {
    return value.width;
  }
};

template <> struct stack_traits<stack_axis::vertical> {
  static auto main(size const &value) -> double { return value.height; }
  static auto cross(size const &value) -> double { return value.width; }
  static auto main(size &value) -> double & { return value.height; }
  static auto cross(size &value) -> double & { return value.width; }
  static auto main_constraint(proposal const &value) -> size_constraint const & {
    return value.height;
  }
};

template <stack_axis Axis> inline auto main_value(size const &value) -> double {
  return stack_traits<Axis>::main(value);
}

template <stack_axis Axis> inline auto cross_value(size const &value) -> double {
  return stack_traits<Axis>::cross(value);
}

template <stack_axis Axis> inline auto main_reference(size &value) -> double & {
  return stack_traits<Axis>::main(value);
}

template <stack_axis Axis> inline auto cross_reference(size &value) -> double & {
  return stack_traits<Axis>::cross(value);
}

inline auto flex_share(flex_policy policy, double extra, std::size_t fill_count, double finite_grow)
    -> double {
  if (extra > 0.0 && fill_count > 0) {
    return policy.fill ? extra / static_cast<double>(fill_count) : 0.0;
  }
  if (extra < 0.0 && finite_grow > 0.0) {
    return extra * policy.grow / finite_grow;
  }
  if (extra < 0.0 && fill_count > 0) {
    return policy.fill ? extra / static_cast<double>(fill_count) : 0.0;
  }
  if (finite_grow > 0.0) {
    return extra * policy.grow / finite_grow;
  }
  return 0.0;
}

template <stack_axis Axis>
inline void distribute_flex(layout_result &result, std::span<flex_policy const> flex,
                            double extra) {
  std::size_t fill_count = 0;
  double finite_grow = 0.0;

  for (auto policy : flex) {
    if (policy.fill) {
      ++fill_count;
    } else {
      finite_grow += policy.grow;
    }
  }

  for (std::size_t i = 0; i < result.children.size(); ++i) {
    double share = flex_share(flex[i], extra, fill_count, finite_grow);
    double &main = stack_traits<Axis>::main(result.children[i]);
    main += share;
    main = std::max(0.0, main);
  }
}

template <stack_axis Axis, std::ranges::input_range Range>
  requires layout_element<std::ranges::range_value_t<Range>>
auto measure_stack(Range const &children, proposal const &proposal, double spacing)
    -> layout_result {
  layout_result result;
  std::vector<flex_policy> flex;
  std::size_t child_count = 0;

  for (auto &&child : children) {
    auto child_size = clamp_size(child.measure(proposal), proposal);
    flex_policy child_flex = layout_flex(child);

    main_reference<Axis>(result.extent) += main_value<Axis>(child_size);
    cross_reference<Axis>(result.extent) =
        std::max(cross_value<Axis>(result.extent), cross_value<Axis>(child_size));
    result.children.push_back(child_size);
    flex.push_back(child_flex);
    ++child_count;
  }

  if (child_count > 1) {
    main_reference<Axis>(result.extent) += spacing * static_cast<double>(child_count - 1);
  }

  std::optional<double> max_main = stack_traits<Axis>::main_constraint(proposal).max;
  double initial_main = main_value<Axis>(result.extent);

  if (max_main && initial_main != *max_main) {
    double extra = *max_main - initial_main;
    distribute_flex<Axis>(result, flex, extra);
    main_reference<Axis>(result.extent) = *max_main;
  }

  result.extent = clamp_size(result.extent, proposal);
  return result;
}

} // namespace detail

/// Returns the space-distribution policy of an element.
/**
 * `grow` is a finite relative share. `fill` consumes all remaining space,
 * similar to a spacer or SwiftUI's flexible frame.
 */
template <layout_element T> auto layout_flex(T const &element) -> flex_policy {
  if constexpr (requires { element.flex(); }) {
    return element.flex();
  } else {
    return {};
  }
}

/// Applies one optional bound to a scalar value.
inline auto apply_constraint(double value, size_constraint const &constraint) -> double {
  if (constraint.min && value < *constraint.min) {
    value = *constraint.min;
  }
  if (constraint.max && value > *constraint.max) {
    value = *constraint.max;
  }
  return value;
}

/// Applies proposal constraints to a size.
inline auto clamp_size(size value, proposal const &proposal) -> size {
  return {apply_constraint(value.width, proposal.width),
          apply_constraint(value.height, proposal.height)};
}

/// Measures horizontal children and distributes width by flexibility.
template <std::ranges::input_range Range>
  requires layout_element<std::ranges::range_value_t<Range>>
auto measure_hstack(Range const &children, proposal const &proposal, double spacing = 0.0)
    -> layout_result {
  return detail::measure_stack<detail::stack_axis::horizontal>(children, proposal, spacing);
}

/// Measures vertical children and distributes height by flexibility.
template <std::ranges::input_range Range>
  requires layout_element<std::ranges::range_value_t<Range>>
auto measure_vstack(Range const &children, proposal const &proposal, double spacing = 0.0)
    -> layout_result {
  return detail::measure_stack<detail::stack_axis::vertical>(children, proposal, spacing);
}

/// Places measured horizontal children according to direction and alignment.
inline auto arrange_hstack(std::span<size const> child_sizes, rect const &bounds,
                           layout_direction direction = layout_direction::left_to_right,
                           layout_alignment alignment = layout_alignment::start,
                           double spacing = 0.0) -> std::vector<rect> {
  std::vector<rect> frames;
  frames.reserve(child_sizes.size());

  double x = direction == layout_direction::left_to_right ? bounds.origin.x
                                                          : bounds.origin.x + bounds.extent.width;

  for (auto child_size : child_sizes) {
    if (direction == layout_direction::right_to_left) {
      x -= child_size.width;
    }

    double y = bounds.origin.y;
    if (alignment == layout_alignment::center) {
      y += (bounds.extent.height - child_size.height) * 0.5;
    } else if (alignment == layout_alignment::end) {
      y += bounds.extent.height - child_size.height;
    } else if (alignment == layout_alignment::stretch) {
      child_size.height = bounds.extent.height;
    }

    frames.push_back({{x, y}, child_size});

    if (direction == layout_direction::left_to_right) {
      x += child_size.width + spacing;
    } else {
      x -= spacing;
    }
  }

  return frames;
}

/// Places measured vertical children according to cross-axis alignment.
inline auto arrange_vstack(std::span<size const> child_sizes, rect const &bounds,
                           layout_alignment alignment = layout_alignment::start,
                           double spacing = 0.0) -> std::vector<rect> {
  std::vector<rect> frames;
  frames.reserve(child_sizes.size());

  double y = bounds.origin.y;
  for (auto child_size : child_sizes) {
    double x = bounds.origin.x;
    if (alignment == layout_alignment::center) {
      x += (bounds.extent.width - child_size.width) * 0.5;
    } else if (alignment == layout_alignment::end) {
      x += bounds.extent.width - child_size.width;
    } else if (alignment == layout_alignment::stretch) {
      child_size.width = bounds.extent.width;
    }

    frames.push_back({{x, y}, child_size});
    y += child_size.height + spacing;
  }

  return frames;
}

/// Measures and arranges children in one horizontal stack pass.
template <std::ranges::input_range Range>
  requires layout_element<std::ranges::range_value_t<Range>>
auto layout_hstack(Range const &children, rect const &bounds,
                   layout_direction direction = layout_direction::left_to_right,
                   layout_alignment alignment = layout_alignment::start, double spacing = 0.0)
    -> arranged_layout {
  auto measurement = measure_hstack(
      children, proposal::bounded(bounds.extent.width, bounds.extent.height), spacing);
  auto frames = arrange_hstack(measurement.children, bounds, direction, alignment, spacing);
  return {std::move(measurement), std::move(frames)};
}

/// Measures and arranges children in one vertical stack pass.
template <std::ranges::input_range Range>
  requires layout_element<std::ranges::range_value_t<Range>>
auto layout_vstack(Range const &children, rect const &bounds,
                   layout_alignment alignment = layout_alignment::start, double spacing = 0.0)
    -> arranged_layout {
  auto measurement = measure_vstack(
      children, proposal::bounded(bounds.extent.width, bounds.extent.height), spacing);
  auto frames = arrange_vstack(measurement.children, bounds, alignment, spacing);
  return {std::move(measurement), std::move(frames)};
}

/// Measures and arranges a horizontal stack using complete options.
template <std::ranges::input_range Range>
  requires layout_element<std::ranges::range_value_t<Range>>
auto layout_hstack(Range const &children, rect const &bounds, stack_options const &options)
    -> arranged_layout {
  auto content = inset_rect(bounds, options.padding);
  auto measurement = measure_hstack(
      children, proposal::bounded(content.extent.width, content.extent.height), options.spacing);
  auto frames = arrange_hstack(measurement.children, content, options.direction, options.alignment,
                               options.spacing);
  return {std::move(measurement), std::move(frames)};
}

/// Measures and arranges a vertical stack using complete options.
template <std::ranges::input_range Range>
  requires layout_element<std::ranges::range_value_t<Range>>
auto layout_vstack(Range const &children, rect const &bounds, stack_options const &options)
    -> arranged_layout {
  auto content = inset_rect(bounds, options.padding);
  auto measurement = measure_vstack(
      children, proposal::bounded(content.extent.width, content.extent.height), options.spacing);
  auto frames = arrange_vstack(measurement.children, content, options.alignment, options.spacing);
  return {std::move(measurement), std::move(frames)};
}

} // namespace stdui
