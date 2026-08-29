#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdui/inspection.hpp>
#include <stdui/runtime.hpp>

#include <stdexcept>

namespace {
struct counter_component {};
struct row_component {};
struct alternate_counter_component {};
struct failing_component {};

auto incrementing_counter(int initial_value) {
  return stdui::component<counter_component>([=](stdui::component_context &cx) {
    auto count = cx.use_state<int>(initial_value);
    auto current = *count;
    *count = current + 1;
    return stdui::text(std::to_string(current));
  });
}

auto incrementing_row(int initial_value) {
  return stdui::component<row_component>([=](stdui::component_context &cx) {
    auto count = cx.use_state<int>(initial_value);
    auto current = *count;
    *count = current + 1;
    return stdui::text(std::to_string(current));
  });
}

auto alternate_incrementing_counter(int initial_value) {
  return stdui::component<alternate_counter_component>([=](stdui::component_context &cx) {
    auto count = cx.use_state<int>(initial_value);
    auto current = *count;
    *count = current + 1;
    return stdui::text(std::to_string(current));
  });
}

auto failing_counter() {
  return stdui::component<failing_component>([](stdui::component_context &cx) {
    auto count = cx.use_state<int>(0);
    *count = 999;
    throw std::runtime_error("component body failed");
    return stdui::text(std::to_string(*count));
  });
}
} // namespace

TEST_CASE("headless inspection preserves composed child order") {
  auto tree = stdui::inspect(stdui::vstack(
      stdui::text("People"), stdui::hstack(stdui::text("AY"), stdui::text("Ada Yoon"))));

  REQUIRE(tree.kind == "vstack");
  REQUIRE(tree.children.size() == 2);
  CHECK(tree.children[0].kind == "text");
  CHECK(tree.children[0].content == "People");
  CHECK(tree.children[1].kind == "hstack");
  REQUIRE(tree.children[1].children.size() == 2);
  CHECK(tree.children[1].children[1].content == "Ada Yoon");
}

TEST_CASE("runtime preserves local state for unchanged component identity") {
  stdui::runtime runtime;

  auto first = runtime.reconcile(incrementing_counter(0));
  auto second = runtime.reconcile(incrementing_counter(100));

  CHECK(first.content == "0");
  CHECK(second.content == "1");
  CHECK(runtime.state_count() == 1);
}

TEST_CASE("unidentified structural position participates in component identity") {
  stdui::runtime runtime;

  auto first = runtime.reconcile(stdui::vstack(incrementing_counter(0)));
  auto shifted = runtime.reconcile(stdui::vstack(stdui::text("Header"), incrementing_counter(100)));

  REQUIRE(first.children.size() == 1);
  REQUIRE(shifted.children.size() == 2);
  CHECK(first.children[0].content == "0");
  CHECK(shifted.children[1].content == "100");
  CHECK(runtime.state_count() == 1);
}

TEST_CASE("component kind participates in component identity") {
  stdui::runtime runtime;

  auto first = runtime.reconcile(incrementing_counter(0));
  auto replaced = runtime.reconcile(alternate_incrementing_counter(100));

  CHECK(first.content == "0");
  CHECK(replaced.content == "100");
  CHECK(runtime.state_count() == 1);
}

TEST_CASE("explicit ids preserve component state across reordering") {
  stdui::runtime runtime;

  auto first = runtime.reconcile(stdui::vstack(stdui::identified("a", incrementing_row(10)),
                                               stdui::identified("b", incrementing_row(20))));
  auto reordered = runtime.reconcile(stdui::vstack(stdui::identified("b", incrementing_row(200)),
                                                   stdui::identified("a", incrementing_row(100))));

  REQUIRE(first.children.size() == 2);
  REQUIRE(reordered.children.size() == 2);
  CHECK(first.children[0].content == "10");
  CHECK(first.children[1].content == "20");
  CHECK(reordered.children[0].content == "21");
  CHECK(reordered.children[1].content == "11");
  CHECK(runtime.state_count() == 2);
}

TEST_CASE("failed reconciliation keeps the previous committed state") {
  stdui::runtime runtime;

  runtime.reconcile(incrementing_counter(0));
  CHECK_THROWS_AS(runtime.reconcile(failing_counter()), std::runtime_error);
  CHECK(runtime.state_count() == 1);

  auto next = runtime.reconcile(incrementing_counter(100));
  CHECK(next.content == "1");
  CHECK(runtime.state_count() == 1);
}

TEST_CASE("duplicate explicit ids are rejected within one sibling group") {
  stdui::runtime runtime;

  CHECK_THROWS_AS(runtime.reconcile(stdui::vstack(stdui::identified("same", incrementing_row(10)),
                                                  stdui::identified("same", incrementing_row(20)))),
                  std::logic_error);
  CHECK(runtime.state_count() == 0);
}

TEST_CASE("reconciliation removes state for components that disappear") {
  stdui::runtime runtime;

  runtime.reconcile(stdui::vstack(incrementing_counter(0)));
  CHECK(runtime.state_count() == 1);

  runtime.reconcile(stdui::vstack(stdui::text("Empty")));
  CHECK(runtime.state_count() == 0);
}
