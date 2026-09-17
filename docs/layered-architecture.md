# Layered Architecture {#layered_architecture}

## Purpose

The framework is organized around four collaborating layers with explicit
ownership boundaries. They are libraries, not inheritance hierarchies:

- `stdui_core` defines the user-facing UI model.
- `stdui_runtime` executes state, events, and animation over time.
- `stdui_render` converts backend-neutral scene descriptions into pixels or GPU commands.
- `stdui_platform` adapts operating-system services such as windows, input, clocks, and accessibility.

The intended dependency direction is:

```text
application
    |
    +---- stdui_runtime
    |         |
    |         +---- stdui_core
    |         +---- stdui_platform
    |
    +---- stdui_render
              |
              +---- stdui_core
```

Backend implementations are optional plugins:

```text
stdui_render_metal
stdui_render_directx
stdui_render_vulkan
stdui_render_software

stdui_platform_macos
stdui_platform_windows
stdui_platform_linux
```

## Layer Ownership

| Layer | Owns | Must not own |
| --- | --- | --- |
| Core | Expressions, identity, state model, layout, text contracts, semantics, effects, animation descriptions, scene descriptions | OS APIs, clocks, event loops, GPU resources |
| Runtime | Event dispatch, focus, gestures, state transactions, invalidation, animation execution, frame scheduling | Rendering backends, windows, native input |
| Render | Scene compilation, render passes, resource management, image and GPU output | UI expressions, layout semantics, native events |
| Platform | Windows, native input, display scale, clocks, IME, clipboard, accessibility bridges | Widgets, layout, scene construction |

The dependency rule is strict: lower-level libraries do not include headers from
the execution or backend layers. A backend may depend on its layer contract and
optional platform surface contract, but platform code must not depend on a
concrete renderer.

## Core

Core is the application-facing layer and the framework equivalent of an HTTP
library: users express intent in core types, while runtime decides when that
intent executes.

Core includes:

- geometry and color;
- text measurement and shaping contracts;
- expressions, components, identity, and layout;
- backend-neutral effects such as opacity, clipping, blur, and shadow;
- declarative animation curves, transitions, and animatable values;
- semantic and accessibility descriptions;
- scene and render-target descriptions.

Core does not rasterize an effect. It represents the requested effect and its
layout consequences. The renderer decides whether to use a blur pass, a shadow
map, a software approximation, or another implementation.

## Runtime

Runtime is an operating-system-independent event, state, and animation engine.
It consumes normalized platform events and a clock. Platform adapters translate
native events into the neutral event model.

Runtime can be tested without a window:

- inject a queued event source;
- advance a manual clock;
- dispatch pointer, keyboard, focus, and gesture events;
- execute frames on demand;
- inspect state, layout, scene, and semantic outputs.

The real platform event loop is a host adapter. It should contain very little
framework behavior.

## Animation

Animation descriptions belong to core. Animation execution belongs to runtime.

Core defines curves, transitions, triggers, and animatable values. Runtime owns
timelines, clocks, frame scheduling, interruption, and invalidation. The
renderer only receives the concrete values for the current frame.

## Rendering And Effects

Rendering consumes a backend-neutral scene. A `render_target` abstracts the
output destination:

- an image target for headless or file output;
- a texture target for offscreen composition;
- a window surface target for presentation.

This keeps rendering into an image a first-class path rather than a backend
special case.

## Semantics

Semantics is the platform-independent accessibility and automation view. It
describes meaning rather than pixels:

- role, label, value, description, and error;
- disabled, selected, checked, focused, and expanded state;
- actions such as activate, increment, and set value;
- hierarchy, sibling order, focus order, and relationships.

Platform bridges map the semantic tree to Accessibility APIs. Core tests assert
the semantic tree and interaction behavior, not rendered pixels.

## Test Strategy

- Core tests are deterministic and headless. They verify layout, semantics,
  scene descriptions, effects, animation descriptions, and scene/effect
  composition.
- Runtime tests use a queued event source and manual clock. They verify dispatch,
  state transactions, invalidation, animation progress, and frame output.
- Renderer tests use offscreen image targets. They verify pixels, bounds,
  compositing, and backend parity.
- Platform tests use null or fake window/input adapters. Native integration tests
  are optional and platform-specific.

Visual effect tests therefore have two levels: core verifies the effect
description and its placement in the scene; the renderer verifies the output.

## Component Library

Standard components are built on core primitives and remain ordinary
composable expressions. Initial components should include:

- container and stack primitives;
- text and image;
- button and text input;
- scroll container;
- popover, dialog, menu, and tooltip;
- list and table.

Behavior and accessibility should be shared rather than reimplemented per
component. A Tauri frontend, if added, should be treated as an optional host or
adapter using web primitives and established accessibility libraries; it does
not define the core component model.

### Tauri, Web UI Libraries, And Native Rust UI

Tauri is an application host and webview shell, not a UI component toolkit. Its
frontend can use React, Vue, Svelte, Solid, Leptos, or another web framework.
For component behavior and accessibility, the strongest references are:

- Radix UI and Base UI for unstyled React primitives;
- React Aria for accessibility-heavy behavior;
- Ark UI and Zag.js for framework-neutral state machines;
- Melt UI and Bits UI for Svelte;
- Kobalte for Solid;
- Headless UI for smaller React and Vue primitive sets;
- Mantine, Chakra, and shadcn/ui for presentational component patterns;
- Tailwind or plain CSS variables for styling primitives.

For native Rust UI, Slint, Iced, egui, Dioxus, Xilem, and GPUI are useful
references, but each carries a different execution and rendering model.

A Tauri adapter should preserve the same core component semantics and
accessibility behavior. It should not create a second widget hierarchy with
different state, identity, or interaction rules. The practical sequence is:

1. Implement and test the core primitives in `stdui_core`.
2. Provide an optional Tauri host that maps runtime events and scene or
   semantic output to the webview where appropriate.
3. Reuse established web accessibility primitives instead of rebuilding
   focus, menus, dialogs, and popovers from scratch.
4. Add common components only after the primitive behavior and semantic tree
   are stable.
