#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdui/color.hpp>

namespace {

auto same_color(stdui::color const &lhs, stdui::color const &rhs) -> bool {
  return lhs.r == doctest::Approx(rhs.r) && lhs.g == doctest::Approx(rhs.g) &&
         lhs.b == doctest::Approx(rhs.b) && lhs.a == doctest::Approx(rhs.a);
}

} // namespace

TEST_CASE("color: exposes channels and standard values") {
  stdui::color value{0.1f, 0.2f, 0.3f, 0.4f};

  CHECK(value.r == doctest::Approx(0.1f));
  CHECK(value.g == doctest::Approx(0.2f));
  CHECK(value.b == doctest::Approx(0.3f));
  CHECK(value.a == doctest::Approx(0.4f));

  CHECK(same_color(stdui::color::black(), stdui::color{0.0f, 0.0f, 0.0f, 1.0f}));
  CHECK(same_color(stdui::color::white(), stdui::color{1.0f, 1.0f, 1.0f, 1.0f}));
  CHECK(same_color(stdui::color::red(), stdui::color{1.0f, 0.0f, 0.0f, 1.0f}));
  CHECK(same_color(stdui::color::green(), stdui::color{0.0f, 1.0f, 0.0f, 1.0f}));
  CHECK(same_color(stdui::color::blue(), stdui::color{0.0f, 0.0f, 1.0f, 1.0f}));
  CHECK(same_color(stdui::color::transparent(), stdui::color{0.0f, 0.0f, 0.0f, 0.0f}));
}

TEST_CASE("color: converts byte and packed representations") {
  auto value = stdui::color::from_rgb(255, 128, 0, 64);

  CHECK(value.r == doctest::Approx(1.0f));
  CHECK(value.g == doctest::Approx(128.0f / 255.0f));
  CHECK(value.b == doctest::Approx(0.0f));
  CHECK(value.a == doctest::Approx(64.0f / 255.0f));
  CHECK(value.to_rgba() == 0xFF800040U);
}
