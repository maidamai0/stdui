#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdui/layout.hpp>

#include <vector>

namespace {

struct fixed_box {
  stdui::size fixed_size;
  stdui::flex_policy policy;

  auto measure(stdui::proposal const &) const -> stdui::size { return fixed_size; }
  auto flex() const -> stdui::flex_policy { return policy; }
};

} // namespace

TEST_CASE("layout element concept accepts measurable values") {
  static_assert(stdui::layout_element<fixed_box>);
}

TEST_CASE("horizontal measurement sums widths and uses maximum height") {
  std::vector<fixed_box> children{
      {{2.0, 5.0}, {1.0, false}},
      {{3.0, 4.0}, {1.0, false}},
  };

  auto result = stdui::measure_hstack(children, stdui::proposal::unbounded());

  CHECK(result.extent == stdui::size{5.0, 5.0});
  CHECK(result.children[0] == stdui::size{2.0, 5.0});
  CHECK(result.children[1] == stdui::size{3.0, 4.0});
}

TEST_CASE("vertical measurement sums heights and uses maximum width") {
  std::vector<fixed_box> children{
      {{2.0, 5.0}, {1.0, false}},
      {{3.0, 4.0}, {1.0, false}},
  };

  auto result = stdui::measure_vstack(children, stdui::proposal::unbounded());

  CHECK(result.extent == stdui::size{3.0, 9.0});
  CHECK(result.children[0] == stdui::size{2.0, 5.0});
  CHECK(result.children[1] == stdui::size{3.0, 4.0});
}

TEST_CASE("horizontal measurement distributes extra width by flex weight") {
  std::vector<fixed_box> children{
      {{10.0, 10.0}, {1.0, false}},
      {{10.0, 10.0}, {3.0, false}},
  };

  auto result = stdui::measure_hstack(children, stdui::proposal::bounded(40.0, 20.0));

  CHECK(result.extent.width == doctest::Approx(40.0));
  CHECK(result.children[0].width == doctest::Approx(15.0));
  CHECK(result.children[1].width == doctest::Approx(25.0));
}

TEST_CASE("fill policy takes all remaining width") {
  std::vector<fixed_box> children{
      {{10.0, 10.0}, {1.0, false}},
      {{10.0, 10.0}, {0.0, true}},
  };

  auto result = stdui::measure_hstack(children, stdui::proposal::bounded(40.0, 20.0));

  CHECK(result.children[0].width == doctest::Approx(10.0));
  CHECK(result.children[1].width == doctest::Approx(30.0));
}

TEST_CASE("vertical fill policy takes all remaining height") {
  std::vector<fixed_box> children{
      {{10.0, 10.0}, {1.0, false}},
      {{10.0, 10.0}, {0.0, true}},
  };

  auto result = stdui::measure_vstack(children, stdui::proposal::bounded(20.0, 40.0));

  CHECK(result.children[0].height == doctest::Approx(10.0));
  CHECK(result.children[1].height == doctest::Approx(30.0));
}

TEST_CASE("negative finite extra compresses children by flex weight") {
  std::vector<fixed_box> children{
      {{10.0, 10.0}, {1.0, false}},
      {{10.0, 10.0}, {3.0, false}},
  };

  auto result = stdui::measure_hstack(children, stdui::proposal::bounded(10.0, 20.0));

  CHECK(result.children[0].width == doctest::Approx(7.5));
  CHECK(result.children[1].width == doctest::Approx(2.5));
}

TEST_CASE("negative fill extra compresses fill child") {
  std::vector<fixed_box> children{
      {{10.0, 10.0}, {0.0, false}},
      {{10.0, 10.0}, {0.0, true}},
  };

  auto result = stdui::measure_hstack(children, stdui::proposal::bounded(10.0, 20.0));

  CHECK(result.children[0].width == doctest::Approx(10.0));
  CHECK(result.children[1].width == doctest::Approx(0.0));
}

