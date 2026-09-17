#pragma once

#include <cstdint>

namespace stdui {

/// Interpolation strategies supported by the core animation model.
enum class animation_curve_kind {
  linear,
  ease_in,
  ease_out,
  ease_in_out,
  spring,
};

/// Declarative timing and interpolation parameters.
struct animation_curve {
  animation_curve_kind kind = animation_curve_kind::linear;
  double duration_seconds = 0.3;
  double damping = 1.0;
  double initial_velocity = 0.0;
};

/// Declarative animation behavior attached to a value or modifier.
struct animation_spec {
  animation_curve curve;
  bool repeat = false;
  bool autoreverse = false;
};

/// Endpoints for a transition between two values of the same type.
template <class T> struct animation_transition {
  T from;
  T to;
};

} // namespace stdui
