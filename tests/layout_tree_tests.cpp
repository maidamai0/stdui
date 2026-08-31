#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdui/layout_tree.hpp>

namespace {

auto measured_text(std::string_view text) -> stdui::size {
  return {8.0 * static_cast<double>(text.size()), 16.0};
}

} // namespace

TEST_CASE("layout tree materializes and measures nested stacks") {
  auto snapshot = stdui::inspect(
      stdui::vstack(stdui::text("AB"), stdui::hstack(stdui::text("C"), stdui::text("DE"))));
  auto tree = stdui::materialize_layout(snapshot, measured_text);

  CHECK(tree.kind == stdui::layout_kind::vstack);
  CHECK(tree.measure(stdui::proposal::unbounded()) == stdui::size{24.0, 32.0});
}

TEST_CASE("layout tree arranges child frames recursively") {
  auto snapshot = stdui::inspect(
      stdui::vstack(stdui::text("AB"), stdui::hstack(stdui::text("C"), stdui::text("DE"))));
  auto tree = stdui::materialize_layout(snapshot, measured_text);

  auto frame = tree.arrange({{0.0, 0.0}, {100.0, 100.0}});

  REQUIRE(frame.children.size() == 2);
  CHECK(frame.children[0].kind == "text");
  CHECK(frame.children[0].bounds == stdui::rect{{0.0, 0.0}, {16.0, 16.0}});

  REQUIRE(frame.children[1].children.size() == 2);
  CHECK(frame.children[1].children[0].bounds.origin == stdui::point{0.0, 16.0});
  CHECK(frame.children[1].children[1].bounds.origin == stdui::point{8.0, 16.0});
}

TEST_CASE("layout tree materializes overlay nodes") {
  auto snapshot = stdui::inspect(stdui::overlay(stdui::text("A"), stdui::text("BB")));
  auto tree = stdui::materialize_layout(snapshot, measured_text);

  CHECK(tree.kind == stdui::layout_kind::overlay);
  CHECK(tree.measure(stdui::proposal::unbounded()) == stdui::size{16.0, 16.0});
}
