# Milestone 01 — Composition, identity, and persistent local state

## Goal

Validate the smallest architectural loop that distinguishes `stdui` from an
ordinary immediate-mode C++ UI builder:

```text
view expression -> reconciliation -> persistent runtime state
```

The milestone proves that expressions can be recreated while state survives
when its component identity remains unchanged. It deliberately does **not**
open a window, draw pixels, or introduce a platform dependency.

## Scope

The prototype provides:

- a small typed expression vocabulary: `text`, `vstack`, and `identified`;
- user components expressed as ordinary C++ functions returning expressions;
- an evaluation context that assigns structural identity paths;
- `use_state<T>(initial_value)`, returning a handle to storage owned by a
  persistent runtime;
- a reconciliation pass that preserves state for equal paths and removes
  state for paths that disappear; and
- unit tests that exercise the identity and state-lifetime rules.

`text` is only a convenient leaf expression. It will not yet perform real
text measurement or rendering.

## Explicit non-goals

- No public `View` base class or framework-wide inheritance hierarchy.
- No type erasure in the public expression vocabulary.
- No event system, callbacks, focus, accessibility, animation, layout, or
  render backend.
- No dependency tracking or automatic scheduling of re-evaluation.
- No dynamic collection diff algorithm beyond explicit ids.
- No ABI stability promise.

## Proposed surface

The exact spelling is provisional, but the prototype should support this
shape of code:

```cpp
struct counter_component {};

auto counter()
{
    return stdui::component<counter_component>([](stdui::component_context& cx) {
        auto count = cx.use_state<int>(0);
        return stdui::vstack(
            stdui::text("Count"),
            stdui::text(std::to_string(*count)));
    });
}

auto app(stdui::runtime& runtime)
{
    runtime.reconcile(stdui::vstack(
        stdui::text("Header"),
        counter()));
}
```

The initial API uses lazy typed component expressions. `component<T>` returns a
value expression whose body is evaluated only after reconciliation establishes a
component identity and supplies an explicit `component_context&`. This avoids
thread-local context, macros, coroutines, hidden global state, source-location
identity, and manual string labels as persistent identity. Ergonomic component
APIs can be layered on top once the lifetime semantics are proved.

## Identity model for this milestone

During evaluation, every expression receives a path from:

1. its structural position in the composition tree; and
2. an explicit id, when `identified` is used.

State slots are identified by the component path plus the order of each
`use_state` call within that component evaluation. The call order is a
deliberate initial restriction, matching the normal rule for declarative
local-state APIs: state hooks must be called unconditionally and in a stable
order within a component.

An explicit id replaces the structural segment at its position; it does not
depend on an address, a lambda instance, or a callback object.

## Acceptance criteria

1. Re-evaluating an unchanged expression preserves a `use_state` value.
2. Recreating component and callback objects does not change persistent state
   when the component path is unchanged.
3. Replacing an unidentified sibling changes the affected structural identity.
4. Explicit ids preserve a dynamic item's state across reordering.
5. Removing a component releases its local state after reconciliation.
6. The public expressions remain value types and contain no exposed runtime
   node pointers.

## Exit decision

After this prototype, decide whether the demonstrated expression and identity
model is suitable for the next milestone: proposal-based measurement and a
minimal layout representation. Do not add rendering until that decision is
recorded.
