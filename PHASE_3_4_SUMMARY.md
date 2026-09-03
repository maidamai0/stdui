# Phase 3 & 4 Implementation Summary

## Overview

This document summarizes the implementation of **Phase 3: Component System** and **Phase 4: Runtime Integration** from the stdui roadmap. These phases add a complete component-based architecture with persistent state and application lifecycle management.

## Phase 3: Component System

### Core Components

#### `component_base` - Base Class
```cpp
class component_base {
public:
  virtual ~component_base() = default;
  virtual auto body(component_context &ctx) const -> inspection_node = 0;
  virtual auto type_id() const -> std::type_index = 0;
  virtual auto instance_id() const -> std::size_t;
};
```

- Base class for all components
- `body()` returns `inspection_node` (runtime representation of the UI tree)
- `type_id()` for component type identification
- `instance_id()` for per-instance identity (defaults to address)
- **Note**: Renamed from `component` to `component_base` to avoid collision with `component()` DSL function in `expressions.hpp`

#### `typed_component<Derived>` - CRTP Helper
```cpp
template <typename Derived> 
class typed_component : public component_base {
public:
  auto type_id() const -> std::type_index override { 
    return typeid(Derived); 
  }
};
```

- CRTP pattern for automatic type identification
- Eliminates boilerplate in component subclasses
- Example usage:
  ```cpp
  class counter_component : public stdui::typed_component<counter_component> {
    auto body(component_context &ctx) const -> inspection_node override {
      auto count = ctx.state(0);
      return inspect(text("Count: " + std::to_string(count.get())));
    }
  };
  ```

#### `component_id` - Identity Tracking
```cpp
struct component_id {
  std::type_index type;
  std::size_t instance_id;
  bool operator==(component_id const &) const = default;
};
```

- Uniquely identifies component instances
- Used as key in `component_registry` for state storage
- Hash specialization for use in `std::unordered_map`

#### `component_registry` - State Management
```cpp
class component_registry {
public:
  auto get_or_create_storage(component_id const &id) -> state_storage &;
  void remove_storage(component_id const &id);
  void clear();
  auto component_count() const -> std::size_t;
};
```

- Maps `component_id` to `state_storage` instances
- Persists component state across evaluations
- One registry per application instance

#### `component_evaluator` - Evaluation Engine
```cpp
class component_evaluator {
public:
  explicit component_evaluator(component_registry &registry);
  auto evaluate(component_base const &comp, 
                std::function<void()> on_change) -> inspection_node;
};
```

- Evaluates components with state context
- Sets up `component_context` with storage and change callback
- Returns `inspection_node` for layout materialization

### Architecture: Bridging Compile-Time and Runtime

**The Challenge**: stdui uses compile-time template expressions (`text_expression`, `vstack_expression<T...>`, etc.) for zero-cost abstractions, but components need runtime polymorphism for dynamic UI trees.

**The Solution**: `inspection_node` as the bridge
1. **Compile-time**: DSL expressions are templates with static types
2. **Runtime bridge**: `inspect()` converts any expression to `inspection_node`
3. **Component body()**: Returns `inspection_node` (runtime representation)
4. **Application**: Works entirely with `inspection_node` trees

```cpp
// Component body() bridges the two worlds:
auto body(component_context &ctx) const -> inspection_node override {
  // Create compile-time DSL expression
  auto expr = vstack(text("Hello"), text("World"));
  // Convert to runtime representation
  return inspect(expr);
}
```

This maintains zero-cost abstractions in user DSL code while allowing dynamic component composition.

## Phase 4: Runtime Integration

### Application Lifecycle

#### `app_config` - Configuration
```cpp
struct app_config {
  std::string title = "stdui Application";
  size window_size = {800.0, 600.0};
  color background_color = color::white();
};
```

#### `application` - Runtime Manager
```cpp
class application {
public:
  explicit application(std::shared_ptr<component_base const> root, 
                      app_config config = {});
  
  void initialize(platform &platform);
  void run();
  void invalidate();
  
  auto registry() -> component_registry &;
  auto layout_tree() const -> layout_node const *;
};
```

**Lifecycle Flow**:
1. **Construction**: Stores root component and config
2. **Initialization**: 
   - Creates platform window
   - Sets up event handlers and redraw callback
   - Performs initial evaluation and layout
3. **Event Loop**: 
   - Platform processes events
   - Components can trigger `invalidate()` via state changes
   - Update cycle: evaluate → materialize → render
4. **Shutdown**: Automatic cleanup

**Update Cycle**:
```
invalidate() → needs_update_ = true → window→request_redraw()
  ↓
update():
  1. component_evaluator.evaluate(root_component)
     → inspection_node tree
  2. materialize_layout(inspection_node)
     → layout_node tree
  ↓
render():
  1. layout_tree→arrange(window_bounds)
     → layout_box tree with final positions
  2. render_box() recursively
     → draw to window
```

### Event Handling

The application integrates with the platform event system:
- Window events dispatched to `handle_event()`
- Future: Route events to specific components
- Current: Global event handling (placeholder for component routing)

### Reactive Updates

