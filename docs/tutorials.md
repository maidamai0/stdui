# stdui Tutorials {#tutorials}

## Tutorial 1: Building Your First Layout

### Introduction

This tutorial walks through creating a simple application layout using stdui's core primitives. You'll learn about stacks, text elements, and basic composition.

### Prerequisites

- C++20 compiler
- stdui installed (see Getting Started guide)
- Basic familiarity with C++

### Step 1: Include Headers

```cpp
#include <stdui/expressions.hpp>
#include <stdui/layout.hpp>
#include <stdui/geometry.hpp>
#include <string>
```

### Step 2: Create a Simple Text Element

The most basic UI element is text:

```cpp
auto greeting = stdui::text("Hello, stdui!");
```

This creates a **view expression** - a description of what should appear, not the actual UI element itself.

### Step 3: Stack Elements Vertically

Let's create a vertical stack of text elements:

```cpp
auto make_profile() {
    return stdui::vstack(
        stdui::text("John Doe"),
        stdui::text("Software Engineer"),
        stdui::text("john@example.com")
    );
}
```

`vstack` arranges children from top to bottom. Each child is a separate text element.

### Step 4: Add Horizontal Layout

Combine vertical and horizontal stacks:

```cpp
auto make_contact_card() {
    return stdui::vstack(
        stdui::text("Contact Information"),
        stdui::text("---"),
        stdui::hstack(
            stdui::text("Name: "),
            stdui::text("John Doe")
        ),
        stdui::hstack(
            stdui::text("Email: "),
            stdui::text("john@example.com")
        )
    );
}
```

`hstack` arranges children from left to right.

### Step 5: Use Overlay for Layering

Overlay places elements on top of each other:

```cpp
auto make_badge() {
    return stdui::overlay(
        stdui::text("Background Layer"),
        stdui::text("Foreground")
    );
}
```

Children are drawn in order (first = back, last = front).

### Step 6: Create a Grid Layout

For more structured layouts, use grids:

```cpp
auto make_dashboard() {
    auto options = stdui::grid_options{};
    options.columns = 2;
    options.spacing = 10.0;
    
    return stdui::grid(
        options,
        stdui::text("Widget 1"),
        stdui::text("Widget 2"),
        stdui::text("Widget 3"),
        stdui::text("Widget 4")
    );
}
```

### Step 7: Measure and Arrange

To actually layout your UI, you need to measure and arrange:

```cpp
int main() {
    auto ui = make_contact_card();
    
    // Define constraints
    auto proposal = stdui::proposal::bounded(400.0, 300.0);
    
    // Measure children
    auto measurement = stdui::measure_vstack(
        /* children array */,
        proposal
    );
    
    // Arrange in a rectangle
    stdui::rect bounds{{0, 0}, {400, 300}};
    auto frames = stdui::arrange_vstack(
        measurement.children,
        bounds
    );
    
    // frames now contains positioned rectangles for each child
    return 0;
}
```

### Key Takeaways

1. **View expressions describe structure** - They're not the actual UI
2. **Three layout primitives** - VStack, HStack, Overlay (plus Grid)
3. **Composition is powerful** - Nest stacks to create complex layouts
4. **Two-phase layout** - Measure constraints, then arrange geometry

### Next Steps

- Tutorial 2: Working with State
- Tutorial 3: Creating Reusable Components
- Tutorial 4: Dynamic Lists and Identity

---

## Tutorial 2: Working with State

### Introduction

Learn how to create interactive UIs with state management. You'll build a counter component that responds to user actions.

### Understanding State in stdui

stdui has two types of state:

1. **Application state** - Your domain data (models, services)
2. **Framework state** - UI-local state managed by stdui

Framework state persists across view expression evaluations.

### Step 1: Define a Stateful Component

```cpp
#include <stdui/component.hpp>
#include <stdui/state.hpp>

struct counter_tag {};  // Unique type for component identity

auto make_counter() {
    return stdui::component<counter_tag>([](auto& ctx) {
        // Request state storage with initial value
        auto count = ctx.state(0);
        
        return stdui::vstack(
            stdui::text("Counter Example"),
            stdui::text("Count: " + std::to_string(count.get()))
            // Button to increment would go here
        );
    });
}
```

### Step 2: Understanding Component Context

The `component_context` provides:

- `state<T>(initial_value)` - Get or create persistent state
- State is tied to component identity
- State survives across re-evaluations

### Step 3: Modifying State

