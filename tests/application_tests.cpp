#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdui/application.hpp>
#include <stdui/component.hpp>
#include <stdui/platform.hpp>

// Simple test component for application tests
class hello_component : public stdui::typed_component<hello_component> {
public:
  auto body(stdui::component_context &ctx) const -> stdui::inspection_node override {
    return stdui::inspect(stdui::text("Hello, World!"));
  }
};

// Component with state for testing updates
class stateful_component : public stdui::typed_component<stateful_component> {
public:
  auto body(stdui::component_context &ctx) const -> stdui::inspection_node override {
    auto count = ctx.state(0);
    return stdui::inspect(stdui::text("Count: " + std::to_string(count.get())));
  }
};

// Component with layout
class layout_component : public stdui::typed_component<layout_component> {
public:
  auto body(stdui::component_context &ctx) const -> stdui::inspection_node override {
    return stdui::inspect(stdui::vstack(
        stdui::text("Header"),
        stdui::hstack(
            stdui::text("Left"),
            stdui::text("Right")
        ),
        stdui::text("Footer")
    ));
  }
};

TEST_CASE("app_config: default values") {
  stdui::app_config config;
  CHECK(config.title == "stdui Application");
  CHECK(config.window_size == stdui::size{800.0, 600.0});
  CHECK(config.background_color == stdui::color::white());
}

TEST_CASE("app_config: custom values") {
  stdui::app_config config{
      .title = "My App",
      .window_size = {1024.0, 768.0},
      .background_color = stdui::color::black(),
  };

  CHECK(config.title == "My App");
  CHECK(config.window_size == stdui::size{1024.0, 768.0});
  CHECK(config.background_color == stdui::color::black());
}

TEST_CASE("application: creates with root component") {
  auto root = std::make_shared<hello_component>();
  stdui::application app(root);

  // Application should exist
  CHECK(app.registry().component_count() == 0); // Not yet initialized
}

TEST_CASE("application: initializes with platform") {
  auto root = std::make_shared<hello_component>();
  stdui::application app(root);

  stdui::null_platform platform;
  app.initialize(platform);

  // After initialization, component should be evaluated
  CHECK(app.registry().component_count() >= 1);
  CHECK(app.layout_tree() != nullptr);
}

TEST_CASE("application: uses custom config") {
  auto root = std::make_shared<hello_component>();
  stdui::app_config config{
      .title = "Test App",
      .window_size = {640.0, 480.0},
  };

  stdui::application app(root, config);
  stdui::null_platform platform;
  app.initialize(platform);

  CHECK(app.layout_tree() != nullptr);
}

TEST_CASE("application: evaluates component tree") {
  auto root = std::make_shared<hello_component>();
  stdui::application app(root);

  stdui::null_platform platform;
  app.initialize(platform);

  auto *tree = app.layout_tree();
  REQUIRE(tree != nullptr);

  // The layout tree should have been materialized from the inspection_node
  CHECK(tree->kind == stdui::layout_kind::text);
}

TEST_CASE("application: evaluates stateful component") {
  auto root = std::make_shared<stateful_component>();
  stdui::application app(root);

  stdui::null_platform platform;
  app.initialize(platform);

  CHECK(app.registry().component_count() >= 1);
  CHECK(app.layout_tree() != nullptr);
}

TEST_CASE("application: evaluates layout component") {
  auto root = std::make_shared<layout_component>();
  stdui::application app(root);

  stdui::null_platform platform;
  app.initialize(platform);

  auto *tree = app.layout_tree();
  REQUIRE(tree != nullptr);

  CHECK(tree->kind == stdui::layout_kind::vstack);
}

TEST_CASE("application: invalidate marks for update") {
  auto root = std::make_shared<hello_component>();
  stdui::application app(root);

  stdui::null_platform platform;
  app.initialize(platform);

  // Invalidate should mark for update
  app.invalidate();

  // Can't directly test needs_update_ flag, but we can verify
  // the application doesn't crash and maintains its state
  CHECK(app.layout_tree() != nullptr);
}

TEST_CASE("application: multiple components share registry") {
  auto root = std::make_shared<stateful_component>();
  stdui::application app(root);

  stdui::null_platform platform;
  app.initialize(platform);

  auto &registry = app.registry();
  auto initial_count = registry.component_count();

  // Registry should have at least the root component
  CHECK(initial_count >= 1);
}