Components trigger updates via `on_change` callback:
```cpp
auto body(component_context &ctx) const -> inspection_node override {
  auto count = ctx.state(0);
  // When count.set() is called, on_change fires
  // which calls application.invalidate()
  // which schedules re-evaluation
  return inspect(text("Count: " + std::to_string(count.get())));
}
```

The `on_change` callback is provided by `component_evaluator` and captures `application::invalidate()`.

## Testing

### Component Tests (`component_tests.cpp`)

10 test cases covering:
- `component_id` equality and hashing
- `component_registry` storage management
- `component_evaluator` evaluation
- State persistence across evaluations
- Separate state for different instances
- Container component evaluation
- `typed_component` automatic type identification
- Default `instance_id` behavior

### Application Tests (`application_tests.cpp`)

9 test cases covering:
- `app_config` default and custom values
- Application construction
- Platform initialization
- Component tree evaluation
- Layout tree materialization
- Stateful component handling
- Layout component structure
- Invalidation mechanism
- Registry sharing across components

### Test Results

```
100% tests passed, 0 tests failed out of 11

Total Test time (real) =   1.13 sec
```

All existing tests continue to pass, confirming backward compatibility.

## Example Usage

### Simple Application

```cpp
class hello_component : public stdui::typed_component<hello_component> {
public:
  auto body(stdui::component_context &ctx) const -> stdui::inspection_node override {
    return stdui::inspect(stdui::text("Hello, World!"));
  }
};

int main() {
  auto root = std::make_shared<hello_component>();
  stdui::run_app(root, stdui::get_platform());
}
```

### Stateful Application

```cpp
class counter_component : public stdui::typed_component<counter_component> {
public:
  auto body(stdui::component_context &ctx) const -> stdui::inspection_node override {
    auto count = ctx.state(0);
    
    return stdui::inspect(stdui::vstack(
      stdui::text("Count: " + std::to_string(count.get())),
      stdui::button("Increment", [count]() mutable { 
        count.set(count.get() + 1); 
      })
    ));
  }
};
```

### Custom Configuration

```cpp
int main() {
  auto root = std::make_shared<my_component>();
  
  stdui::app_config config{
    .title = "My App",
    .window_size = {1024.0, 768.0},
    .background_color = stdui::color::black(),
  };
  
  stdui::run_app(root, stdui::get_platform(), config);
}
```

## Key Design Decisions

### 1. Component Base Class Rename
**Problem**: Name collision between `component` class and `component()` DSL function.
**Solution**: Renamed class to `component_base`, keeping DSL function unchanged.
**Impact**: Clear separation, minimal user-facing change (both use `typed_component` helper).

### 2. Inspection Node Bridge
**Problem**: Compile-time template DSL vs runtime component polymorphism.
**Solution**: Components return `inspection_node`, the runtime representation.
**Impact**: Clean separation of concerns, maintains zero-cost DSL abstractions.

### 3. Address-Based Instance Identity
**Problem**: Need unique identity for component instances.
**Solution**: Default `instance_id()` returns component address.
**Impact**: Simple, automatic, can be overridden for list items with keys.

### 4. Single Registry Per Application
**Problem**: Where to store component state?
**Solution**: Application owns one `component_registry` for all components.
**Impact**: Centralized state management, clean lifecycle.

## Integration with Existing System

### Phase 1 Integration
- Components use `ctx.state<T>()` from Phase 1 state management
- State storage mechanism unchanged
- Reactive updates via existing `on_change` callbacks

### Phase 2 Integration
- Application uses `platform` abstraction for window creation
- Rendering via `renderer` interface
- Event handling via `event_dispatcher`
- Text measurement for layout materialization

### DSL Integration
- Components use existing DSL expressions (`text`, `vstack`, `hstack`, etc.)
- `inspect()` function converts DSL to `inspection_node`
- Layout materialization works on `inspection_node` trees
- Zero-cost abstractions preserved in user code

## What's Not Implemented (Future Work)

### Event Routing to Components
Currently, all events go to `application::handle_event()`. Future:
- Route mouse events to components based on layout bounds
- Propagate keyboard events to focused component
- Touch event handling

### Component-Specific DSL Function
The `component<Kind>(Body)` DSL function exists but isn't integrated:
- Would allow embedding components in DSL: `vstack(text("Header"), component<MyComp>(...), text("Footer"))`
- Needs recursive component evaluation in `component_evaluator`
- Deferred to maintain focus on core architecture

### Diffing and Minimal Updates
Current: Full re-evaluation and re-materialization on every update.
Future optimization:
- Diff `inspection_node` trees
- Only re-materialize changed subtrees
- Partial rendering

### Component Lifecycle Hooks
Future: `onMount()`, `onUnmount()`, `onUpdate()` lifecycle methods.

### Async Components
Future: Components that return `std::future<inspection_node>` for async data loading.

## Files Added

- `include/stdui/component.hpp` - Component system (169 lines)
- `include/stdui/application.hpp` - Application runtime (171 lines)
- `tests/component_tests.cpp` - Component tests (175 lines)
- `tests/application_tests.cpp` - Application tests (153 lines)

## Status

✅ **Phase 3: Component System** - Complete
✅ **Phase 4: Runtime Integration** - Complete

Next: Phase 5 (Backend Rendering) - Platform-specific window and rendering implementations.
