#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdui/component.hpp>

// Simple test component
class counter_component : public stdui::typed_component<counter_component> {
public:
  auto body(stdui::component_context &ctx) const -> stdui::inspection_node override {
    auto count = ctx.state(0);
    return stdui::inspect(stdui::text("Count: " + std::to_string(count.get())));
  }
};

// Component with multiple state slots
class multi_state_component : public stdui::typed_component<multi_state_component> {
public:
  auto body(stdui::component_context &ctx) const -> stdui::inspection_node override {
    auto name = ctx.state<std::string>("Alice");
    auto age = ctx.state(25);
    return stdui::inspect(stdui::text(name.get() + " is " + std::to_string(age.get())));
  }
};

// Component that returns a container
class container_component : public stdui::typed_component<container_component> {
public:
  auto body(stdui::component_context &ctx) const -> stdui::inspection_node override {
    return stdui::inspect(stdui::vstack(
        stdui::text("Header"),
        stdui::text("Body"),
        stdui::text("Footer")
    ));
  }
};

TEST_CASE("component_id: equality and hashing") {
  stdui::component_id id1{typeid(counter_component), 1};
  stdui::component_id id2{typeid(counter_component), 1};
  stdui::component_id id3{typeid(counter_component), 2};

  CHECK(id1 == id2);
  CHECK_FALSE(id1 == id3);

  std::hash<stdui::component_id> hasher;
  CHECK(hasher(id1) == hasher(id2));
}

TEST_CASE("component_registry: creates and retrieves storage") {
  stdui::component_registry registry;
  stdui::component_id id{typeid(counter_component), 1};

  auto &storage1 = registry.get_or_create_storage(id);
  auto &storage2 = registry.get_or_create_storage(id);

  CHECK(&storage1 == &storage2); // Same instance
  CHECK(registry.component_count() == 1);
}

TEST_CASE("component_registry: removes storage") {
  stdui::component_registry registry;
  stdui::component_id id{typeid(counter_component), 1};

  registry.get_or_create_storage(id);
  CHECK(registry.component_count() == 1);

  registry.remove_storage(id);
  CHECK(registry.component_count() == 0);
}

TEST_CASE("component_registry: multiple components") {
  stdui::component_registry registry;
  stdui::component_id id1{typeid(counter_component), 1};
  stdui::component_id id2{typeid(counter_component), 2};

  registry.get_or_create_storage(id1);
  registry.get_or_create_storage(id2);

  CHECK(registry.component_count() == 2);

  registry.clear();
  CHECK(registry.component_count() == 0);
}

TEST_CASE("component_evaluator: evaluates simple component") {
  stdui::component_registry registry;
  stdui::component_evaluator evaluator(registry);

  counter_component comp;

  bool change_called = false;
  auto snapshot = evaluator.evaluate(comp, [&] { change_called = true; });

  // Should return an inspection_node
  CHECK(snapshot.kind == "text");
  CHECK(snapshot.content == "Count: 0");
  CHECK(registry.component_count() == 1);
}

TEST_CASE("component_evaluator: state persists across evaluations") {
  stdui::component_registry registry;
  stdui::component_evaluator evaluator(registry);

  multi_state_component comp;

  // First evaluation - creates state with defaults
  auto snapshot1 = evaluator.evaluate(comp, [] {});
  CHECK(snapshot1.kind == "text");
  CHECK(snapshot1.content == "Alice is 25");

  // Modify state directly via storage
  stdui::component_id id{comp.type_id(), comp.instance_id()};
  auto &storage = registry.get_or_create_storage(id);
  auto name_ptr = storage.get_or_create<std::string>(0, "Alice");
  *name_ptr = "Bob";
  auto age_ptr = storage.get_or_create<int>(1, 25);
  *age_ptr = 30;

  // Second evaluation - should see updated state
  auto snapshot2 = evaluator.evaluate(comp, [] {});
  CHECK(snapshot2.kind == "text");
  CHECK(snapshot2.content == "Bob is 30");
}

TEST_CASE("component_evaluator: different instances have separate state") {
  stdui::component_registry registry;
  stdui::component_evaluator evaluator(registry);

  // Two instances with different addresses
  auto comp1 = std::make_unique<counter_component>();
  auto comp2 = std::make_unique<counter_component>();

  auto snapshot1 = evaluator.evaluate(*comp1, [] {});
  auto snapshot2 = evaluator.evaluate(*comp2, [] {});

  CHECK(snapshot1.content == "Count: 0");
  CHECK(snapshot2.content == "Count: 0");
  CHECK(registry.component_count() == 2); // Separate state
}

