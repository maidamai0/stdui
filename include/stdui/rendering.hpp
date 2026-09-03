#pragma once

#include <stdui/geometry.hpp>
#include <stdui/text_measurement.hpp>

#include <cstdint>
#include <memory>
#include <string_view>

namespace stdui {

/// RGBA color representation (0.0 to 1.0 per channel).
struct color {
  double red = 0.0;
  double green = 0.0;
  double blue = 0.0;
  double alpha = 1.0;

  static auto black() -> color { return {0.0, 0.0, 0.0, 1.0}; }
  static auto white() -> color { return {1.0, 1.0, 1.0, 1.0}; }
  static auto transparent() -> color { return {0.0, 0.0, 0.0, 0.0}; }

  bool operator==(color const &) const = default;
};

/// Line cap styles for stroked paths.
enum class line_cap { butt, round, square };

/// Line join styles for stroked paths.
enum class line_join { miter, round, bevel };

/// Stroke style for drawing operations.
struct stroke_style {
  double width = 1.0;
  line_cap cap = line_cap::butt;
  line_join join = line_join::miter;
  double miter_limit = 10.0;
};

/**
 * Platform-agnostic rendering interface.
 *
 * Implementations wrap platform graphics APIs:
 * - macOS: Core Graphics or Metal
 * - Linux: Cairo or Skia
 * - Windows: Direct2D or Skia
 */
class renderer {
public:
  virtual ~renderer() = default;

  /// Begins a new rendering frame. Must be called before any draw operations.
  virtual void begin_frame() = 0;

  /// Ends the current rendering frame and presents to the display.
  virtual void end_frame() = 0;

  /// Clears the entire canvas with the specified color.
  virtual void clear(color const &fill_color) = 0;

  /// Saves the current graphics state (transforms, clips).
  virtual void save() = 0;

  /// Restores the most recently saved graphics state.
  virtual void restore() = 0;

  /// Translates the coordinate system.
  virtual void translate(double dx, double dy) = 0;

  /// Clips rendering to the specified rectangle.
  virtual void clip_rect(rect const &bounds) = 0;

  /// Fills a rectangle with the specified color.
  virtual void fill_rect(rect const &bounds, color const &fill_color) = 0;

  /// Strokes a rectangle with the specified color and style.
  virtual void stroke_rect(rect const &bounds, color const &stroke_color,
                           stroke_style const &style) = 0;

  /// Draws text at the specified position.
  virtual void draw_text(std::string_view text, point const &position,
                         font_descriptor const &font, color const &text_color) = 0;

  /// Returns the text measurer associated with this renderer.
  virtual auto get_text_measurer() const -> stdui::text_measurer const & = 0;
};

/**
 * Factory for creating platform-specific renderers.
 *
 * Platform backends provide implementations that create renderers
 * bound to native windows or surfaces.
 */
class renderer_factory {
public:
  virtual ~renderer_factory() = default;

  /// Creates a renderer for the specified surface size.
  virtual auto create(size surface_size) -> std::unique_ptr<renderer> = 0;
};

/**
 * Null renderer for testing or headless operation.
 *
 * All draw operations are no-ops. Useful for testing layout
 * algorithms without requiring a display.
 */
class null_renderer : public renderer {
public:
  explicit null_renderer(std::unique_ptr<text_measurer> measurer)
      : measurer_(std::move(measurer)) {}

  void begin_frame() override {}
  void end_frame() override {}
  void clear(color const &) override {}
  void save() override {}
  void restore() override {}
  void translate(double, double) override {}
  void clip_rect(rect const &) override {}
  void fill_rect(rect const &, color const &) override {}
  void stroke_rect(rect const &, color const &, stroke_style const &) override {}
  void draw_text(std::string_view, point const &, font_descriptor const &,
                 color const &) override {}

  auto get_text_measurer() const -> stdui::text_measurer const & override { return *measurer_; }

private:
  std::unique_ptr<stdui::text_measurer> measurer_;
};

} // namespace stdui
