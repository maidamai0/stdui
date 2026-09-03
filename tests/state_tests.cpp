#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdui/expressions.hpp>
#include <stdui/state.hpp>

TEST_CASE("state_storage: can allocate and retrieve values") {
  stdui::state_storage storage;

  int *slot0 = storage.get_or_create(0, 42);
  REQUIRE(slot0 != nullptr);
  CHECK(*slot0 == 42);

  int *slot0_again = storage.get_or_create(0, 100);
  CHECK(slot0_again == slot0);
  CHECK(*slot0_again == 42); // Original value preserved
}

TEST_CASE("state_storage: allocates multiple slots") {
  stdui::state_storage storage;

  int *slot0 = storage.get_or_create(0, 10);
  double *slot1 = storage.get_or_create(1, 3.14);
  std::string *slot2 = storage.get_or_create(2, std::string("hello"));

  CHECK(*slot0 == 10);
  CHECK(*slot1 == 3.14);
  CHECK(*slot2 == "hello");
  CHECK(storage.slot_count() == 3);
}

TEST_CASE("state_storage: can reset all slots") {
  stdui::state_storage storage;

  storage.get_or_create(0, 42);
  storage.get_or_create(1, 3.14);
  CHECK(storage.slot_count() == 2);

  storage.reset();
  CHECK(storage.slot_count() == 0);

  // After reset, can allocate fresh values
  int *new_slot0 = storage.get_or_create(0, 99);
  CHECK(*new_slot0 == 99);
}

TEST_CASE("component_context: creates state handles") {
  stdui::state_storage storage;
  bool changed = false;
  stdui::component_context ctx(&storage, [&] { changed = true; });

  auto counter = ctx.state(0);
  CHECK(counter.get() == 0);
  CHECK_FALSE(changed);

  counter.set(5);
  CHECK(counter.get() == 5);
  CHECK(changed);
}

TEST_CASE("component_context: state persists across re-evaluations") {
  stdui::state_storage storage;
  stdui::component_context ctx(&storage, [] {});

  // First evaluation
  auto count1 = ctx.state(0);
  count1.set(42);

  // Simulate re-evaluation by resetting slot counter
  ctx.reset_slot_counter();

  // Second evaluation - same slot
  auto count2 = ctx.state(0);
  CHECK(count2.get() == 42); // Value persisted!
}

TEST_CASE("component_context: multiple state slots") {
  stdui::state_storage storage;
  stdui::component_context ctx(&storage, [] {});

  auto count = ctx.state(0);
  auto name = ctx.state(std::string("unnamed"));
  auto enabled = ctx.state(true);

  CHECK(count.get() == 0);
  CHECK(name.get() == "unnamed");
  CHECK(enabled.get() == true);

  count.set(10);
  name.set(std::string("Alice"));
  enabled.set(false);

  CHECK(count.get() == 10);
  CHECK(name.get() == "Alice");
  CHECK(enabled.get() == false);
}

TEST_CASE("state: modify method updates in place") {
  stdui::state_storage storage;
  bool changed = false;
  stdui::component_context ctx(&storage, [&] { changed = true; });

  auto count = ctx.state(5);
  CHECK(count.get() == 5);

  changed = false;
  count.modify([](int &val) { val += 10; });

  CHECK(count.get() == 15);
  CHECK(changed);
}

TEST_CASE("state: supports complex types") {
  stdui::state_storage storage;
  stdui::component_context ctx(&storage, [] {});

  struct Data {
    int x;
    std::string name;
  };

  auto data = ctx.state(Data{42, "test"});

  CHECK(data.get().x == 42);
  CHECK(data.get().name == "test");

  data.set(Data{100, "updated"});
  CHECK(data.get().x == 100);
  CHECK(data.get().name == "updated");
}

TEST_CASE("state: dereference operator") {
  stdui::state_storage storage;
  stdui::component_context ctx(&storage, [] {});

  auto count = ctx.state(42);
  CHECK(*count == 42);
}

TEST_CASE("state: arrow operator for structs") {
  stdui::state_storage storage;
  stdui::component_context ctx(&storage, [] {});

  struct Point {
    double x;
    double y;
  };

  auto point = ctx.state(Point{10.0, 20.0});
  CHECK(point->x == 10.0);
  CHECK(point->y == 20.0);
}

TEST_CASE("state: throws when accessing uninitialized handle") {
  stdui::state<int> uninitialized;

  CHECK_THROWS_AS(uninitialized.get(), std::runtime_error);
  CHECK_THROWS_AS(uninitialized.set(42), std::runtime_error);
  CHECK_THROWS_AS(uninitialized.modify([](int &) {}), std::runtime_error);
}

TEST_CASE("component_context: throws without storage") {
  stdui::component_context ctx;

  CHECK_THROWS_AS(ctx.state(0), std::runtime_error);
}

TEST_CASE("state: change callback fires on every update") {
  stdui::state_storage storage;
  int change_count = 0;
  stdui::component_context ctx(&storage, [&] { ++change_count; });

  auto count = ctx.state(0);
  CHECK(change_count == 0);

  count.set(1);
  CHECK(change_count == 1);

  count.set(2);
  CHECK(change_count == 2);

  count.modify([](int &val) { val++; });
  CHECK(change_count == 3);
}

TEST_CASE("state: slot counter increments correctly") {
  stdui::state_storage storage;
  stdui::component_context ctx(&storage, [] {});

  CHECK(ctx.current_slot() == 0);

  auto s1 = ctx.state(1);
  CHECK(ctx.current_slot() == 1);

  auto s2 = ctx.state(2);
  CHECK(ctx.current_slot() == 2);

  auto s3 = ctx.state(3);
  CHECK(ctx.current_slot() == 3);

  ctx.reset_slot_counter();
  CHECK(ctx.current_slot() == 0);
}

TEST_CASE("integration: counter component pattern") {
  stdui::state_storage storage;
  int render_count = 0;

  auto counter_body = [&](stdui::component_context &ctx) {
    ++render_count;
    auto count = ctx.state(0);

    // Simulate button click incrementing counter
    if (render_count == 2) {
      count.set(count.get() + 1);
    }

    return stdui::text(std::to_string(count.get()));
  };

  // First render
  stdui::component_context ctx1(&storage, [] {});
  auto expr1 = counter_body(ctx1);
  CHECK(render_count == 1);

  // Simulate re-render after state change
  ctx1.reset_slot_counter();
  auto expr2 = counter_body(ctx1);
  CHECK(render_count == 2);

  // Value persisted across renders
  ctx1.reset_slot_counter();
  auto count_verify = ctx1.state(0);
  CHECK(count_verify.get() == 1);
}
