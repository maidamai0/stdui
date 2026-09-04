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

TEST_CASE("zero-column grid returns an empty layout") {
  std::vector<grid_box> children{
      {{2.0, 3.0}},
  };
  stdui::grid_options options{
      .columns = 0,
  };

  auto result = stdui::layout_grid(children, {{0.0, 0.0}, {10.0, 10.0}}, options);

  CHECK(result.measurement.extent == stdui::size{0.0, 0.0});
  CHECK(result.frames.empty());
}

TEST_CASE("grid: single column layout") {
  std::vector<grid_box> children{
      {{10.0, 5.0}},
      {{10.0, 8.0}},
      {{10.0, 3.0}},
  };
  stdui::grid_options options{.columns = 1};

  auto result = stdui::measure_grid(children, stdui::proposal::unbounded(), options);

  CHECK(result.extent.width == 10.0);
  CHECK(result.extent.height == 16.0); // 5 + 8 + 3
}

TEST_CASE("grid: single row layout") {
  std::vector<grid_box> children{
      {{5.0, 10.0}},
      {{8.0, 10.0}},
  };
  stdui::grid_options options{.columns = 2};

  auto result = stdui::measure_grid(children, stdui::proposal::unbounded(), options);

  CHECK(result.extent.width == 13.0); // 5 + 8
  CHECK(result.extent.height == 10.0);
}

TEST_CASE("grid: with large spacing") {
  std::vector<grid_box> children{
      {{5.0, 5.0}},
      {{5.0, 5.0}},
      {{5.0, 5.0}},
      {{5.0, 5.0}},
  };
  stdui::grid_options options{
      .columns = 2,
      .row_spacing = 10.0,
      .column_spacing = 20.0,
  };

  auto result = stdui::measure_grid(children, stdui::proposal::unbounded(), options);

  CHECK(result.extent.width == 30.0);  // 5 + 20 + 5
  CHECK(result.extent.height == 20.0); // 5 + 10 + 5
}

TEST_CASE("grid: empty children") {
  std::vector<grid_box> children;
  stdui::grid_options options{.columns = 2};

  auto result = stdui::measure_grid(children, stdui::proposal::unbounded(), options);

  CHECK(result.extent == stdui::size{0.0, 0.0});
  CHECK(result.children.empty());
}

TEST_CASE("grid: alignment start") {
  std::vector<stdui::size> const child_sizes{{5.0, 5.0}};
  stdui::grid_options options{
      .columns = 1,
      .cell_alignment = stdui::layout_alignment::start,
  };

  auto frames = stdui::arrange_grid(child_sizes, {{0.0, 0.0}, {20.0, 20.0}}, options);

  REQUIRE(frames.size() == 1);
  CHECK(frames[0].origin == stdui::point{0.0, 0.0});
  CHECK(frames[0].extent == stdui::size{5.0, 5.0});
}

TEST_CASE("grid: alignment end") {
  std::vector<stdui::size> const child_sizes{{5.0, 5.0}};
  stdui::grid_options options{
      .columns = 1,
      .cell_alignment = stdui::layout_alignment::end,
  };

  auto frames = stdui::arrange_grid(child_sizes, {{0.0, 0.0}, {20.0, 20.0}}, options);

  REQUIRE(frames.size() == 1);
  CHECK(frames[0].origin == stdui::point{15.0, 15.0});
}

TEST_CASE("grid: many columns") {
  std::vector<grid_box> children;
  for (int i = 0; i < 10; ++i) {
    children.push_back({{5.0, 5.0}});
  }

  stdui::grid_options options{.columns = 5};

  auto result = stdui::measure_grid(children, stdui::proposal::unbounded(), options);

  CHECK(result.extent.width == 25.0); // 5 * 5
  CHECK(result.extent.height == 10.0); // 2 rows * 5
  CHECK(result.children.size() == 10);
}

TEST_CASE("grid: uneven last row") {
  std::vector<grid_box> children{
      {{5.0, 5.0}},
      {{5.0, 5.0}},
      {{5.0, 5.0}},
  };
  stdui::grid_options options{.columns = 2};

  auto result = stdui::measure_grid(children, stdui::proposal::unbounded(), options);

  // 2 rows: first with 2 items, second with 1 item
  CHECK(result.extent.height == 10.0);
}

TEST_CASE("grid: different sized cells in same column") {
  std::vector<grid_box> children{
      {{5.0, 10.0}},
      {{5.0, 3.0}},
      {{5.0, 7.0}},
      {{5.0, 15.0}},
  };
  stdui::grid_options options{.columns = 2};

  auto result = stdui::measure_grid(children, stdui::proposal::unbounded(), options);

  // Row heights should be max of each row
  CHECK(result.extent.height == 25.0); // max(10,3) + max(7,15) = 10 + 15
}

TEST_CASE("grid: zero spacing") {
  std::vector<grid_box> children{
      {{5.0, 5.0}},
      {{5.0, 5.0}},
  };
  stdui::grid_options options{
      .columns = 2,
      .row_spacing = 0.0,
      .column_spacing = 0.0,
  };

  auto result = stdui::measure_grid(children, stdui::proposal::unbounded(), options);

  CHECK(result.extent == stdui::size{10.0, 5.0});
}
