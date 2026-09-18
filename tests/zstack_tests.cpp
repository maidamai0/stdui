#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdui/core/zstack.hpp>

#include <vector>

namespace {

struct zstack_box {
  stdui::size fixed_size;

  auto measure(stdui::proposal const &) const -> stdui::size { return fixed_size; }
};

} // namespace

TEST_CASE("zstack measurement uses maximum child extent") {
  std::vector<zstack_box> children{
      {{2.0, 5.0}},
      {{6.0, 3.0}},
      {{4.0, 7.0}},
  };

  auto result = stdui::measure_zstack(children, stdui::proposal::unbounded());

  CHECK(result.extent == stdui::size{6.0, 7.0});
  CHECK(result.children[0] == stdui::size{2.0, 5.0});
  CHECK(result.children[1] == stdui::size{6.0, 3.0});
  CHECK(result.children[2] == stdui::size{4.0, 7.0});
}

TEST_CASE("zstack arrangement aligns all children to the same bounds") {
  std::vector<stdui::size> const child_sizes{{2.0, 4.0}, {6.0, 3.0}};
  stdui::zstack_options options{
      .alignment = stdui::layout_alignment::start,
  };

  auto frames = stdui::arrange_zstack(child_sizes, {{0.0, 0.0}, {10.0, 10.0}}, options);

  REQUIRE(frames.size() == 2);
  CHECK(frames[0] == stdui::rect{{0.0, 0.0}, {2.0, 4.0}});
  CHECK(frames[1] == stdui::rect{{0.0, 0.0}, {6.0, 3.0}});
}

TEST_CASE("zstack center alignment centers each child") {
  std::vector<stdui::size> const child_sizes{{2.0, 4.0}};
  stdui::zstack_options options{
      .alignment = stdui::layout_alignment::center,
  };

  auto frames = stdui::arrange_zstack(child_sizes, {{0.0, 0.0}, {10.0, 10.0}}, options);

  REQUIRE(frames.size() == 1);
  CHECK(frames[0] == stdui::rect{{4.0, 3.0}, {2.0, 4.0}});
}

TEST_CASE("zstack stretch alignment fills all children") {
  std::vector<stdui::size> const child_sizes{{2.0, 4.0}, {6.0, 3.0}};
  stdui::zstack_options options{
      .alignment = stdui::layout_alignment::start,
      .sizing = stdui::cross_axis_sizing::stretch,
  };

  auto frames = stdui::arrange_zstack(child_sizes, {{0.0, 0.0}, {10.0, 10.0}}, options);

  REQUIRE(frames.size() == 2);
  CHECK(frames[0] == stdui::rect{{0.0, 0.0}, {10.0, 10.0}});
  CHECK(frames[1] == stdui::rect{{0.0, 0.0}, {10.0, 10.0}});
}

TEST_CASE("combined zstack layout measures and arranges") {
  std::vector<zstack_box> children{
      {{2.0, 4.0}},
      {{6.0, 3.0}},
  };
  stdui::zstack_options options{
      .alignment = stdui::layout_alignment::center,
  };

  auto result = stdui::layout_zstack(children, {{0.0, 0.0}, {10.0, 10.0}}, options);

  CHECK(result.measurement.extent == stdui::size{6.0, 4.0});
  REQUIRE(result.frames.size() == 2);
  CHECK(result.frames[0] == stdui::rect{{4.0, 3.0}, {2.0, 4.0}});
  CHECK(result.frames[1] == stdui::rect{{2.0, 3.5}, {6.0, 3.0}});
}

TEST_CASE("zstack: empty children") {
  std::vector<zstack_box> children;

  auto result = stdui::measure_zstack(children, stdui::proposal::unbounded());

  CHECK(result.extent == stdui::size{0.0, 0.0});
  CHECK(result.children.empty());
}

TEST_CASE("zstack: single child") {
  std::vector<zstack_box> children{{{10.0, 20.0}}};

  auto result = stdui::measure_zstack(children, stdui::proposal::unbounded());

  CHECK(result.extent == stdui::size{10.0, 20.0});
  CHECK(result.children.size() == 1);
}