TEST_CASE("negative finite extra compresses vertical children") {
  std::vector<fixed_box> children{
      {{10.0, 10.0}, {1.0, false}},
      {{10.0, 10.0}, {3.0, false}},
  };

  auto result = stdui::measure_vstack(children, stdui::proposal::bounded(20.0, 10.0));

  CHECK(result.children[0].height == doctest::Approx(7.5));
  CHECK(result.children[1].height == doctest::Approx(2.5));
}

TEST_CASE("negative fill extra compresses vertical fill child") {
  std::vector<fixed_box> children{
      {{10.0, 10.0}, {0.0, false}},
      {{10.0, 10.0}, {0.0, true}},
  };

  auto result = stdui::measure_vstack(children, stdui::proposal::bounded(20.0, 10.0));

  CHECK(result.children[0].height == doctest::Approx(10.0));
  CHECK(result.children[1].height == doctest::Approx(0.0));
}

TEST_CASE("zero flexibility cannot compress children") {
  std::vector<fixed_box> children{
      {{10.0, 10.0}, {0.0, false}},
      {{10.0, 10.0}, {0.0, false}},
  };

  auto result = stdui::measure_hstack(children, stdui::proposal::bounded(5.0, 10.0));

  CHECK(result.children[0].width == doctest::Approx(5.0));
  CHECK(result.children[1].width == doctest::Approx(5.0));
}

TEST_CASE("minimum proposal bound is enforced") {
  std::vector<fixed_box> children{{{1.0, 1.0}, {1.0, false}}};
  stdui::proposal proposal;
  proposal.width.min = 3.0;
  proposal.height.min = 4.0;

  auto result = stdui::measure_hstack(children, proposal);

  CHECK(result.extent == stdui::size{3.0, 4.0});
}

TEST_CASE("maximum proposal bound is enforced") {
  std::vector<fixed_box> children{{{10.0, 10.0}, {1.0, false}}};

  auto result = stdui::measure_hstack(children, stdui::proposal::bounded(5.0, 5.0));

  CHECK(result.extent == stdui::size{5.0, 5.0});
}

TEST_CASE("horizontal arrangement places frames left to right") {
  std::vector<stdui::size> const child_sizes{{2.0, 4.0}, {3.0, 5.0}};

  auto frames = stdui::arrange_hstack(child_sizes, {{0.0, 0.0}, {10.0, 10.0}});

  REQUIRE(frames.size() == 2);
  CHECK(frames[0] == stdui::rect{{0.0, 0.0}, {2.0, 4.0}});
  CHECK(frames[1] == stdui::rect{{2.0, 0.0}, {3.0, 5.0}});
}

TEST_CASE("horizontal center alignment offsets child") {
  std::vector<stdui::size> const child_sizes{{2.0, 4.0}};

  auto frames = stdui::arrange_hstack(child_sizes, {{0.0, 0.0}, {10.0, 10.0}},
                                      stdui::layout_direction::left_to_right,
                                      stdui::layout_alignment::center);

  REQUIRE(frames.size() == 1);
  CHECK(frames[0].origin == stdui::point{0.0, 3.0});
}

TEST_CASE("horizontal end alignment offsets child") {
  std::vector<stdui::size> const child_sizes{{2.0, 4.0}};

  auto frames =
      stdui::arrange_hstack(child_sizes, {{0.0, 0.0}, {10.0, 10.0}},
                            stdui::layout_direction::left_to_right, stdui::layout_alignment::end);

  REQUIRE(frames.size() == 1);
  CHECK(frames[0].origin == stdui::point{0.0, 6.0});
}

TEST_CASE("horizontal stretch alignment fills cross axis") {
  std::vector<stdui::size> const child_sizes{{2.0, 4.0}};

  auto frames = stdui::arrange_hstack(child_sizes, {{0.0, 0.0}, {10.0, 10.0}},
                                      stdui::layout_direction::left_to_right,
                                      stdui::layout_alignment::stretch);

  REQUIRE(frames.size() == 1);
  CHECK(frames[0] == stdui::rect{{0.0, 0.0}, {2.0, 10.0}});
}

