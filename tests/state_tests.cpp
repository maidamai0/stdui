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

TEST_CASE("state_storage: non-sequential slot allocation") {
  stdui::state_storage storage;

  // Allocate slot 5 first (skipping 0-4)
  int *slot5 = storage.get_or_create(5, 100);
  CHECK(*slot5 == 100);
  CHECK(storage.slot_count() == 6); // Slots 0-5 allocated

  // Now allocate slot 2
  int *slot2 = storage.get_or_create(2, 42);
  CHECK(*slot2 == 42);
  CHECK(storage.slot_count() == 6); // No change

  // Verify slot 5 still intact
  int *slot5_again = storage.get_or_create(5, 999);
  CHECK(*slot5_again == 100); // Original value
}

TEST_CASE("state_storage: same slot different types returns nullptr") {
  stdui::state_storage storage;

  int *slot0_int = storage.get_or_create(0, 42);
  REQUIRE(slot0_int != nullptr);
  CHECK(*slot0_int == 42);

  // Try to get the same slot as a different type
  double *slot0_double = storage.get_or_create(0, 3.14);
  CHECK(slot0_double == nullptr); // Type mismatch
}

TEST_CASE("state_storage: slot count after partial allocation") {
  stdui::state_storage storage;

  storage.get_or_create(0, 1);
  CHECK(storage.slot_count() == 1);

  storage.get_or_create(2, 3); // Skip slot 1
  CHECK(storage.slot_count() == 3);

  storage.get_or_create(10, 11); // Skip slots 3-9
  CHECK(storage.slot_count() == 11);
}

TEST_CASE("state: dereference operator with const") {
  stdui::state_storage storage;
  stdui::component_context ctx(&storage, [] {});

  auto const count = ctx.state(42);
  CHECK(*count == 42);
}

TEST_CASE("state: modify with complex lambda") {
  stdui::state_storage storage;
  int change_count = 0;
  stdui::component_context ctx(&storage, [&] { ++change_count; });

  struct Counter {
    int value;
    std::string label;
  };

  auto counter = ctx.state(Counter{0, "count"});

  counter.modify([](Counter &c) {
    c.value += 10;
    c.label = "updated";
  });

  CHECK(counter.get().value == 10);
  CHECK(counter.get().label == "updated");
  CHECK(change_count == 1);
}

TEST_CASE("state: modify with mutable lambda") {
  stdui::state_storage storage;
  stdui::component_context ctx(&storage, [] {});

  auto count = ctx.state(0);

  int increment = 5;
  count.modify([increment](int &val) mutable {
    val += increment;
    increment = 10; // Mutate captured variable
  });

  CHECK(count.get() == 5);
}

TEST_CASE("state: multiple modifications in sequence") {
  stdui::state_storage storage;
  int change_count = 0;
  stdui::component_context ctx(&storage, [&] { ++change_count; });

  auto count = ctx.state(0);

  count.modify([](int &val) { val += 1; });
  count.modify([](int &val) { val *= 2; });
  count.modify([](int &val) { val -= 3; });

  CHECK(count.get() == -1); // (0 + 1) * 2 - 3 = -1
  CHECK(change_count == 3);
}

TEST_CASE("state: arrow operator with nested structs") {
  stdui::state_storage storage;
  stdui::component_context ctx(&storage, [] {});

  struct Inner {
    int value;
  };

  struct Outer {
    Inner inner;
    std::string name;
  };

  auto outer = ctx.state(Outer{{42}, "test"});
  CHECK(outer->inner.value == 42);
  CHECK(outer->name == "test");
}

TEST_CASE("state: get returns const reference") {
  stdui::state_storage storage;
  stdui::component_context ctx(&storage, [] {});

  auto count = ctx.state(42);
  auto const &ref = count.get();

  CHECK(ref == 42);
  CHECK(&ref == &count.get()); // Same address
}

TEST_CASE("state: set with move semantics") {
  stdui::state_storage storage;
  stdui::component_context ctx(&storage, [] {});

  auto str = ctx.state(std::string("initial"));

  std::string moved_str = "moved";
  str.set(std::move(moved_str));

  CHECK(str.get() == "moved");
  CHECK(moved_str.empty()); // Was moved from
}

TEST_CASE("state: default constructed handle is invalid") {
  stdui::state<int> handle;

  CHECK_THROWS_WITH(handle.get(), "state: accessing uninitialized state handle");
  CHECK_THROWS_WITH(*handle, "state: accessing uninitialized state handle");
}

TEST_CASE("component_context: default constructed is invalid") {
  stdui::component_context ctx;

  CHECK(ctx.current_slot() == 0);
  CHECK_THROWS_WITH(ctx.state(0), "component_context: no storage available");
}

TEST_CASE("component_context: reset preserves storage") {
  stdui::state_storage storage;
  stdui::component_context ctx(&storage, [] {});

  auto s1 = ctx.state(1);
  auto s2 = ctx.state(2);
  CHECK(ctx.current_slot() == 2);

  ctx.reset_slot_counter();
  CHECK(ctx.current_slot() == 0);

  // Storage still accessible
  auto s1_again = ctx.state(999); // Gets existing value
  CHECK(s1_again.get() == 1); // Original value preserved
}

TEST_CASE("state_storage: handles move-only types") {
  stdui::state_storage storage;

  auto ptr = storage.get_or_create(0, std::make_unique<int>(42));
  REQUIRE(ptr != nullptr);
  CHECK(**ptr == 42);
}

TEST_CASE("state: works with vector types") {
  stdui::state_storage storage;
  stdui::component_context ctx(&storage, [] {});

  auto vec = ctx.state(std::vector<int>{1, 2, 3});
  CHECK(vec.get().size() == 3);
  CHECK(vec.get()[0] == 1);

  vec.modify([](std::vector<int> &v) {
    v.push_back(4);
  });

  CHECK(vec.get().size() == 4);
  CHECK(vec.get()[3] == 4);
}

TEST_CASE("state: no change callback is valid") {
  stdui::state_storage storage;
  stdui::component_context ctx(&storage, nullptr);

  auto count = ctx.state(0);
  count.set(5); // Should not crash with null callback
  CHECK(count.get() == 5);
}

TEST_CASE("component_context: empty on_change callback") {
  stdui::state_storage storage;
  stdui::component_context ctx(&storage, std::function<void()>{});

  auto count = ctx.state(0);
  count.set(5); // Should not crash with empty callback
  CHECK(count.get() == 5);
}