TEST_CASE("component_evaluator: container component") {
  stdui::component_registry registry;
  stdui::component_evaluator evaluator(registry);

  container_component comp;

  auto snapshot = evaluator.evaluate(comp, [] {});

  CHECK(snapshot.kind == "vstack");
  CHECK(snapshot.children.size() == 3);
  CHECK(snapshot.children[0].kind == "text");
  CHECK(snapshot.children[0].content == "Header");
  CHECK(snapshot.children[1].kind == "text");
  CHECK(snapshot.children[1].content == "Body");
  CHECK(snapshot.children[2].kind == "text");
  CHECK(snapshot.children[2].content == "Footer");
}

TEST_CASE("typed_component: provides automatic type_id") {
  counter_component comp1;
  multi_state_component comp2;

  CHECK(comp1.type_id() == typeid(counter_component));
  CHECK(comp2.type_id() == typeid(multi_state_component));
  CHECK(comp1.type_id() != comp2.type_id());
}

TEST_CASE("component_base: default instance_id based on address") {
  counter_component comp1;
  counter_component comp2;

  CHECK(comp1.instance_id() == reinterpret_cast<std::size_t>(&comp1));
  CHECK(comp2.instance_id() == reinterpret_cast<std::size_t>(&comp2));
  CHECK(comp1.instance_id() != comp2.instance_id());
}

TEST_CASE("component_id: different types have different hashes") {
  stdui::component_id id1{typeid(counter_component), 1};
  stdui::component_id id2{typeid(multi_state_component), 1};

  std::hash<stdui::component_id> hasher;
  CHECK(hasher(id1) != hasher(id2)); // Different types
}

TEST_CASE("component_id: same instance_id different types") {
  stdui::component_id id1{typeid(counter_component), 100};
  stdui::component_id id2{typeid(multi_state_component), 100};

  CHECK_FALSE(id1 == id2); // Different despite same instance_id
}

TEST_CASE("component_registry: remove non-existent id is safe") {
  stdui::component_registry registry;
  stdui::component_id id{typeid(counter_component), 999};

  // Should not crash
  registry.remove_storage(id);
  CHECK(registry.component_count() == 0);
}

TEST_CASE("component_registry: clear with multiple types") {
  stdui::component_registry registry;

  stdui::component_id id1{typeid(counter_component), 1};
  stdui::component_id id2{typeid(multi_state_component), 1};
  stdui::component_id id3{typeid(container_component), 1};

  registry.get_or_create_storage(id1);
  registry.get_or_create_storage(id2);
  registry.get_or_create_storage(id3);

  CHECK(registry.component_count() == 3);

  registry.clear();
  CHECK(registry.component_count() == 0);
}

TEST_CASE("component_registry: storage independence") {
  stdui::component_registry registry;

  stdui::component_id id1{typeid(counter_component), 1};
  stdui::component_id id2{typeid(counter_component), 2};

  auto &storage1 = registry.get_or_create_storage(id1);
  auto &storage2 = registry.get_or_create_storage(id2);

  // Modify storage1
  storage1.get_or_create(0, 42);

  // storage2 should be independent
  auto *val = storage2.get_or_create(0, 99);
  CHECK(*val == 99);

  // storage1 unchanged
  auto *val1 = storage1.get_or_create(0, 0);
  CHECK(*val1 == 42);
}

TEST_CASE("component_evaluator: on_change callback fires") {
  stdui::component_registry registry;
  stdui::component_evaluator evaluator(registry);

  // Component that modifies state
  class modifying_component : public stdui::typed_component<modifying_component> {
  public:
    mutable bool should_modify = false;

    auto body(stdui::component_context &ctx) const -> stdui::inspection_node override {
      auto count = ctx.state(0);
      if (should_modify) {
        count.set(count.get() + 1);
      }
      return stdui::inspect(stdui::text("Count: " + std::to_string(count.get())));
    }
  };

  modifying_component comp;
  bool change_called = false;

  // First evaluation - no change
  evaluator.evaluate(comp, [&] { change_called = true; });
  CHECK_FALSE(change_called);

  // Second evaluation - with change
  change_called = false;
  comp.should_modify = true;
  evaluator.evaluate(comp, [&] { change_called = true; });
  CHECK(change_called);
}

TEST_CASE("component_evaluator: multiple evaluations same component") {
  stdui::component_registry registry;
  stdui::component_evaluator evaluator(registry);

  counter_component comp;

  auto snapshot1 = evaluator.evaluate(comp, [] {});
  auto snapshot2 = evaluator.evaluate(comp, [] {});
  auto snapshot3 = evaluator.evaluate(comp, [] {});

  CHECK(snapshot1.content == "Count: 0");
  CHECK(snapshot2.content == "Count: 0");
  CHECK(snapshot3.content == "Count: 0");
  CHECK(registry.component_count() == 1); // Same component
}