TEST_CASE("zstack: many children different sizes") {
  std::vector<zstack_box> children{
      {{1.0, 1.0}}, {{5.0, 3.0}}, {{3.0, 8.0}}, {{7.0, 2.0}}, {{2.0, 6.0}},
  };

  auto result = stdui::measure_zstack(children, stdui::proposal::unbounded());

  CHECK(result.extent == stdui::size{7.0, 8.0}); // Max width and height
  CHECK(result.children.size() == 5);
}

TEST_CASE("zstack: end alignment") {
  std::vector<stdui::size> const child_sizes{{5.0, 5.0}};
  stdui::zstack_options options{.alignment = stdui::layout_alignment::end};

  auto frames = stdui::arrange_zstack(child_sizes, {{0.0, 0.0}, {20.0, 20.0}}, options);

  REQUIRE(frames.size() == 1);
  CHECK(frames[0] == stdui::rect{{15.0, 15.0}, {5.0, 5.0}});
}

TEST_CASE("zstack: start alignment with offset bounds") {
  std::vector<stdui::size> const child_sizes{{5.0, 5.0}};
  stdui::zstack_options options{.alignment = stdui::layout_alignment::start};

  auto frames = stdui::arrange_zstack(child_sizes, {{10.0, 20.0}, {30.0, 40.0}}, options);

  REQUIRE(frames.size() == 1);
  CHECK(frames[0].origin == stdui::point{10.0, 20.0});
}

TEST_CASE("zstack: center alignment with small child in large bounds") {
  std::vector<stdui::size> const child_sizes{{2.0, 2.0}};
  stdui::zstack_options options{.alignment = stdui::layout_alignment::center};

  auto frames = stdui::arrange_zstack(child_sizes, {{0.0, 0.0}, {100.0, 100.0}}, options);

  REQUIRE(frames.size() == 1);
  CHECK(frames[0] == stdui::rect{{49.0, 49.0}, {2.0, 2.0}});
}

TEST_CASE("zstack: stretch with multiple children") {
  std::vector<stdui::size> const child_sizes{{5.0, 5.0}, {10.0, 10.0}, {3.0, 3.0}};
  stdui::zstack_options options{.alignment = stdui::layout_alignment::start,
                                .sizing = stdui::cross_axis_sizing::stretch};

  auto frames = stdui::arrange_zstack(child_sizes, {{0.0, 0.0}, {50.0, 60.0}}, options);

  REQUIRE(frames.size() == 3);
  // All stretched to full bounds
  CHECK(frames[0] == stdui::rect{{0.0, 0.0}, {50.0, 60.0}});
  CHECK(frames[1] == stdui::rect{{0.0, 0.0}, {50.0, 60.0}});
  CHECK(frames[2] == stdui::rect{{0.0, 0.0}, {50.0, 60.0}});
}

TEST_CASE("zstack: same sized children") {
  std::vector<zstack_box> children{
      {{10.0, 10.0}},
      {{10.0, 10.0}},
      {{10.0, 10.0}},
  };

  auto result = stdui::measure_zstack(children, stdui::proposal::unbounded());

  CHECK(result.extent == stdui::size{10.0, 10.0});
}

TEST_CASE("zstack: zero-sized child among others") {
  std::vector<zstack_box> children{
      {{0.0, 0.0}},
      {{10.0, 10.0}},
      {{5.0, 5.0}},
  };

  auto result = stdui::measure_zstack(children, stdui::proposal::unbounded());

  CHECK(result.extent == stdui::size{10.0, 10.0});
}

TEST_CASE("zstack: all zero-sized children") {
  std::vector<zstack_box> children{
      {{0.0, 0.0}},
      {{0.0, 0.0}},
  };

  auto result = stdui::measure_zstack(children, stdui::proposal::unbounded());

  CHECK(result.extent == stdui::size{0.0, 0.0});
}
