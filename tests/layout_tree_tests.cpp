#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdui/layout_tree.hpp>
#include <stdui/runtime.hpp>

#include <vector>

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

TEST_CASE("container boxes report the union of their arranged children") {
  auto snapshot = stdui::inspect(
      stdui::vstack(stdui::text("AB"), stdui::hstack(stdui::text("C"), stdui::text("DE"))));
  auto tree = stdui::materialize_layout(snapshot, measured_text);

  auto frame = tree.arrange({{0.0, 0.0}, {100.0, 100.0}});

  CHECK(frame.bounds == stdui::rect{{0.0, 0.0}, {24.0, 32.0}});
  CHECK(frame.children[1].bounds == stdui::rect{{0.0, 16.0}, {24.0, 16.0}});
}

TEST_CASE("layout tree materializes overlay nodes") {
  auto snapshot = stdui::inspect(stdui::overlay(stdui::text("A"), stdui::text("BB")));
  auto tree = stdui::materialize_layout(snapshot, measured_text);

  CHECK(tree.kind == stdui::layout_kind::overlay);
  CHECK(tree.measure(stdui::proposal::unbounded()) == stdui::size{16.0, 16.0});
}

TEST_CASE("overlay alignment positions children within the shared frame") {
  auto snapshot = stdui::inspect(stdui::overlay(stdui::text("A"), stdui::text("B")));
  auto tree = stdui::materialize_layout(snapshot, measured_text);
  auto bounds = stdui::rect{{0.0, 0.0}, {100.0, 100.0}};

  auto started = tree.arrange(bounds);
  CHECK(started.children[0].bounds == stdui::rect{{0.0, 0.0}, {8.0, 16.0}});

  tree.overlay.alignment = stdui::layout_alignment::center;
  auto centered = tree.arrange(bounds);
  CHECK(centered.children[0].bounds.origin == stdui::point{46.0, 42.0});

  tree.overlay.alignment = stdui::layout_alignment::end;
  auto ended = tree.arrange(bounds);
  CHECK(ended.children[0].bounds.origin == stdui::point{92.0, 84.0});

  tree.overlay.alignment = stdui::layout_alignment::stretch;
  auto stretched = tree.arrange(bounds);
  CHECK(stretched.children[0].bounds == stdui::rect{{0.0, 0.0}, {8.0, 16.0}});
}

TEST_CASE("dynamic list children stack vertically instead of overlapping") {
  stdui::runtime runtime;
  auto snapshot = runtime.reconcile(stdui::dynamic_list(
      std::vector<int>{1, 2, 3}, [](int item) { return item; },
      [](int item) { return stdui::text(std::to_string(item)); }));
  CHECK(snapshot.kind == "dynamic_list");

  auto tree = stdui::materialize_layout(snapshot, measured_text);
  CHECK(tree.kind == stdui::layout_kind::dynamic_list);
  CHECK(tree.measure(stdui::proposal::unbounded()) == stdui::size{8.0, 48.0});

  auto frame = tree.arrange({{0.0, 0.0}, {100.0, 100.0}});
  REQUIRE(frame.children.size() == 3);
  CHECK(frame.children[0].bounds.origin == stdui::point{0.0, 0.0});
  CHECK(frame.children[1].bounds.origin == stdui::point{0.0, 16.0});
  CHECK(frame.children[2].bounds.origin == stdui::point{0.0, 32.0});
  CHECK(frame.bounds.extent == stdui::size{8.0, 48.0});
}

TEST_CASE("dynamic list rejects duplicate item ids during reconciliation") {
  stdui::runtime runtime;
  auto list = stdui::dynamic_list(
      std::vector<int>{1, 1}, [](int item) { return item; },
      [](int item) { return stdui::text(std::to_string(item)); });

  CHECK_THROWS_AS(runtime.reconcile(list), std::logic_error);
}

TEST_CASE("materialization rejects unknown node kinds") {
  stdui::inspection_node unknown{"button", "label", {}};

  CHECK_THROWS_AS(stdui::materialize_layout(unknown, measured_text), std::logic_error);
}

TEST_CASE("text without a measure function reports zero size") {
  auto snapshot = stdui::inspect(stdui::text("A"));
  auto tree = stdui::materialize_layout(snapshot, stdui::text_measure_fn{});

  CHECK(tree.measure(stdui::proposal::unbounded()) == stdui::size{});
}

TEST_CASE("empty containers measure to zero and arrange empty boxes") {
  auto snapshot = stdui::inspect(stdui::vstack());
  auto tree = stdui::materialize_layout(snapshot, measured_text);

  CHECK(tree.measure(stdui::proposal::unbounded()) == stdui::size{});
  auto box = tree.arrange({{0.0, 0.0}, {100.0, 100.0}});
  CHECK(box.bounds == stdui::rect{{0.0, 0.0}, {0.0, 0.0}});
  CHECK(box.children.empty());
}

