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
