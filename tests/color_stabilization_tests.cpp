#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdui/color.hpp>

namespace {

auto same_color(stdui::color const &lhs, stdui::color const &rhs) -> bool {
  return lhs.red == doctest::Approx(rhs.red) && lhs.green == doctest::Approx(rhs.green) &&
         lhs.blue == doctest::Approx(rhs.blue) && lhs.alpha == doctest::Approx(rhs.alpha);
}

} // namespace

TEST_CASE("color: exposes channels and standard values") {
  stdui::color value{0.1f, 0.2f, 0.3f, 0.4f};

  CHECK(value.red == doctest::Approx(0.1f));
  CHECK(value.green == doctest::Approx(0.2f));
  CHECK(value.blue == doctest::Approx(0.3f));
  CHECK(value.alpha == doctest::Approx(0.4f));

  CHECK(same_color(stdui::color::black(), stdui::color{0.0f, 0.0f, 0.0f, 1.0f}));
  CHECK(same_color(stdui::color::white(), stdui::color{1.0f, 1.0f, 1.0f, 1.0f}));
  CHECK(same_color(stdui::color::red_color(), stdui::color{1.0f, 0.0f, 0.0f, 1.0f}));
  CHECK(same_color(stdui::color::green_color(), stdui::color{0.0f, 1.0f, 0.0f, 1.0f}));
  CHECK(same_color(stdui::color::blue_color(), stdui::color{0.0f, 0.0f, 1.0f, 1.0f}));
  CHECK(same_color(stdui::color::transparent(), stdui::color{0.0f, 0.0f, 0.0f, 0.0f}));
}

TEST_CASE("color: converts byte and packed representations") {
  auto value = stdui::color::from_rgb(255, 128, 0, 64);

  CHECK(value.red == doctest::Approx(1.0f));
  CHECK(value.green == doctest::Approx(128.0f / 255.0f));
  CHECK(value.blue == doctest::Approx(0.0f));
  CHECK(value.alpha == doctest::Approx(64.0f / 255.0f));
  CHECK(value.to_rgba() == 0xFF800040U);
}