TEST_CASE("component_evaluator: different component types") {
  stdui::component_registry registry;
  stdui::component_evaluator evaluator(registry);

  counter_component comp1;
  multi_state_component comp2;
  container_component comp3;

  evaluator.evaluate(comp1, [] {});
  evaluator.evaluate(comp2, [] {});
  evaluator.evaluate(comp3, [] {});

  CHECK(registry.component_count() == 3); // Three different types
}

TEST_CASE("typed_component: same type different instances") {
  counter_component comp1;
  counter_component comp2;
  counter_component comp3;

  // All have same type_id
  CHECK(comp1.type_id() == comp2.type_id());
  CHECK(comp2.type_id() == comp3.type_id());

  // But different instance_ids
  CHECK(comp1.instance_id() != comp2.instance_id());
  CHECK(comp2.instance_id() != comp3.instance_id());
  CHECK(comp1.instance_id() != comp3.instance_id());
}

TEST_CASE("component: hstack container") {
  class hstack_component : public stdui::typed_component<hstack_component> {
  public:
    auto body(stdui::component_context &ctx) const -> stdui::inspection_node override {
      return stdui::inspect(stdui::hstack(
          stdui::text("Left"),
          stdui::text("Center"),
          stdui::text("Right")
      ));
    }
  };

  stdui::component_registry registry;
  stdui::component_evaluator evaluator(registry);

  hstack_component comp;
  auto snapshot = evaluator.evaluate(comp, [] {});

  CHECK(snapshot.kind == "hstack");
  CHECK(snapshot.children.size() == 3);
}

TEST_CASE("component: overlay container") {
  class overlay_component : public stdui::typed_component<overlay_component> {
  public:
    auto body(stdui::component_context &ctx) const -> stdui::inspection_node override {
      return stdui::inspect(stdui::overlay(
          stdui::text("Background"),
          stdui::text("Foreground")
      ));
    }
  };

  stdui::component_registry registry;
  stdui::component_evaluator evaluator(registry);

  overlay_component comp;
  auto snapshot = evaluator.evaluate(comp, [] {});

  CHECK(snapshot.kind == "overlay");
  CHECK(snapshot.children.size() == 2);
}

TEST_CASE("component: with no state") {
  class stateless_component : public stdui::typed_component<stateless_component> {
  public:
    auto body(stdui::component_context &ctx) const -> stdui::inspection_node override {
      return stdui::inspect(stdui::text("Static content"));
    }
  };

  stdui::component_registry registry;
  stdui::component_evaluator evaluator(registry);

  stateless_component comp;
  auto snapshot = evaluator.evaluate(comp, [] {});

  CHECK(snapshot.kind == "text");
  CHECK(snapshot.content == "Static content");

  // Component registered even without state
  CHECK(registry.component_count() == 1);
}

TEST_CASE("component: state modification across multiple evaluations") {
  stdui::component_registry registry;
  stdui::component_evaluator evaluator(registry);

  counter_component comp;

  // Get initial state
  auto snapshot1 = evaluator.evaluate(comp, [] {});
  CHECK(snapshot1.content == "Count: 0");

  // Modify state externally
  stdui::component_id id{comp.type_id(), comp.instance_id()};
  auto &storage = registry.get_or_create_storage(id);
  auto *count_ptr = storage.get_or_create(0, 0);
  *count_ptr = 5;

  // Re-evaluate and see change
  auto snapshot2 = evaluator.evaluate(comp, [] {});
  CHECK(snapshot2.content == "Count: 5");

  // Modify again
  *count_ptr = 10;
  auto snapshot3 = evaluator.evaluate(comp, [] {});
  CHECK(snapshot3.content == "Count: 10");
}

TEST_CASE("component_id: hash consistency") {
  stdui::component_id id{typeid(counter_component), 42};

  std::hash<stdui::component_id> hasher;
  auto hash1 = hasher(id);
  auto hash2 = hasher(id);

  CHECK(hash1 == hash2); // Hash is consistent
}

TEST_CASE("component_registry: get_or_create is idempotent") {
  stdui::component_registry registry;
  stdui::component_id id{typeid(counter_component), 1};

  auto &storage1 = registry.get_or_create_storage(id);
  auto &storage2 = registry.get_or_create_storage(id);
  auto &storage3 = registry.get_or_create_storage(id);

  CHECK(&storage1 == &storage2);
  CHECK(&storage2 == &storage3);
  CHECK(registry.component_count() == 1);
}
