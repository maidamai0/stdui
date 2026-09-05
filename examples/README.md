# stdui Examples

This directory contains example applications demonstrating stdui's features and capabilities.

## Building Examples

Examples are not built by default. To build them:

```bash
cmake -S . -B build -DBUILD_EXAMPLES=ON
cmake --build build
```

Run an example:

```bash
./build/examples/01_simple_layout
```

## Examples Overview

### Example 1: Simple Layout
**File:** `01_simple_layout.cpp`

Demonstrates basic layout primitives:
- Text elements
- VStack (vertical stack)
- HStack (horizontal stack)
- Overlay (z-axis layering)
- Simple composition patterns

**Concepts:** View expressions, composition, layout primitives

---

### Example 2: Stateful Counter
**File:** `02_stateful_counter.cpp`

Demonstrates state management:
- Component system
- Framework-managed state
- State persistence across evaluations
- Component identity
- Multiple state variables

**Concepts:** `component<Tag>`, `ctx.state()`, identity

---

### Example 3: Dynamic List
**File:** `03_dynamic_list.cpp`

Demonstrates dynamic collections:
- Runtime-sized lists
- Explicit identity for list items
- Data-driven UI
- Identity preservation across reorders

**Concepts:** `dynamic_list()`, explicit identity, reconciliation

---

### Example 4: Application Layout
**File:** `04_application_layout.cpp`

Demonstrates a complete application layout:
- Side panel navigation
- Main viewport (for heavy rendering)
- Properties panel
- Status bar
- Grid-based layout

**Concepts:** Target use case, Neovim/Blender-style UI

**This represents stdui's primary use case:**
- Large viewport for graphics/rendering
- Lightweight UI chrome around it
- Navigation and settings in side panels

---

## Understanding the Examples

### Progression

1. **Example 1** - Start here to understand the DSL
2. **Example 2** - Learn state management
3. **Example 3** - Master dynamic content
4. **Example 4** - See realistic application structure

### What's Not Shown Yet

These examples are **headless** - they create view expressions but don't render them. This is because:

1. Phase 4 (Rendering) is currently in progress
2. The examples demonstrate the **programming model**
3. Rendering backends will be added in subsequent phases

Once rendering is complete, these examples will be extended to show:
- Actual visual output
- User interaction
- Event handling
- Animation

### Code Style

Examples follow these conventions:

- `make_*()` functions return view expressions
- Component tags use `*_tag` suffix
- Simple functions for stateless components
- `component<Tag>()` for stateful components
- Composition over inheritance

### Next Steps

After exploring the examples:

1. Read `docs/tutorials.md` for deeper explanations
2. Check `docs/getting-started.md` for setup
3. Review `docs/architecture.md` for design philosophy
4. See `tests/` for comprehensive API usage

## Contributing Examples

Want to add an example? Make sure it:

1. Demonstrates a distinct concept
2. Is well-commented
3. Follows the existing style
4. Includes a description in this README
5. Can be built standalone

Submit a PR with your example!
