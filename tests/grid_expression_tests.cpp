#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdui/expressions.hpp>

TEST_CASE("grid_expression: can be created with default options") {
  auto expr = stdui::grid(stdui::text("A"), stdui::text("B"), stdui::text("C"));

  static_assert(stdui::view_expression<decltype(expr)>);
  CHECK(expr.options.columns == 1);
  CHECK(expr.options.row_spacing == 0.0);
  CHECK(expr.options.column_spacing == 0.0);
}

TEST_CASE("grid_expression: accepts explicit grid_options") {
  stdui::grid_options opts{.columns = 3, .row_spacing = 5.0, .column_spacing = 10.0};
  auto expr = stdui::grid(opts, stdui::text("A"), stdui::text("B"), stdui::text("C"),
                          stdui::text("D"), stdui::text("E"));

  CHECK(expr.options.columns == 3);
  CHECK(expr.options.row_spacing == 5.0);
  CHECK(expr.options.column_spacing == 10.0);
  CHECK(std::tuple_size_v<decltype(expr.children)> == 5);
}

TEST_CASE("grid_expression: stores children correctly") {
  auto expr = stdui::grid(stdui::text("First"), stdui::text("Second"));

  CHECK(std::tuple_size_v<decltype(expr.children)> == 2);
}

TEST_CASE("grid_expression: composes with other expressions") {
  auto expr = stdui::vstack(stdui::text("Header"),
                            stdui::grid(stdui::grid_options{.columns = 2}, stdui::text("A"),
                                        stdui::text("B"), stdui::text("C"), stdui::text("D")),
                            stdui::text("Footer"));

  static_assert(stdui::view_expression<decltype(expr)>);
}