TEST_CASE("stack padding applies through the materialized tree") {
  auto snapshot = stdui::inspect(stdui::vstack(stdui::text("A")));
  auto tree = stdui::materialize_layout(snapshot, measured_text);
  tree.stack.padding = {1.0, 2.0, 1.0, 2.0};

  CHECK(tree.measure(stdui::proposal::unbounded()) == stdui::size{10.0, 20.0});
  CHECK(tree.measure(stdui::proposal::bounded(50.0, 50.0)) == stdui::size{10.0, 50.0});

  auto frame = tree.arrange({{0.0, 0.0}, {100.0, 100.0}});
  REQUIRE(frame.children.size() == 1);
  CHECK(frame.children[0].bounds == stdui::rect{{1.0, 2.0}, {8.0, 16.0}});
  CHECK(frame.bounds.extent == stdui::size{9.0, 18.0});
}

TEST_CASE("flex distribution applies through the materialized tree") {
  auto snapshot = stdui::inspect(stdui::hstack(stdui::text("A"), stdui::text("B")));
  auto tree = stdui::materialize_layout(snapshot, measured_text);
  tree.children[0].policy.grow = 1.0;
  tree.children[1].policy.grow = 3.0;

  CHECK(tree.measure(stdui::proposal::bounded(100.0, 16.0)) == stdui::size{100.0, 16.0});

  auto frame = tree.arrange({{0.0, 0.0}, {100.0, 16.0}});
  REQUIRE(frame.children.size() == 2);
  CHECK(frame.children[0].bounds == stdui::rect{{0.0, 0.0}, {8.0, 16.0}});
  CHECK(frame.children[1].bounds.origin == stdui::point{29.0, 0.0});
  CHECK(frame.bounds.extent == stdui::size{37.0, 16.0});
}

TEST_CASE("DSL stack options flow into materialized measure and arrange") {
  stdui::stack_options options;
  options.spacing = 4.0;
  options.padding = {1.0, 2.0, 1.0, 2.0};

  auto snapshot = stdui::inspect(stdui::vstack(options, stdui::text("A"), stdui::text("B")));
  auto tree = stdui::materialize_layout(snapshot, measured_text);

  CHECK(tree.measure(stdui::proposal::unbounded()) == stdui::size{10.0, 40.0});

  auto frame = tree.arrange({{0.0, 0.0}, {100.0, 100.0}});
  REQUIRE(frame.children.size() == 2);
  CHECK(frame.children[0].bounds == stdui::rect{{1.0, 2.0}, {8.0, 16.0}});
  CHECK(frame.children[1].bounds.origin == stdui::point{1.0, 22.0});
  CHECK(frame.bounds.extent == stdui::size{9.0, 38.0});
}

TEST_CASE("DSL overlay alignment flows into materialized arrangement") {
  stdui::overlay_options options;
  options.alignment = stdui::layout_alignment::end;

  auto snapshot = stdui::inspect(stdui::overlay(options, stdui::text("A")));
  auto tree = stdui::materialize_layout(snapshot, measured_text);

  auto frame = tree.arrange({{0.0, 0.0}, {100.0, 100.0}});
  REQUIRE(frame.children.size() == 1);
  CHECK(frame.children[0].bounds.origin == stdui::point{92.0, 84.0});
}

TEST_CASE("DSL grow policies distribute space through the materialized tree") {
  stdui::runtime runtime;
  stdui::stack_options options;
  options.spacing = 4.0;

  auto snapshot = runtime.reconcile(stdui::hstack(options, stdui::grow(1.0, stdui::text("A")),
                                                  stdui::grow(3.0, stdui::text("B"))));
  auto tree = stdui::materialize_layout(snapshot, measured_text);

  CHECK(tree.measure(stdui::proposal::bounded(100.0, 16.0)) == stdui::size{100.0, 16.0});

  auto frame = tree.arrange({{0.0, 0.0}, {100.0, 16.0}});
  REQUIRE(frame.children.size() == 2);
  CHECK(frame.children[0].bounds == stdui::rect{{0.0, 0.0}, {8.0, 16.0}});
  CHECK(frame.children[1].bounds.origin == stdui::point{32.0, 0.0});
  CHECK(frame.bounds.extent == stdui::size{40.0, 16.0});
}

TEST_CASE("DSL fill policy consumes the remaining main-axis space") {
  stdui::runtime runtime;
  auto snapshot = runtime.reconcile(stdui::hstack(stdui::text("A"), stdui::fill(stdui::text("B"))));
  auto tree = stdui::materialize_layout(snapshot, measured_text);

  auto frame = tree.arrange({{0.0, 0.0}, {100.0, 16.0}});
  REQUIRE(frame.children.size() == 2);
  CHECK(frame.children[1].bounds.origin == stdui::point{8.0, 0.0});
  CHECK(frame.bounds.extent == stdui::size{16.0, 16.0});
}
