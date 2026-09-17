#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdui/core/text_measurement.hpp>

#include <memory>

namespace {

class recording_text_measurer : public stdui::text_measurer {
public:
  auto measure(std::string_view text, stdui::font_descriptor const &font) const
      -> stdui::size override {
    ++measure_calls;
    last_text = std::string(text);
    last_font = font;
    return {100.0, 20.0};
  }

  auto measure_wrapped(std::string_view text, stdui::font_descriptor const &font,
                       double max_width) const -> stdui::size override {
    ++wrapped_calls;
    last_wrap_width = max_width;
    return {max_width, 40.0};
  }

  mutable int measure_calls = 0;
  mutable int wrapped_calls = 0;
  mutable std::string last_text;
  mutable stdui::font_descriptor last_font;
  mutable double last_wrap_width = 0.0;
};

} // namespace

TEST_CASE("cached text measurer delegates measurement and wrapping") {
  auto backend = std::make_unique<recording_text_measurer>();
  auto *backend_ptr = backend.get();
  stdui::cached_text_measurer measurer(std::move(backend));

  stdui::font_descriptor font{.family = "Test", .size = 18.0};
  CHECK(measurer.measure("hello", font) == stdui::size{100.0, 20.0});
  CHECK(measurer.measure_wrapped("hello", font, 50.0) == stdui::size{50.0, 40.0});

  CHECK(backend_ptr->measure_calls == 1);
  CHECK(backend_ptr->wrapped_calls == 1);
  CHECK(backend_ptr->last_text == "hello");
  CHECK(backend_ptr->last_font == font);
  CHECK(backend_ptr->last_wrap_width == doctest::Approx(50.0));
}
