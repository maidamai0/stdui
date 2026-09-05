# Getting Started with stdui {#getting_started}

## What is stdui?

`stdui` is an experimental modern declarative UI framework for C++20. It provides a strongly-typed compositional DSL for building user interfaces, inspired by modern declarative frameworks but designed specifically for C++.

**Key characteristics:**
- Header-only library (currently)
- Strongly typed compositional DSL
- Separation between view expressions and persistent runtime state
- Generic layout system with intrinsic sizing
- Platform-agnostic rendering abstraction

**⚠️ Warning:** stdui is under heavy development and is not ready for production use.

## Installation

### Prerequisites

- C++20-compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.20 or later

### Building from source

```bash
git clone https://github.com/maidamai0/stdui.git
cd stdui
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

### Installing

```bash
cmake --install build --prefix /path/to/install
```

### Using stdui in your project

Add to your `CMakeLists.txt`:

```cmake
find_package(stdui CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE stdui::stdui)
```

## Core Concepts

### 1. View Expressions

View expressions describe UI structure. They are temporary descriptions, not the persistent UI itself:

```cpp
#include <stdui/expressions.hpp>

auto greeting = stdui::text("Hello, World!");
```

### 2. Composition

Build complex UIs by composing simple expressions:

```cpp
auto ui = stdui::vstack(
    stdui::text("Title"),
    stdui::text("Subtitle"),
    stdui::hstack(
        stdui::text("Left"),
        stdui::text("Right")
    )
);
```

### 3. Layout Primitives

stdui provides three fundamental layout primitives:

- **VStack** - Vertical stack (top to bottom)
- **HStack** - Horizontal stack (left to right)
- **Overlay** - Z-axis layering (back to front)
- **Grid** - Fixed-column grid layout

### 4. Components

Components are reusable UI building blocks. They can be simple functions:

```cpp
auto make_header(std::string title) {
    return stdui::vstack(
        stdui::text(std::move(title)),
        stdui::text("---")
    );
}
```

Or stateful using the component system:

```cpp
struct counter_tag {};

auto counter_component() {
    return stdui::component<counter_tag>([](auto& ctx) {
        auto count = ctx.state(0);
        
        return stdui::vstack(
            stdui::text("Count: " + std::to_string(count.get())),
            // button would increment count.set(count.get() + 1)
        );
    });
}
```

### 5. State Management

stdui distinguishes between:

- **Application state** - Your domain data
- **Framework-managed state** - UI-local state tied to component identity

State persists across expression evaluations through reconciliation.

### 6. Identity

Components need stable identity for state to persist:

- **Structural identity** - Default, based on position in the tree
- **Explicit identity** - Use `identified()` for dynamic collections

```cpp
auto items = std::vector<item>{/*...*/};

stdui::dynamic_list(
    items,
    [](auto& item) { return item.id; },  // explicit identity
    [](auto& item) { return stdui::text(item.name); }
);
```

## Your First Application

Here's a minimal example showing the core concepts:

```cpp
#include <stdui/expressions.hpp>
#include <stdui/layout.hpp>
#include <stdui/geometry.hpp>

int main() {
    // 1. Create a view expression
    auto ui = stdui::vstack(
        stdui::text("Welcome to stdui"),
        stdui::hstack(
            stdui::text("Version: "),
            stdui::text("0.1.0")
        )
    );
    
    // 2. Measure the layout
    auto proposal = stdui::proposal::bounded(400.0, 300.0);
    auto measurement = stdui::measure_vstack(
        std::array{
            stdui::text("Welcome to stdui"),
            stdui::hstack(
                stdui::text("Version: "),
                stdui::text("0.1.0")
            )
        },
        proposal
    );
    
    // 3. In a real app, this would be rendered through a backend
    // Currently stdui provides the foundation; rendering backends
    // are planned for Phase 4
    
    return 0;
}
```

## Architecture Overview

```
Application Code
       ↓
  View Expressions (DSL)
       ↓
  Reconciliation
       ↓
Persistent Representation
       ↓
   ┌────┴────┬──────┬──────────┐
   ↓         ↓      ↓          ↓
Layout  Interaction  Semantics  Rendering
```

Key boundaries:
- View expressions ≠ persistent UI
- UI primitives ≠ rendering primitives  
- Component identity ≠ C++ object identity
- State handles ≠ actual storage

## What's Currently Available

**✅ Implemented (Phase 1-3):**
- Typed view expressions
- Component system with state management
- Layout primitives (stacks, grids, overlays)
- Reconciliation and identity tracking
- Geometry and measurement system
- Event handling foundation

**🚧 In Progress (Phase 4):**
- Rendering backend abstraction
- Platform-specific renderers

**📅 Planned (Phase 5+):**
- Animation and transitions
- Theming and environment values
- Standard component library (Button, TextField, etc.)
- Accessibility support

## Next Steps

1. **Read the tutorials** - See `docs/tutorials/` for guided examples
2. **Explore examples** - Check `examples/` for complete applications
3. **Read architecture docs** - `docs/architecture.md` explains the design
4. **Run tests** - `tests/` shows API usage patterns
5. **Join development** - See `docs/roadmap.md` for what's coming

## Learning Resources

- [Architecture Overview](architecture.md) - Deep dive into design decisions
- [API Reference](https://your-docs-site.com) - Complete API documentation (generated via Doxygen)
- [Design Decisions](design-decisions.md) - Why stdui works this way
- [Roadmap](roadmap.md) - Development phases and timeline

## Getting Help

- File an issue on GitHub for bugs
- Check existing issues for known problems
- Read the architecture docs for design questions

## Contributing

stdui is in active development. See the roadmap for current priorities and contribution opportunities.
