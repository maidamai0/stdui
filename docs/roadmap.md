# stdui Roadmap

The roadmap is organized into phases. Near-term phases are detailed into
review-sized items; later phases are coarse epics that decompose when work
reaches them. Each phase maps to a GitHub milestone, and the GitHub project
tracks the items on this roadmap.

## Phase 1 — Foundation (validated)

- typed view expressions and headless reconciliation;
- persistent local state with structural and explicit identity;
- generic layout primitives for stacks, grids, and overlays;
- persistent layout tree materialized from reconciled expressions;
- package installation and CMake `find_package` consumption;
- initial GitHub release and deployed API docs.

## Phase 2 — Layout and measurement

Goal: layout becomes configurable through the DSL and covers the primitives
a real screen needs.

- Expose layout configuration through the expression DSL.
- Add grid expressions so the generic grid layout is reachable from the DSL.
- Add a scroll container primitive (viewport, clipped content, scroll offsets).
- Add a text measurement bridge to a platform text engine (measure-only).

`measure()` reports a node's layout extent while `layout_box::bounds` reports
the rect its content occupies within the assigned frame. Both passes run the
same deterministic layout algorithms, so arranged frames agree with measured
child sizes; the two values differ only where flex leaves unclaimed space.

## Phase 3 — Interaction and state

Goal: the UI reacts to input and to application-owned data.

- Add hit-testing over the materialized layout tree.
- Add pointer and keyboard events with propagation rules.
- Add a focus model.
- Add gesture recognition (tap, press, pan).
- Add observable application state with dependency tracking and invalidation.

## Phase 4 — Rendering

Goal: prove the backend pluggability claim. An interface is only real once
two implementations exist.

- Introduce a render tree and backend-independent rendering primitives.
- Define the backend abstraction interface.
- Implement a reference CPU rasterizer backend.
- Implement a second platform backend rendering the same demo.
- Add frame scheduling and vsync.

Acceptance: one demo application renders identically through both backends.

## Phase 5 — Batteries

Goal: standard components built from the same composition machinery.

- Design the framework text abstraction (hybrid platform/common shaping).
- Add semantics and accessibility representation.
- Add environment values and theming (appearance, locale, scale).
- Add animation and transitions.
- Add a standard component catalog in increments (button, toggle, slider,
  text field, picker, list, table, navigation, dialogs).

## Phase 6 — Platform hardening

- Add platform windowing, application lifecycle, and device scale handling.
- Add accessibility platform bridges (screen reader integration).
- Add IME/text input integration.
- Add clipboard and drag-and-drop.

## Project management

The GitHub project tracks this roadmap in `Todo`, `In Progress`, and `Done`
columns. Each item is a GitHub issue and a single complete feature suitable
for one review and one commit.
