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

TEST_CASE("point: custom values") {
  stdui::point p1{5.5, 10.5};
  CHECK(p1.x == 5.5);
  CHECK(p1.y == 10.5);

  stdui::point p2{-3.0, -7.0};
  CHECK(p2.x == -3.0);
  CHECK(p2.y == -7.0);
}

TEST_CASE("point: equality comparisons") {
  stdui::point p1{1.0, 2.0};
  stdui::point p2{1.0, 2.0};
  stdui::point p3{1.0, 2.1};
  stdui::point p4{1.1, 2.0};

  CHECK(p1 == p2);
  CHECK_FALSE(p1 == p3);
  CHECK_FALSE(p1 == p4);
  CHECK(p1 != p3);
  CHECK(p1 != p4);
}

TEST_CASE("size: custom values") {
  stdui::size s1{100.0, 200.0};
  CHECK(s1.width == 100.0);
  CHECK(s1.height == 200.0);

  stdui::size s2{0.5, 0.75};
  CHECK(s2.width == 0.5);
  CHECK(s2.height == 0.75);
}

TEST_CASE("size: equality comparisons") {
  stdui::size s1{10.0, 20.0};
  stdui::size s2{10.0, 20.0};
  stdui::size s3{10.0, 20.1};
  stdui::size s4{10.1, 20.0};

  CHECK(s1 == s2);
  CHECK_FALSE(s1 == s3);
  CHECK_FALSE(s1 == s4);
  CHECK(s1 != s3);
  CHECK(s1 != s4);
}

TEST_CASE("rect: various positions and sizes") {
  stdui::rect r1{{100.0, 200.0}, {50.0, 75.0}};
  CHECK(r1.origin.x == 100.0);
  CHECK(r1.origin.y == 200.0);
  CHECK(r1.extent.width == 50.0);
  CHECK(r1.extent.height == 75.0);

  stdui::rect r2{{-10.0, -20.0}, {30.0, 40.0}};
  CHECK(r2.origin.x == -10.0);
  CHECK(r2.origin.y == -20.0);
}

TEST_CASE("rect: contains edge cases") {
  stdui::rect rect{{10.0, 10.0}, {20.0, 20.0}};

  // Inside
  CHECK(rect.contains({15.0, 15.0}));
  CHECK(rect.contains({20.0, 20.0}));

  // Edges (inclusive based on implementation)
  CHECK(rect.contains({10.0, 10.0})); // top-left
  CHECK(rect.contains({30.0, 30.0})); // bottom-right
  CHECK(rect.contains({10.0, 30.0})); // bottom-left
  CHECK(rect.contains({30.0, 10.0})); // top-right

  // Outside
  CHECK_FALSE(rect.contains({9.9, 15.0}));
  CHECK_FALSE(rect.contains({30.1, 15.0}));
  CHECK_FALSE(rect.contains({15.0, 9.9}));
  CHECK_FALSE(rect.contains({15.0, 30.1}));
}

TEST_CASE("rect: contains with negative coordinates") {
  stdui::rect rect{{-10.0, -10.0}, {20.0, 20.0}};

  CHECK(rect.contains({0.0, 0.0}));
  CHECK(rect.contains({-10.0, -10.0}));
  CHECK(rect.contains({10.0, 10.0}));
  CHECK_FALSE(rect.contains({-11.0, 0.0}));
  CHECK_FALSE(rect.contains({11.0, 0.0}));
}

TEST_CASE("rect: inset with various amounts") {
  stdui::rect rect{{0.0, 0.0}, {100.0, 100.0}};

  auto inset5 = rect.inset(5.0);
  CHECK(inset5.origin == stdui::point{5.0, 5.0});
  CHECK(inset5.extent == stdui::size{90.0, 90.0});

  auto inset10 = rect.inset(10.0);
  CHECK(inset10.origin == stdui::point{10.0, 10.0});
  CHECK(inset10.extent == stdui::size{80.0, 80.0});

  auto inset0 = rect.inset(0.0);
  CHECK(inset0 == rect);
}

TEST_CASE("rect: inset with non-square rect") {
  stdui::rect rect{{5.0, 10.0}, {30.0, 50.0}};

  auto inset = rect.inset(2.0);
  CHECK(inset.origin == stdui::point{7.0, 12.0});
  CHECK(inset.extent == stdui::size{26.0, 46.0});
}

TEST_CASE("rect: inset with negative result (over-inset)") {
  stdui::rect rect{{0.0, 0.0}, {10.0, 10.0}};

  auto inset = rect.inset(6.0);
  CHECK(inset.origin == stdui::point{6.0, 6.0});
  CHECK(inset.extent == stdui::size{-2.0, -2.0}); // Negative extent
}

TEST_CASE("rect: zero-sized rectangles") {
  stdui::rect rect{{10.0, 10.0}, {0.0, 0.0}};

  CHECK(rect.contains({10.0, 10.0}));
  CHECK_FALSE(rect.contains({10.1, 10.0}));
  CHECK_FALSE(rect.contains({10.0, 10.1}));
}

TEST_CASE("rect: equality with nested structs") {
  stdui::rect r1{{1.0, 2.0}, {3.0, 4.0}};
  stdui::rect r2{{1.0, 2.0}, {3.0, 4.0}};
  stdui::rect r3{{1.0, 2.0}, {3.0, 4.1}};
  stdui::rect r4{{1.1, 2.0}, {3.0, 4.0}};

  CHECK(r1 == r2);
  CHECK_FALSE(r1 == r3);
  CHECK_FALSE(r1 == r4);
}
