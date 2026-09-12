#pragma once

#include <stdui/events.hpp>

#include <chrono>
#include <cstddef>
#include <optional>
#include <queue>

namespace stdui {

/// Monotonic logical time used by the runtime.
using runtime_time = std::chrono::nanoseconds;

/// Clock abstraction. Platform hosts provide the real clock; tests use manual time.
class runtime_clock {
public:
  virtual ~runtime_clock() = default;
  virtual auto now() const -> runtime_time = 0;
};

/// Deterministic clock for runtime tests.
class manual_clock : public runtime_clock {
public:
  auto now() const -> runtime_time override { return now_; }

  void advance(std::chrono::nanoseconds delta) { now_ += delta; }

private:
  runtime_time now_{};
};

/// Source of normalized runtime events.
class event_source {
public:
  virtual ~event_source() = default;
  virtual auto poll() -> std::optional<platform_event> = 0;
};

/// Deterministic event source for runtime tests.
class queued_event_source : public event_source {
public:
  void push(platform_event event) { events_.push(std::move(event)); }

  auto poll() -> std::optional<platform_event> override {
    if (events_.empty()) {
      return std::nullopt;
    }
    auto event = std::move(events_.front());
    events_.pop();
    return event;
  }

  auto empty() const -> bool { return events_.empty(); }

  auto size() const -> std::size_t { return events_.size(); }

private:
  std::queue<platform_event> events_;
};

/// Requests the next animation or rendering frame.
class frame_scheduler {
public:
  virtual ~frame_scheduler() = default;
  virtual void request_frame() = 0;
};

} // namespace stdui
