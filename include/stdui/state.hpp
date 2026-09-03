#pragma once

#include <any>
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace stdui {

/// Forward declaration
class component_context;

/**
 * Handle to framework-managed persistent state storage.
 *
 * A state<T> handle is not ordinary local C++ storage. It is a reference
 * to persistent storage keyed by component identity and state slot.
 * The actual value survives re-evaluation of view expressions.
 */
template <class T> class state {
public:
  state() = default;

  /// Returns the current value of this state.
  auto get() const -> T const & {
    if (!storage_) {
      throw std::runtime_error("state: accessing uninitialized state handle");
    }
    return *storage_;
  }

  /// Updates the stored value and marks the component dirty.
  void set(T value) {
    if (!storage_) {
      throw std::runtime_error("state: accessing uninitialized state handle");
    }
    *storage_ = std::move(value);
    if (on_change_) {
      on_change_();
    }
  }

  /// Allows reading the state value.
  auto operator*() const -> T const & { return get(); }

  /// Allows accessing the state value members.
  auto operator->() const -> T const * { return &get(); }

  /// Modifies the state in place with a function.
  template <class Fn>
    requires std::invocable<Fn, T &>
  void modify(Fn &&fn) {
    if (!storage_) {
      throw std::runtime_error("state: accessing uninitialized state handle");
    }
    std::forward<Fn>(fn)(*storage_);
    if (on_change_) {
      on_change_();
    }
  }

private:
  friend class component_context;

  state(T *storage, std::function<void()> on_change)
      : storage_(storage), on_change_(std::move(on_change)) {}

  T *storage_ = nullptr;
  std::function<void()> on_change_;
};

/**
 * Storage for persistent component state.
 *
 * Each component instance has its own state_storage, keyed by component
 * identity in the runtime. State slots within a component are allocated
 * sequentially as state<T> handles are created during body evaluation.
 */
class state_storage {
public:
  state_storage() = default;

  /// Allocates a state slot or returns the existing value.
  template <class T> auto get_or_create(std::size_t slot, T initial_value) -> T * {
    if (slot >= slots_.size()) {
      slots_.resize(slot + 1);
    }

    auto &slot_data = slots_[slot];
    if (!slot_data.has_value()) {
      slot_data = std::make_shared<T>(std::move(initial_value));
    }

    auto ptr = std::any_cast<std::shared_ptr<T>>(&slot_data);
    return ptr ? ptr->get() : nullptr;
  }

  /// Returns the number of allocated state slots.
  auto slot_count() const -> std::size_t { return slots_.size(); }

  /// Resets all state slots (used when component identity changes).
  void reset() { slots_.clear(); }

private:
  std::vector<std::any> slots_;
};

/**
 * Context provided to component bodies during evaluation.
 *
 * Provides access to framework-managed state and other contextual services.
 */
class component_context {
public:
  component_context() = default;

  explicit component_context(state_storage *storage, std::function<void()> on_state_change)
      : storage_(storage), on_state_change_(std::move(on_state_change)), current_slot_(0) {}

  /// Creates or retrieves a persistent state handle.
  /**
   * Each call to state() within a component body allocates a new state slot.
   * The slot is identified by call order, so calls must be stable across
   * re-evaluations (no conditional state allocation).
   */
  template <class T> auto state(T initial_value) -> state<T> {
    if (!storage_) {
      throw std::runtime_error("component_context: no storage available");
    }

    T *value_ptr = storage_->get_or_create(current_slot_++, std::move(initial_value));
    stdui::state<T> handle;
    handle.storage_ = value_ptr;
    handle.on_change_ = on_state_change_;
    return handle;
  }

  /// Resets the state slot counter (called before re-evaluating component body).
  void reset_slot_counter() { current_slot_ = 0; }

  /// Returns the current state slot counter.
  auto current_slot() const -> std::size_t { return current_slot_; }

private:
  state_storage *storage_ = nullptr;
  std::function<void()> on_state_change_;
  std::size_t current_slot_ = 0;
};

} // namespace stdui
