#pragma once

#include <stdui/geometry.hpp>

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <variant>

namespace stdui {

/// Mouse button identifiers.
enum class mouse_button { left, right, middle };

/// Keyboard modifier flags.
struct keyboard_modifiers {
  bool shift = false;
  bool control = false;
  bool alt = false;
  bool meta = false; // Command on macOS, Windows key on Windows

  bool operator==(keyboard_modifiers const &) const = default;
};

/// Mouse event data.
struct mouse_event {
  point position;
  mouse_button button = mouse_button::left;
  keyboard_modifiers modifiers;
  std::uint32_t click_count = 1;
};

/// Keyboard event data.
struct keyboard_event {
  std::string key; // Platform-normalized key string (e.g., "Enter", "a", "ArrowUp")
  keyboard_modifiers modifiers;
  bool is_repeat = false;
};

/// Window resize event data.
struct resize_event {
  size new_size;
};

/// Platform event types.
using platform_event = std::variant<mouse_event, keyboard_event, resize_event>;

/**
 * Event handler callback type.
 *
 * Returns true if the event was handled (stops propagation),
 * false to allow further processing.
 */
using event_handler = std::function<bool(platform_event const &)>;

/**
 * Event dispatcher interface.
 *
 * Platform backends implement this to translate native events
 * into stdui's platform_event representation.
 */
class event_dispatcher {
public:
  virtual ~event_dispatcher() = default;

  /// Registers an event handler. Returns a handler ID for later removal.
  virtual auto add_handler(event_handler handler) -> std::uint64_t = 0;

  /// Removes a previously registered handler.
  virtual void remove_handler(std::uint64_t handler_id) = 0;

  /// Dispatches a platform event to registered handlers.
  virtual void dispatch(platform_event const &event) = 0;
};

/**
 * Simple event dispatcher implementation.
 *
 * This is a reference implementation suitable for single-threaded
 * applications. Platform backends may provide more sophisticated
 * implementations with thread-safety or priority handling.
 */
class simple_event_dispatcher : public event_dispatcher {
public:
  auto add_handler(event_handler handler) -> std::uint64_t override {
    auto id = next_id_++;
    handlers_.emplace_back(id, std::move(handler));
    return id;
  }

  void remove_handler(std::uint64_t handler_id) override {
    handlers_.erase(
        std::remove_if(handlers_.begin(), handlers_.end(),
                       [handler_id](auto const &pair) { return pair.first == handler_id; }),
        handlers_.end());
  }

  void dispatch(platform_event const &event) override {
    for (auto const &[id, handler] : handlers_) {
      if (handler(event)) {
        break; // Event was handled, stop propagation
      }
    }
  }

private:
  std::vector<std::pair<std::uint64_t, event_handler>> handlers_;
  std::uint64_t next_id_ = 1;
};

} // namespace stdui
