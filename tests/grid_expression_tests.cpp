#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdui/core/expressions.hpp>

TEST_CASE("grid_expression: can be created with default options") {
  auto expr = stdui::grid(stdui::text("A"), stdui::text("B"), stdui::text("C"));

  static_assert(stdui::view_expression<decltype(expr)>);
  REQUIRE(expr.options.columns.size() == 1);
  CHECK(expr.options.columns[0].type == stdui::grid_track::kind::flexible);
  CHECK_FALSE(expr.options.row_spacing.has_value());
  CHECK_FALSE(expr.options.column_spacing.has_value());
}

TEST_CASE("grid_expression: accepts explicit grid_options") {
  stdui::grid_options opts{
      .columns = stdui::repeat_track(stdui::grid_track::flexible(), 3),
      .row_spacing = 5.0,
      .column_spacing = 10.0,
  };
  auto expr = stdui::grid(opts, stdui::text("A"), stdui::text("B"), stdui::text("C"),
                          stdui::text("D"), stdui::text("E"));

  CHECK(expr.options.columns.size() == 3);
  CHECK(expr.options.row_spacing == 5.0);
  CHECK(expr.options.column_spacing == 10.0);
  CHECK(std::tuple_size_v<decltype(expr.children)> == 5);
}

TEST_CASE("grid_expression: stores children correctly") {
  auto expr = stdui::grid(stdui::text("First"), stdui::text("Second"));

  CHECK(std::tuple_size_v<decltype(expr.children)> == 2);
}

TEST_CASE("grid_expression: composes with other expressions") {
  auto expr = stdui::vstack(
      stdui::text("Header"),
      stdui::grid(
          stdui::grid_options{.columns = stdui::repeat_track(stdui::grid_track::flexible(), 2)},
          stdui::text("A"), stdui::text("B"), stdui::text("C"), stdui::text("D")),
      stdui::text("Footer"));

  static_assert(stdui::view_expression<decltype(expr)>);
}
