#pragma once

#include <stdui/component.hpp>
#include <stdui/inspection.hpp>
#include <stdui/layout_tree.hpp>
#include <stdui/platform.hpp>

#include <functional>
#include <memory>
#include <string>

namespace stdui {

/**
 * Application configuration.
 *
 * Specifies initial window properties and platform settings.
 */
struct app_config {
  std::string title = "stdui Application";
  size window_size = {800.0, 600.0};
  color background_color = color::white();
};

/**
 * Application runtime.
 *
 * Manages the complete lifecycle of a stdui application:
 * - Component evaluation and state management
 * - Layout materialization
 * - Rendering
 * - Event handling
 * - Reactive updates
 */
class application {
public:
  /// Creates an application with the specified root component.
  explicit application(std::shared_ptr<component_base const> root_comp, app_config cfg = {})
      : root_component_(std::move(root_comp)), config_(std::move(cfg)) {}

  /// Initializes the application and creates the window.
  void initialize(platform &platform) {
    platform_ = &platform;
    platform_->initialize();

    window_ = platform_->create_window(config_.window_size, config_.title);

    // Set up redraw callback
    window_->set_redraw_callback([this] { render(); });

    // Set up event handlers
    window_->event_dispatcher().add_handler([this](platform_event const &event) {
      return handle_event(event);
    });

    // Initial evaluation and layout
    needs_update_ = true;
    update();
  }

  /// Runs the application event loop (blocks until window closes).
  void run() {
    if (!platform_) {
      throw std::runtime_error("Application not initialized");
    }
    window_->show();
    platform_->run_event_loop();
  }

  /// Requests a re-evaluation and redraw.
  void invalidate() {
    needs_update_ = true;
    if (window_) {
      window_->request_redraw();
    }
  }

  /// Returns the component registry (for testing/debugging).
  auto registry() -> component_registry & { return registry_; }

  /// Returns the current layout tree (for testing/debugging).
  auto layout_tree() const -> layout_node const * { return layout_tree_.get(); }

private:
  /// Evaluates the component tree and materializes layout.
  void update() {
    if (!needs_update_) {
      return;
    }

    // Evaluate root component with state context
    component_evaluator evaluator(registry_);
    auto snapshot = evaluator.evaluate(*root_component_, [this] { invalidate(); });

    // Materialize layout from inspection_node
    layout_tree_ = std::make_unique<layout_node>(
        materialize_layout(snapshot, [](std::string_view text) -> size {
          // TODO: Use platform text measurer
          return {static_cast<double>(text.length()) * 8.0, 16.0};
        }));

    needs_update_ = false;
  }

  /// Renders the current layout tree.
  void render() {
    if (needs_update_) {
      update();
    }

    if (!window_ || !layout_tree_) {
      return;
    }

    auto &renderer = window_->renderer();
    renderer.begin_frame();
    renderer.clear(config_.background_color);

    // Arrange layout within window bounds
    auto bounds = rect{{0.0, 0.0}, window_->size()};
    auto arranged = layout_tree_->arrange(bounds);

    // Render the layout tree
    render_box(renderer, arranged);

    renderer.end_frame();
  }

  /// Recursively renders a layout box and its children.
  void render_box(renderer &renderer, layout_box const &box) {
    if (box.kind == "text") {
      renderer.draw_text(box.content, box.bounds.origin, font_descriptor{}, color::black());
    }

    for (auto const &child : box.children) {
      render_box(renderer, child);
    }
  }

  /// Handles platform events.
  bool handle_event(platform_event const &event) {
    // TODO: Implement event routing to components
    // For now, just consume all events
    return true;
  }

  std::shared_ptr<component_base const> root_component_;
  app_config config_;
  platform *platform_ = nullptr;
  std::unique_ptr<platform_window> window_;
  component_registry registry_;
  std::unique_ptr<layout_node> layout_tree_;
  bool needs_update_ = false;
};

/**
 * Convenience function to create and run an application.
 *
 * Example:
 *   auto root = std::make_shared<my_component>();
 *   stdui::run_app(root, stdui::get_platform());
 */
inline void run_app(std::shared_ptr<component_base const> root, platform &plat,
                    app_config config = {}) {
  application app(std::move(root), std::move(config));
  app.initialize(plat);
  app.run();
}

} // namespace stdui
