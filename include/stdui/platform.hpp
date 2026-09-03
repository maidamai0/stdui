#pragma once

#include <stdui/events.hpp>
#include <stdui/geometry.hpp>
#include <stdui/rendering.hpp>
#include <stdui/text_measurement.hpp>

#include <functional>
#include <memory>
#include <string>

namespace stdui {

/**
 * Platform window abstraction.
 *
 * Represents a native window with rendering and event handling capabilities.
 * Platform backends implement this to wrap native window APIs:
 * - macOS: NSWindow
 * - Linux: X11 Window or Wayland surface
 * - Windows: HWND
 */
class platform_window {
public:
  virtual ~platform_window() = default;

  /// Shows the window.
  virtual void show() = 0;

  /// Hides the window.
  virtual void hide() = 0;

  /// Returns the current window size.
  virtual auto size() const -> stdui::size = 0;

  /// Sets the window size.
  virtual void set_size(stdui::size new_size) = 0;

  /// Returns the window title.
  virtual auto title() const -> std::string = 0;

  /// Sets the window title.
  virtual void set_title(std::string const &title) = 0;

  /// Returns the renderer for this window.
  virtual auto renderer() -> stdui::renderer & = 0;

  /// Returns the event dispatcher for this window.
  virtual auto event_dispatcher() -> stdui::event_dispatcher & = 0;

  /// Requests a redraw of the window contents.
  virtual void request_redraw() = 0;

  /// Sets the redraw callback (called when the window needs to be redrawn).
  virtual void set_redraw_callback(std::function<void()> callback) = 0;
};

/**
 * Platform-specific initialization and window creation.
 *
 * Each platform backend provides an implementation of this interface
 * as the entry point for creating windows and accessing platform services.
 */
class platform {
public:
  virtual ~platform() = default;

  /// Initializes the platform backend. Must be called before creating windows.
  virtual void initialize() = 0;

  /// Shuts down the platform backend. Called on application exit.
  virtual void shutdown() = 0;

  /// Creates a new window with the specified size and title.
  virtual auto create_window(stdui::size size, std::string const &title)
      -> std::unique_ptr<platform_window> = 0;

  /// Returns the default text measurer factory for this platform.
  virtual auto text_measurer_factory() const -> stdui::text_measurer_factory const & = 0;

  /// Runs the platform event loop (blocks until all windows are closed).
  virtual void run_event_loop() = 0;

  /// Stops the event loop (causes run_event_loop to return).
  virtual void stop_event_loop() = 0;
};

/**
 * Returns the platform instance for the current system.
 *
 * This function is implemented by each platform backend:
 * - libstdui_platform_macos.a provides macOS implementation
 * - libstdui_platform_linux.a provides Linux implementation
 * - libstdui_platform_windows.a provides Windows implementation
 *
 * Applications link against the appropriate platform library.
 */
auto get_platform() -> platform &;

/**
 * Null platform implementation for testing.
 *
 * All operations are no-ops or return dummy values. Useful for
 * testing layout and state management without requiring a display.
 */
class null_platform : public platform {
public:
  void initialize() override {}
  void shutdown() override {}

  auto create_window(stdui::size, std::string const &) -> std::unique_ptr<platform_window> override;

  auto text_measurer_factory() const -> stdui::text_measurer_factory const & override {
    struct null_text_measurer_impl : public text_measurer {
      auto measure(std::string_view text, font_descriptor const &) const -> size override {
        // Simple heuristic: 8 pixels per character, 16 pixels height
        return {static_cast<double>(text.length()) * 8.0, 16.0};
      }
      auto measure_wrapped(std::string_view text, font_descriptor const &,
                           double max_width) const -> size override {
        double chars_per_line = max_width / 8.0;
        double lines = std::ceil(text.length() / chars_per_line);
        return {std::min(static_cast<double>(text.length()) * 8.0, max_width), lines * 16.0};
      }
    };

    struct null_text_measurer_factory_impl : public stdui::text_measurer_factory {
      auto create() const -> std::unique_ptr<text_measurer> override {
        return std::make_unique<null_text_measurer_impl>();
      }
    };

    static null_text_measurer_factory_impl factory;
    return factory;
  }

  void run_event_loop() override {}
  void stop_event_loop() override {}
};

/**
 * Null window implementation for testing.
 */
class null_window : public platform_window {
public:
  explicit null_window(stdui::size size, std::string title,
                       std::unique_ptr<stdui::renderer> renderer)
      : size_(size), title_(std::move(title)), renderer_(std::move(renderer)) {}

  void show() override {}
  void hide() override {}

  auto size() const -> stdui::size override { return size_; }
  void set_size(stdui::size new_size) override { size_ = new_size; }

  auto title() const -> std::string override { return title_; }
  void set_title(std::string const &title) override { title_ = title; }

  auto renderer() -> stdui::renderer & override { return *renderer_; }
  auto event_dispatcher() -> stdui::event_dispatcher & override { return dispatcher_; }

  void request_redraw() override {
    if (redraw_callback_) {
      redraw_callback_();
    }
  }

  void set_redraw_callback(std::function<void()> callback) override {
    redraw_callback_ = std::move(callback);
  }

private:
  stdui::size size_;
  std::string title_;
  std::unique_ptr<stdui::renderer> renderer_;
  simple_event_dispatcher dispatcher_;
  std::function<void()> redraw_callback_;
};

inline auto null_platform::create_window(stdui::size size, std::string const &title)
    -> std::unique_ptr<platform_window> {
  auto measurer = text_measurer_factory().create();
  auto renderer = std::make_unique<null_renderer>(std::move(measurer));
  return std::make_unique<null_window>(size, title, std::move(renderer));
}

} // namespace stdui
