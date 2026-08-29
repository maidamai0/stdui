# Milestone 01 proposal: headless expression composition

**Status:** Proposal — design work only. No implementation starts until this is
reviewed.

## Purpose

Before state, identity, layout, windows, or rendering, prove the smallest
useful promise of the public API:

> An application can write normal C++ functions that compose UI expressions,
> and the framework can inspect the resulting structure without a backend.

This is deliberately smaller than a usable UI toolkit. It validates the
language shape first, where mistakes are cheapest to correct.

## What this milestone includes

### Three expression constructors

```cpp
text("Hello")
vstack(child_a, child_b, ...)
hstack(child_a, child_b, ...)
```

- `text` represents an owned UTF-8 string of display text. It has no font,
  wrapping, measurement, or drawing behavior yet.
- `vstack` means “these children are vertically ordered.”
- `hstack` means “these children are horizontally ordered.”

At this stage, stack direction is semantic structure only. It produces no
pixel geometry; layout is the next separately designed layer.

### Stateless custom components are ordinary functions

```cpp
auto person_row(Person const& person)
{
    return hstack(
        text(person.initials),
        vstack(text(person.name), text(person.email))
    );
}

auto screen(Person const& person)
{
    return vstack(text("People"), person_row(person));
}
```

`person_row` is not registered, allocated, inherited from a base class, or
rendered by a special backend function. It is simply a C++ function that
returns another expression. For this milestone, calling it expands its result
into its parent expression.

Stateful components are intentionally deferred. They need lazy component
boundaries and persistent identity; adding them before the basic expression
model works would hide the design question we need to answer next.

## The headless inspector

An inspector is a small **test-only reader** of a UI expression. Think of it as
the equivalent of asking a Qt layout/widget tree to print its hierarchy for a
unit test. It does not draw the UI and it is not a production rendering
backend.

For this milestone, the inspector turns an expression into a simple immutable
inspection tree:

```text
vstack
  text: "People"
  hstack
    text: "AY"
    vstack
      text: "Ada Yoon"
      text: "ada@example.test"
```

It creates no window, native widget, image, or GPU resource. Its job is to
make composition observable and testable. A test can compare the expected tree
with the inspected result and answer questions such as “did this custom
component produce an `hstack` with these two children in this order?”

The public expression has no knowledge of the inspector, and the inspector
only understands framework expression kinds (`text`, `vstack`, `hstack`). A
future layout/render pipeline will consume the same composed expression, not
an application-specific `person_row` object.

## When rectangles and geometry arrive

`rectangle` is an important primitive, but it needs a few decisions that this
first composition-only slice deliberately avoids: its width/height policy,
fill/stroke model, corner radius, clipping, intrinsic size, and how it
participates in layout and hit testing.

It belongs in the first **layout and render-primitives milestone**, after we
have proved expression composition and persistent component identity. That
milestone will introduce framework-level geometry types such as `size`,
`point`, and `rect`, plus a basic `rectangle`/`box` expression. The headless
output will then include computed rectangles and draw-primitives, for example:

```text
draw_rectangle bounds=(0, 0, 120, 40) fill=blue
```

This does not postpone geometry indefinitely. It ensures we first decide what
“120 by 40” means when its parent has less space, more space, a scale factor,
or a child with its own intrinsic size.

## What this milestone proves

1. The proposed C++ call syntax composes naturally.
2. Expressions own their text data safely; no borrowed temporary string is
   retained.
3. Child ordering is preserved exactly.
4. User component functions are indistinguishable from framework composition
   at the consumer boundary.
5. The framework can introduce an internal representation for inspection
   without forcing a DOM/node base class into the public component API.

## Explicit non-goals

- No local state or `component_context`.
- No reconciliation, keys, dynamic collections, or conditionals.
- No modifiers, events, actions, focus, or accessibility.
- No sizes, positions, constraints, fonts, text shaping, or drawing.
- No `overlay`.
- No windowing or graphics backend.

## Test tooling

The project will use **doctest**, not Catch2, for unit tests. When
implementation begins, CMake will obtain and build it through
`ExternalProject_Add`, as requested. The first tests will cover the headless
inspector's output; they do not require a platform window or graphics driver.

## Example test, conceptually

```cpp
auto actual = inspect(screen({"Ada Yoon", "AY", "ada@example.test"}));

expect_tree(actual,
    vstack_node(
        text_node("People"),
        hstack_node(
            text_node("AY"),
            vstack_node(
                text_node("Ada Yoon"),
                text_node("ada@example.test")))));
```

The exact test helpers are not being designed yet. This shows the useful
property: a user can verify the public composition model without a platform
window or a screenshot test.

## Exit criteria

This milestone is complete only when the API shape and headless inspection
tests make the examples above unsurprising to both a C++ UI-framework user and
someone with a Qt/wxWidgets background.

Only then do we design Milestone 02: lazy component boundaries, local state,
identity, and reconciliation. Layout follows after that.

## Question for review

Is this sufficiently small as the first working vertical slice, or do you
want the first slice to include one additional concept (for example a simple
modifier) before we proceed?
