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

TEST_CASE("app_config: various background colors") {
  stdui::app_config config1{.background_color = stdui::color{1.0, 0.0, 0.0, 1.0}};
  CHECK(config1.background_color.red == 1.0);

  stdui::app_config config2{.background_color = stdui::color{0.5, 0.5, 0.5, 0.8}};
  CHECK(config2.background_color.alpha == 0.8);
}

TEST_CASE("app_config: various window sizes") {
  stdui::app_config small{.window_size = {320.0, 240.0}};
  CHECK(small.window_size.width == 320.0);
  CHECK(small.window_size.height == 240.0);

  stdui::app_config large{.window_size = {1920.0, 1080.0}};
  CHECK(large.window_size.width == 1920.0);
  CHECK(large.window_size.height == 1080.0);
}

TEST_CASE("application: multiple invalidations") {
  auto root = std::make_shared<hello_component>();
  stdui::application app(root);

  stdui::null_platform platform;
  app.initialize(platform);

  app.invalidate();
  app.invalidate();
  app.invalidate();

  // Should handle multiple invalidations gracefully
  CHECK(app.layout_tree() != nullptr);
}

TEST_CASE("application: registry persists across invalidations") {
  auto root = std::make_shared<stateful_component>();
  stdui::application app(root);

  stdui::null_platform platform;
  app.initialize(platform);

  auto initial_count = app.registry().component_count();

  app.invalidate();

  // Registry should still have same count
  CHECK(app.registry().component_count() == initial_count);
}

TEST_CASE("application: layout tree structure for nested components") {
  auto root = std::make_shared<layout_component>();
  stdui::application app(root);

  stdui::null_platform platform;
  app.initialize(platform);

  auto *tree = app.layout_tree();
  REQUIRE(tree != nullptr);

  CHECK(tree->kind == stdui::layout_kind::vstack);
  CHECK(tree->children.size() == 3); // Header, hstack, footer
}

TEST_CASE("application: with empty title") {
  auto root = std::make_shared<hello_component>();
  stdui::app_config config{.title = ""};

  stdui::application app(root, config);
  stdui::null_platform platform;
  app.initialize(platform);

  CHECK(app.layout_tree() != nullptr);
}

TEST_CASE("application: with very long title") {
  auto root = std::make_shared<hello_component>();
  stdui::app_config config{.title = std::string(1000, 'X')};

  stdui::application app(root, config);
  stdui::null_platform platform;
  app.initialize(platform);

  CHECK(app.layout_tree() != nullptr);
}

TEST_CASE("application: with zero window size") {
  auto root = std::make_shared<hello_component>();
  stdui::app_config config{.window_size = {0.0, 0.0}};

  stdui::application app(root, config);
  stdui::null_platform platform;
  app.initialize(platform);

  CHECK(app.layout_tree() != nullptr);
}

TEST_CASE("application: registry access before initialization") {
  auto root = std::make_shared<hello_component>();
  stdui::application app(root);

  // Registry should be accessible even before initialization
  auto &registry = app.registry();
  CHECK(registry.component_count() == 0);
}

TEST_CASE("application: layout_tree is null before initialization") {
  auto root = std::make_shared<hello_component>();
  stdui::application app(root);

  CHECK(app.layout_tree() == nullptr);
}

TEST_CASE("application: different component types") {
  class custom_component : public stdui::typed_component<custom_component> {
  public:
    auto body(stdui::component_context &ctx) const -> stdui::inspection_node override {
      return stdui::inspect(stdui::text("Custom"));
    }
  };

  auto root = std::make_shared<custom_component>();
  stdui::application app(root);

  stdui::null_platform platform;
  app.initialize(platform);

  CHECK(app.registry().component_count() >= 1);
  CHECK(app.layout_tree() != nullptr);
}

TEST_CASE("application: overlay component") {
  class overlay_component : public stdui::typed_component<overlay_component> {
  public:
    auto body(stdui::component_context &ctx) const -> stdui::inspection_node override {
      return stdui::inspect(stdui::overlay(
          stdui::text("Back"),
          stdui::text("Front")
      ));
    }
  };

  auto root = std::make_shared<overlay_component>();
  stdui::application app(root);

  stdui::null_platform platform;
  app.initialize(platform);

  auto *tree = app.layout_tree();
  REQUIRE(tree != nullptr);
  CHECK(tree->kind == stdui::layout_kind::overlay);
}

TEST_CASE("application: component with multiple state slots") {
  class multi_slot_component : public stdui::typed_component<multi_slot_component> {
  public:
    auto body(stdui::component_context &ctx) const -> stdui::inspection_node override {
      auto s1 = ctx.state(1);
      auto s2 = ctx.state(2);
      auto s3 = ctx.state(3);
      return stdui::inspect(stdui::text("Multi"));
    }
  };

  auto root = std::make_shared<multi_slot_component>();
  stdui::application app(root);

  stdui::null_platform platform;
  app.initialize(platform);

  CHECK(app.registry().component_count() >= 1);
}

TEST_CASE("app_config: copy construction") {
  stdui::app_config config1{
      .title = "Original",
      .window_size = {800.0, 600.0},
      .background_color = stdui::color::black(),
  };

  stdui::app_config config2 = config1;

  CHECK(config2.title == "Original");
  CHECK(config2.window_size == stdui::size{800.0, 600.0});
  CHECK(config2.background_color == stdui::color::black());
}

TEST_CASE("application: stateless component") {
  class stateless : public stdui::typed_component<stateless> {
  public:
    auto body(stdui::component_context &ctx) const -> stdui::inspection_node override {
      // No state calls
      return stdui::inspect(stdui::text("Stateless"));
    }
  };

  auto root = std::make_shared<stateless>();
  stdui::application app(root);

  stdui::null_platform platform;
  app.initialize(platform);

  CHECK(app.registry().component_count() >= 1);
  CHECK(app.layout_tree() != nullptr);
}
