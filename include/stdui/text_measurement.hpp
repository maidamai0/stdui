#pragma once

#include <stdui/geometry.hpp>

#include <memory>
#include <string>
#include <string_view>

namespace stdui {

/**
 * Font description for text measurement.
 *
 * This is a platform-agnostic font specification. Platform backends
 * resolve it to native font handles.
 */
struct font_descriptor {
  std::string family = "system";
  double size = 14.0;
  enum class weight_t { regular, bold } weight = weight_t::regular;
  enum class style_t { normal, italic } style = style_t::normal;

  bool operator==(font_descriptor const &) const = default;
};

/**
 * Platform-agnostic text measurement interface.
 *
 * Implementations wrap platform text engines:
 * - macOS: Core Text
 * - Linux: Pango or HarfBuzz
 * - Windows: DirectWrite
 */
class text_measurer {
public:
  virtual ~text_measurer() = default;

  /// Measures the bounding box of the given text with the specified font.
  virtual auto measure(std::string_view text, font_descriptor const &font) const -> size = 0;

  /// Measures text constrained to a maximum width (for wrapping).
  virtual auto measure_wrapped(std::string_view text, font_descriptor const &font,
                                double max_width) const -> size = 0;
};

/**
 * Factory for creating platform-specific text measurers.
 *
 * The runtime provides a default implementation. Applications can
 * inject custom measurers for testing or specialized rendering.
 */
class text_measurer_factory {
public:
  virtual ~text_measurer_factory() = default;

  /// Creates a text measurer for the current platform.
  virtual auto create() const -> std::unique_ptr<text_measurer> = 0;
};

/// Convenience wrapper that caches font metrics.
class cached_text_measurer : public text_measurer {
public:
  explicit cached_text_measurer(std::unique_ptr<text_measurer> backend)
      : backend_(std::move(backend)) {}

  auto measure(std::string_view text, font_descriptor const &font) const -> size override {
    // TODO: Implement caching strategy
    return backend_->measure(text, font);
  }

  auto measure_wrapped(std::string_view text, font_descriptor const &font,
                       double max_width) const -> size override {
    return backend_->measure_wrapped(text, font, max_width);
  }

private:
  std::unique_ptr<text_measurer> backend_;
};

} // namespace stdui
