#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdui/overlay.hpp>

#include <vector>

namespace {

struct overlay_box {
  stdui::size fixed_size;

  auto measure(stdui::proposal const &) const -> stdui::size { return fixed_size; }
};

} // namespace

TEST_CASE("overlay measurement uses maximum child extent") {
  std::vector<overlay_box> children{
      {{2.0, 5.0}},
      {{6.0, 3.0}},
      {{4.0, 7.0}},
  };

  auto result = stdui::measure_overlay(children, stdui::proposal::unbounded());

  CHECK(result.extent == stdui::size{6.0, 7.0});
  CHECK(result.children[0] == stdui::size{2.0, 5.0});
  CHECK(result.children[1] == stdui::size{6.0, 3.0});
  CHECK(result.children[2] == stdui::size{4.0, 7.0});
}

TEST_CASE("overlay arrangement aligns all children to the same bounds") {
  std::vector<stdui::size> const child_sizes{{2.0, 4.0}, {6.0, 3.0}};
  stdui::overlay_options options{
      .alignment = stdui::layout_alignment::start,
  };

  auto frames = stdui::arrange_overlay(child_sizes, {{0.0, 0.0}, {10.0, 10.0}}, options);

  REQUIRE(frames.size() == 2);
  CHECK(frames[0] == stdui::rect{{0.0, 0.0}, {2.0, 4.0}});
  CHECK(frames[1] == stdui::rect{{0.0, 0.0}, {6.0, 3.0}});
}

TEST_CASE("overlay center alignment centers each child") {
  std::vector<stdui::size> const child_sizes{{2.0, 4.0}};
  stdui::overlay_options options{
      .alignment = stdui::layout_alignment::center,
  };

  auto frames = stdui::arrange_overlay(child_sizes, {{0.0, 0.0}, {10.0, 10.0}}, options);

  REQUIRE(frames.size() == 1);
  CHECK(frames[0] == stdui::rect{{4.0, 3.0}, {2.0, 4.0}});
}

TEST_CASE("overlay stretch alignment fills all children") {
  std::vector<stdui::size> const child_sizes{{2.0, 4.0}, {6.0, 3.0}};
  stdui::overlay_options options{
      .alignment = stdui::layout_alignment::stretch,
  };

  auto frames = stdui::arrange_overlay(child_sizes, {{0.0, 0.0}, {10.0, 10.0}}, options);

  REQUIRE(frames.size() == 2);
  CHECK(frames[0] == stdui::rect{{0.0, 0.0}, {10.0, 10.0}});
  CHECK(frames[1] == stdui::rect{{0.0, 0.0}, {10.0, 10.0}});
}

TEST_CASE("combined overlay layout measures and arranges") {
  std::vector<overlay_box> children{
      {{2.0, 4.0}},
      {{6.0, 3.0}},
  };
  stdui::overlay_options options{
      .alignment = stdui::layout_alignment::center,
  };

  auto result = stdui::layout_overlay(children, {{0.0, 0.0}, {10.0, 10.0}}, options);

  CHECK(result.measurement.extent == stdui::size{6.0, 4.0});
  REQUIRE(result.frames.size() == 2);
  CHECK(result.frames[0] == stdui::rect{{4.0, 3.0}, {2.0, 4.0}});
  CHECK(result.frames[1] == stdui::rect{{2.0, 3.5}, {6.0, 3.0}});
}
