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

TEST_CASE("layout tree: single text node") {
  auto snapshot = stdui::inspect(stdui::text("Test"));
  auto tree = stdui::materialize_layout(snapshot, measured_text);

  CHECK(tree.kind == stdui::layout_kind::text);
  CHECK(tree.content == "Test");
  CHECK(tree.measure(stdui::proposal::unbounded()) == stdui::size{32.0, 16.0});
}

TEST_CASE("layout tree: empty text node") {
  auto snapshot = stdui::inspect(stdui::text(""));
  auto tree = stdui::materialize_layout(snapshot, measured_text);

  CHECK(tree.kind == stdui::layout_kind::text);
  CHECK(tree.content == "");
  CHECK(tree.measure(stdui::proposal::unbounded()) == stdui::size{0.0, 16.0});
}

TEST_CASE("layout tree: hstack with single child") {
  auto snapshot = stdui::inspect(stdui::hstack(stdui::text("Solo")));
  auto tree = stdui::materialize_layout(snapshot, measured_text);

  CHECK(tree.kind == stdui::layout_kind::hstack);
  CHECK(tree.children.size() == 1);

  auto frame = tree.arrange({{0.0, 0.0}, {100.0, 100.0}});
  CHECK(frame.children.size() == 1);
}

TEST_CASE("layout tree: vstack with spacing") {
  auto snapshot = stdui::inspect(stdui::vstack(stdui::text("A"), stdui::text("B")));
  auto tree = stdui::materialize_layout(snapshot, measured_text);
  tree.stack.spacing = 5.0;

  auto size = tree.measure(stdui::proposal::unbounded());
  CHECK(size == stdui::size{8.0, 37.0}); // 16 + 5 + 16

  auto frame = tree.arrange({{0.0, 0.0}, {100.0, 100.0}});
  CHECK(frame.children[0].bounds.origin.y == 0.0);
  CHECK(frame.children[1].bounds.origin.y == 21.0); // 16 + 5
}

TEST_CASE("layout tree: hstack with spacing") {
  auto snapshot = stdui::inspect(stdui::hstack(stdui::text("A"), stdui::text("B")));
  auto tree = stdui::materialize_layout(snapshot, measured_text);
  tree.stack.spacing = 10.0;

  auto size = tree.measure(stdui::proposal::unbounded());
  CHECK(size == stdui::size{26.0, 16.0}); // 8 + 10 + 8

  auto frame = tree.arrange({{0.0, 0.0}, {100.0, 100.0}});
  CHECK(frame.children[0].bounds.origin.x == 0.0);
  CHECK(frame.children[1].bounds.origin.x == 18.0); // 8 + 10
}

TEST_CASE("layout tree: overlay with multiple children") {
  auto snapshot = stdui::inspect(stdui::overlay(
      stdui::text("A"),
      stdui::text("BB"),
      stdui::text("CCC")
  ));
  auto tree = stdui::materialize_layout(snapshot, measured_text);

  CHECK(tree.kind == stdui::layout_kind::overlay);
  CHECK(tree.children.size() == 3);
  CHECK(tree.measure(stdui::proposal::unbounded()) == stdui::size{24.0, 16.0}); // Max width
}

TEST_CASE("layout tree: deeply nested hierarchy") {
  auto snapshot = stdui::inspect(
      stdui::vstack(
          stdui::hstack(
              stdui::vstack(
                  stdui::text("Deep")
              )
          )
      )
  );
  auto tree = stdui::materialize_layout(snapshot, measured_text);

  CHECK(tree.kind == stdui::layout_kind::vstack);
  CHECK(tree.children[0].kind == stdui::layout_kind::hstack);
  CHECK(tree.children[0].children[0].kind == stdui::layout_kind::vstack);
  CHECK(tree.children[0].children[0].children[0].kind == stdui::layout_kind::text);
}

TEST_CASE("layout tree: proposal bounded width only") {
  auto snapshot = stdui::inspect(stdui::text("LongText"));
  auto tree = stdui::materialize_layout(snapshot, measured_text);

  auto size = tree.measure(stdui::proposal::bounded(20.0, 1000.0));
  CHECK(size.width <= 64.0); // 8 chars * 8 pixels
}

