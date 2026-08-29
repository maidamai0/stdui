#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdui/geometry.hpp>

TEST_CASE("geometry value types are default constructed at the origin") {
  stdui::point point;
  stdui::size size;
  stdui::rect rectangle;

  CHECK(point.x == 0.0);
  CHECK(point.y == 0.0);
  CHECK(size.width == 0.0);
  CHECK(size.height == 0.0);
  CHECK(rectangle.origin == point);
  CHECK(rectangle.extent == size);
}

TEST_CASE("rectangle identity includes origin and extent") {
  stdui::rect const rectangle{{10.0, 20.0}, {30.0, 40.0}};

  CHECK(rectangle.origin == stdui::point{10.0, 20.0});
  CHECK(rectangle.extent == stdui::size{30.0, 40.0});
  CHECK(rectangle == stdui::rect{{10.0, 20.0}, {30.0, 40.0}});
  CHECK(rectangle != stdui::rect{{11.0, 20.0}, {30.0, 40.0}});
}

TEST_CASE("rectangle containment uses half-open visual bounds") {
  stdui::rect const rectangle{{0.0, 0.0}, {10.0, 10.0}};

  CHECK(rectangle.contains({0.0, 0.0}));
  CHECK(rectangle.contains({10.0, 10.0}));
  CHECK_FALSE(rectangle.contains({-0.1, 0.0}));
  CHECK_FALSE(rectangle.contains({0.0, 10.1}));
}

TEST_CASE("rectangle inset reduces extent around its center") {
  stdui::rect const rectangle{{0.0, 0.0}, {10.0, 8.0}};

  CHECK(rectangle.inset(1.0) == stdui::rect{{1.0, 1.0}, {8.0, 6.0}});
}