```cpp
auto make_interactive_counter() {
    return stdui::component<counter_tag>([](auto& ctx) {
        auto count = ctx.state(0);
        
        // In a real implementation, this would be triggered by a button
        auto increment = [&count, &ctx]() {
            count.set(count.get() + 1);
            ctx.mark_dirty();  // Request re-render
        };
        
        return stdui::vstack(
            stdui::text("Count: " + std::to_string(count.get())),
            stdui::text("[+] Button")  // Placeholder for actual button
        );
    });
}
```

### Step 4: Multiple State Variables

Components can have multiple state variables:

```cpp
struct form_tag {};

auto make_form() {
    return stdui::component<form_tag>([](auto& ctx) {
        auto name = ctx.state(std::string{});
        auto age = ctx.state(0);
        auto submitted = ctx.state(false);
        
        return stdui::vstack(
            stdui::text("Name: " + name.get()),
            stdui::text("Age: " + std::to_string(age.get())),
            stdui::text(submitted.get() ? "Submitted!" : "Not submitted")
        );
    });
}
```

### Step 5: Component Identity

Identity determines which state belongs to which component:

```cpp
// Same component type, different instances
auto ui = stdui::vstack(
    make_counter(),  // Instance 1
    make_counter()   // Instance 2
);
// Each has independent state!
```

Use explicit identity for dynamic scenarios:

```cpp
auto items = std::vector<int>{1, 2, 3};

auto ui = stdui::dynamic_list(
    items,
    [](int id) { return id; },  // Identity function
    [](int id) {
        return stdui::identified(
            id,
            make_counter()  // Each list item gets unique state
        );
    }
);
```

### Key Takeaways

1. **State persists across evaluations** - Not ordinary C++ variables
2. **Component identity is crucial** - Determines which state belongs where
3. **Use component<Tag>** - The tag type provides compile-time identity
4. **Explicit identity for lists** - Use `identified()` and `dynamic_list()`

---

## Tutorial 3: Creating Reusable Components

### Introduction

Learn to build reusable, composable UI components following stdui's architecture.

### Simple Function Components

The simplest component is just a function:

```cpp
auto button_label(std::string_view text) {
    return stdui::hstack(
        stdui::text("["),
        stdui::text(text),
        stdui::text("]")
    );
}

// Use it
auto ui = stdui::vstack(
    button_label("OK"),
    button_label("Cancel")
);
```

### Parameterized Components

Components can accept configuration:

```cpp
struct card_config {
    std::string title;
    std::string subtitle;
    bool highlighted = false;
};

auto make_card(card_config const& config) {
    auto title_text = config.highlighted 
        ? "*** " + config.title + " ***"
        : config.title;
    
    return stdui::vstack(
        stdui::text(title_text),
        stdui::text(config.subtitle),
        stdui::text("---")
    );
}
```

### Components with Children

Accept view expressions as children:

```cpp
template<stdui::view_expression... Children>
auto make_panel(std::string title, Children&&... children) {
    return stdui::vstack(
        stdui::text("=== " + title + " ==="),
        stdui::vstack(std::forward<Children>(children)...),
        stdui::text("================")
    );
}

// Use it
auto ui = make_panel(
    "Settings",
    stdui::text("Option 1"),
    stdui::text("Option 2")
);
```

### Stateful Reusable Components

For components needing state, use the component system:

```cpp
struct expandable_section_tag {};

struct expandable_config {
    std::string title;
    std::function<stdui::inspection_node()> content;
};

auto make_expandable_section(expandable_config config) {
    return stdui::component<expandable_section_tag>([cfg = std::move(config)](auto& ctx) {
        auto expanded = ctx.state(false);
        
        if (expanded.get()) {
            return stdui::vstack(
                stdui::text("▼ " + cfg.title),
                cfg.content()
            );
        } else {
            return stdui::vstack(
                stdui::text("▶ " + cfg.title)
            );
        }
    });
}
```

### Component Libraries

Organize related components:

```cpp
namespace my_ui {
    auto heading(std::string text) {
        return stdui::text("# " + std::move(text));
    }
    
    auto paragraph(std::string text) {
        return stdui::text(std::move(text));
    }
    
    auto divider() {
        return stdui::text("---");
    }
}

// Use your library
auto doc = stdui::vstack(
    my_ui::heading("Introduction"),
    my_ui::paragraph("This is a paragraph."),
    my_ui::divider(),
    my_ui::heading("Next Section")
);
```

### Key Takeaways

1. **Start with functions** - Simplest components are pure functions
2. **Compose, don't inherit** - Build complex from simple
3. **Use templates for flexibility** - Accept any view expression
4. **State when needed** - Use `component<Tag>` for stateful components
5. **Create libraries** - Group related components in namespaces

### Next Steps

- Explore the standard component library (when available)
- Read design-decisions.md to understand the philosophy
- Check examples/ for complete applications