TEST_CASE("horizontal arrangement supports right-to-left direction") {
  std::vector<stdui::size> const child_sizes{{2.0, 4.0}, {3.0, 5.0}};

  auto frames = stdui::arrange_hstack(child_sizes, {{0.0, 0.0}, {10.0, 10.0}},
                                      stdui::layout_direction::right_to_left);

  REQUIRE(frames.size() == 2);
  CHECK(frames[0] == stdui::rect{{8.0, 0.0}, {2.0, 4.0}});
  CHECK(frames[1] == stdui::rect{{5.0, 0.0}, {3.0, 5.0}});
}

TEST_CASE("vertical center alignment offsets child on cross axis") {
  std::vector<stdui::size> const child_sizes{{2.0, 4.0}};

  auto frames = stdui::arrange_vstack(child_sizes, {{0.0, 0.0}, {10.0, 10.0}},
                                      stdui::layout_alignment::center);

  REQUIRE(frames.size() == 1);
  CHECK(frames[0] == stdui::rect{{4.0, 0.0}, {2.0, 4.0}});
}

TEST_CASE("vertical end alignment offsets child") {
  std::vector<stdui::size> const child_sizes{{2.0, 4.0}};

  auto frames =
      stdui::arrange_vstack(child_sizes, {{0.0, 0.0}, {10.0, 10.0}}, stdui::layout_alignment::end);

  REQUIRE(frames.size() == 1);
  CHECK(frames[0].origin == stdui::point{8.0, 0.0});
}

TEST_CASE("vertical stretch alignment fills cross axis") {
  std::vector<stdui::size> const child_sizes{{2.0, 4.0}};

  auto frames = stdui::arrange_vstack(child_sizes, {{0.0, 0.0}, {10.0, 10.0}},
                                      stdui::layout_alignment::stretch);

  REQUIRE(frames.size() == 1);
  CHECK(frames[0] == stdui::rect{{0.0, 0.0}, {10.0, 4.0}});
}

TEST_CASE("vertical arrangement places frames top to bottom") {
  std::vector<stdui::size> const child_sizes{{2.0, 4.0}, {3.0, 5.0}};

  auto frames = stdui::arrange_vstack(child_sizes, {{0.0, 0.0}, {10.0, 10.0}});

  REQUIRE(frames.size() == 2);
  CHECK(frames[0] == stdui::rect{{0.0, 0.0}, {2.0, 4.0}});
  CHECK(frames[1] == stdui::rect{{0.0, 4.0}, {3.0, 5.0}});
}

TEST_CASE("combined horizontal layout measures and arranges") {
  std::vector<fixed_box> children{
      {{2.0, 4.0}, {1.0, false}},
      {{3.0, 5.0}, {1.0, false}},
  };

  auto result = stdui::layout_hstack(children, {{0.0, 0.0}, {10.0, 10.0}});

  CHECK(result.measurement.extent == stdui::size{10.0, 5.0});
  REQUIRE(result.frames.size() == 2);
  CHECK(result.frames[0].origin == stdui::point{0.0, 0.0});
  CHECK(result.frames[0].extent.width == doctest::Approx(4.5));
  CHECK(result.frames[0].extent.height == 4.0);
  CHECK(result.frames[1].origin == stdui::point{4.5, 0.0});
  CHECK(result.frames[1].extent.width == doctest::Approx(5.5));
  CHECK(result.frames[1].extent.height == 5.0);
}

TEST_CASE("combined vertical layout measures and arranges") {
  std::vector<fixed_box> children{
      {{2.0, 4.0}, {1.0, false}},
      {{3.0, 5.0}, {1.0, false}},
  };

  auto result = stdui::layout_vstack(children, {{0.0, 0.0}, {10.0, 10.0}});

  CHECK(result.measurement.extent == stdui::size{3.0, 10.0});
  REQUIRE(result.frames.size() == 2);
  CHECK(result.frames[0].origin == stdui::point{0.0, 0.0});
  CHECK(result.frames[0].extent.width == 2.0);
  CHECK(result.frames[0].extent.height == doctest::Approx(4.5));
  CHECK(result.frames[1].origin == stdui::point{0.0, 4.5});
  CHECK(result.frames[1].extent.width == 3.0);
  CHECK(result.frames[1].extent.height == doctest::Approx(5.5));
}