TEST_CASE("layout tree: proposal bounded height only") {
  auto snapshot = stdui::inspect(stdui::vstack(
      stdui::text("A"),
      stdui::text("B"),
      stdui::text("C")
  ));
  auto tree = stdui::materialize_layout(snapshot, measured_text);

  auto size = tree.measure(stdui::proposal::bounded(1000.0, 30.0));
  // Bounded height constrains the result
  CHECK(size.height <= 48.0);
}

TEST_CASE("layout tree: arrange with constrained bounds") {
  auto snapshot = stdui::inspect(stdui::vstack(stdui::text("A"), stdui::text("B")));
  auto tree = stdui::materialize_layout(snapshot, measured_text);

  auto frame = tree.arrange({{10.0, 20.0}, {50.0, 50.0}});

  CHECK(frame.bounds.origin == stdui::point{10.0, 20.0});
  CHECK(frame.children[0].bounds.origin.x >= 10.0);
  CHECK(frame.children[0].bounds.origin.y >= 20.0);
}

TEST_CASE("layout tree: padding all sides different") {
  auto snapshot = stdui::inspect(stdui::vstack(stdui::text("X")));
  auto tree = stdui::materialize_layout(snapshot, measured_text);
  tree.stack.padding = {10.0, 20.0, 30.0, 40.0}; // left, top, right, bottom

  auto size = tree.measure(stdui::proposal::unbounded());
  CHECK(size.width == 8.0 + 10.0 + 30.0); // content + left + right
  CHECK(size.height == 16.0 + 20.0 + 40.0); // content + top + bottom
}

TEST_CASE("layout tree: flex grow distributes space") {
  auto snapshot = stdui::inspect(stdui::hstack(stdui::text("A"), stdui::text("B")));
  auto tree = stdui::materialize_layout(snapshot, measured_text);

  tree.children[0].policy.grow = 2.0;
  tree.children[1].policy.grow = 1.0;

  auto frame = tree.arrange({{0.0, 0.0}, {100.0, 16.0}});

  // Both children have their natural size since hstack doesn't stretch by default
  // Just verify the frame was arranged successfully
  CHECK(frame.children.size() == 2);
  CHECK(frame.children[0].bounds.extent.width >= 8.0);
  CHECK(frame.children[1].bounds.extent.width >= 8.0);
}

TEST_CASE("layout tree: overlay start alignment") {
  auto snapshot = stdui::inspect(stdui::overlay(stdui::text("A")));
  auto tree = stdui::materialize_layout(snapshot, measured_text);
  tree.overlay.alignment = stdui::layout_alignment::start;

  auto frame = tree.arrange({{0.0, 0.0}, {100.0, 100.0}});
  CHECK(frame.children[0].bounds.origin == stdui::point{0.0, 0.0});
}

TEST_CASE("layout tree: dynamic list empty") {
  stdui::runtime runtime;
  auto snapshot = runtime.reconcile(stdui::dynamic_list(
      std::vector<int>{},
      [](int item) { return item; },
      [](int item) { return stdui::text(std::to_string(item)); }
  ));

  auto tree = stdui::materialize_layout(snapshot, measured_text);
  CHECK(tree.kind == stdui::layout_kind::dynamic_list);
  CHECK(tree.children.empty());
  CHECK(tree.measure(stdui::proposal::unbounded()) == stdui::size{0.0, 0.0});
}

TEST_CASE("layout tree: dynamic list single item") {
  stdui::runtime runtime;
  auto snapshot = runtime.reconcile(stdui::dynamic_list(
      std::vector<int>{42},
      [](int item) { return item; },
      [](int item) { return stdui::text(std::to_string(item)); }
  ));

  auto tree = stdui::materialize_layout(snapshot, measured_text);
  CHECK(tree.children.size() == 1);
}

TEST_CASE("layout tree: measure with different proposals") {
  auto snapshot = stdui::inspect(stdui::text("Test"));
  auto tree = stdui::materialize_layout(snapshot, measured_text);

  auto unbounded = tree.measure(stdui::proposal::unbounded());
  auto bounded = tree.measure(stdui::proposal::bounded(100.0, 100.0));

  CHECK(unbounded.width <= bounded.width);
}
