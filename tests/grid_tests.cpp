#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdui/grid.hpp>

#include <vector>

namespace {

struct grid_box {
  stdui::size fixed_size;

  auto measure(stdui::proposal const &) const -> stdui::size { return fixed_size; }
};

} // namespace

TEST_CASE("grid measurement uses columns rows and spacing") {
  std::vector<grid_box> children{
      {{2.0, 3.0}},
      {{4.0, 5.0}},
      {{6.0, 7.0}},
  };

  stdui::grid_options options{
      .columns = 2,
      .row_spacing = 1.0,
      .column_spacing = 2.0,
      .cell_alignment = stdui::layout_alignment::stretch,
  };

  auto result = stdui::measure_grid(children, stdui::proposal::unbounded(), options);

  CHECK(result.extent == stdui::size{12.0, 13.0});
  CHECK(result.children[0] == stdui::size{2.0, 3.0});
  CHECK(result.children[1] == stdui::size{4.0, 5.0});
  CHECK(result.children[2] == stdui::size{6.0, 7.0});
}

TEST_CASE("grid arrangement places measured children by row") {
  std::vector<stdui::size> const child_sizes{{2.0, 3.0}, {4.0, 5.0}, {6.0, 7.0}};
  stdui::grid_options options{
      .columns = 2,
      .row_spacing = 1.0,
      .column_spacing = 2.0,
      .cell_alignment = stdui::layout_alignment::stretch,
  };

  auto frames = stdui::arrange_grid(child_sizes, {{0.0, 0.0}, {12.0, 9.0}}, options);

  REQUIRE(frames.size() == 3);
  CHECK(frames[0] == stdui::rect{{0.0, 0.0}, {6.0, 5.0}});
  CHECK(frames[1] == stdui::rect{{8.0, 0.0}, {4.0, 5.0}});
  CHECK(frames[2] == stdui::rect{{0.0, 6.0}, {6.0, 7.0}});
}

TEST_CASE("grid center alignment positions content within its cell") {
  std::vector<stdui::size> const child_sizes{{2.0, 3.0}};
  stdui::grid_options options{
      .columns = 1,
      .row_spacing = 0.0,
      .column_spacing = 0.0,
      .cell_alignment = stdui::layout_alignment::center,
  };

  auto frames = stdui::arrange_grid(child_sizes, {{0.0, 0.0}, {10.0, 10.0}}, options);

  REQUIRE(frames.size() == 1);
  CHECK(frames[0] == stdui::rect{{4.0, 3.5}, {2.0, 3.0}});
}

TEST_CASE("combined grid layout measures and arranges") {
  std::vector<grid_box> children{
      {{2.0, 3.0}},
      {{4.0, 5.0}},
      {{6.0, 7.0}},
  };
  stdui::grid_options options{
      .columns = 2,
      .row_spacing = 1.0,
      .column_spacing = 2.0,
      .cell_alignment = stdui::layout_alignment::stretch,
  };

  auto result = stdui::layout_grid(children, {{0.0, 0.0}, {12.0, 9.0}}, options);

  CHECK(result.measurement.extent == stdui::size{12.0, 9.0});
  REQUIRE(result.frames.size() == 3);
  CHECK(result.frames[0] == stdui::rect{{0.0, 0.0}, {6.0, 5.0}});
  CHECK(result.frames[1] == stdui::rect{{8.0, 0.0}, {4.0, 5.0}});
  CHECK(result.frames[2] == stdui::rect{{0.0, 6.0}, {6.0, 7.0}});
}
