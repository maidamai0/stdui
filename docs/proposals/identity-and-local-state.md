# Proposal: identity, reconciliation, and framework-local state

**Status:** Proposal — no implementation decision has been made.

## Question

How should `stdui` decide whether a newly evaluated component expression is
the same persistent component instance as one from the previous evaluation?

This decision controls the lifetime of framework-local state, focus,
animations, event captures, and eventually platform resources. It must be
defined before designing a public state API or committing to a reconciliation
representation.

## Design invariants

The following are non-negotiable consequences of the existing architecture:

1. A C++ expression object is ephemeral; persistent state is not stored in it.
2. State is attached to a component instance in the persistent composition,
   never to an object address, lambda instance, or callback identity.
3. Identity is local to a parent. A key only needs to distinguish siblings.
4. A change in explicit identity resets the state of that instance and its
   descendants.
5. Removing an instance removes its framework-local state at the end of a
   successful reconciliation transaction.
6. Application/domain state is outside this mechanism and must outlive UI
   instances according to normal application ownership rules.

## Proposed semantic model

The runtime maintains a persistent **composition tree**. It is neither the
public expression tree nor the future render tree.

Each composition instance has an identity under its parent:

```text
(parent instance, component kind, child discriminator)
```

- **Component kind** distinguishes incompatible component definitions at the
  same location. Replacing `Counter` with `Editor` resets state, even if their
  child position is unchanged.
- **Child discriminator** is one of:
  - an ordinal structural position for ordinary static children; or
  - an explicit key for a keyed/dynamic child.

An explicit key *replaces* ordinal identity at that sibling position; it is
not global, does not preserve state after moving to another parent, and must
be stable for the semantic item it identifies.

The complete path is an implementation detail. It may be represented as a
tree of instance records, rather than as concatenated strings.

## What counts as a component boundary

A component boundary is created only by an explicit component expression.
Primitive expressions such as text, stack, shape, or modifier participate in
the composition and later in layout/rendering, but do not automatically own a
separate local-state scope.

This avoids accidental state lifetimes based on implementation detail. A
component boundary creates:

- a state-slot namespace;
- a reconciliation identity/type boundary; and
- later, a natural invalidation and diagnostics boundary.

## Local-state model

Within a component instance, local-state calls occupy slots in evaluation
order:

```text
component instance
    |- slot 0: state<int>
    |- slot 1: state<selection>
```

The first evaluation initializes each slot. Later evaluations reuse the slot
only when the component identity and the slot index are unchanged. Changing a
slot's value type is a programming error and must be diagnosed in debug
builds.

### Rule for component authors

Calls that allocate framework-local state must be unconditional and in a
stable order. This is the same restriction used by other declarative systems,
but it must be documented as an explicit `stdui` rule rather than hidden.

```cpp
// Valid: both calls always occupy the same slots.
auto editor() {
    auto text = state(std::string{});
    auto selected = state(false);
    // ...
}

// Invalid: `selected` changes its slot depending on `advanced`.
auto editor(bool advanced) {
    auto text = state(std::string{});
    if (advanced) {
        auto selected = state(false);
        // ...
    }
}
```

This first design does **not** support named state slots, state declared as an
ordinary automatic variable, or calling state from arbitrary helper functions.
Those might be ergonomic extensions later, but should not obscure the lifetime
rule now.

## Reconciliation transaction

One application update produces one transaction:

1. Evaluate the root expression into a declarative expression graph.
2. Reconcile it against the prior composition tree using parent-local identity
   and component kind.
3. Reuse matching composition instances and their state-slot storage.
4. Create unmatched instances and initialize their slots lazily.
5. Mark prior unmatched instances for destruction.
6. Commit the new composition tree, then destroy removed instances.

If evaluation or reconciliation fails, the prior committed composition remains
valid. The required failure behavior for partially initialized new state is
open, but it must be transactional rather than silently corrupting the prior
tree.

## C++ expression representation: alternatives

### A. Eager function execution with an explicit runtime context

```cpp
component(runtime, "counter", [&] {
    auto count = use_state(runtime, 0);
    return vstack(text("Count"), text(*count));
});
```

**Advantages:** small experiment; straightforward control flow.

**Problems:** child structure is evaluated before a reconciler establishes
structural positions; manual string labels become identity; it cannot model
dynamic keyed children correctly without adding a second evaluation protocol.

**Conclusion:** useful only as a disposable spike. It must not become the
production public model.

### B. Lazy, typed component expressions

```cpp
return component<Counter>([&] {
    auto count = state(0);
    return vstack(text("Count"), text(count));
});
```

`component` returns a value expression that contains a body factory. During
reconciliation, the runtime establishes the instance identity and state scope,
then invokes the factory. Static composition can remain strongly typed through
templates and tuples.

**Advantages:** matches the required ordering; supports component boundaries
without manual runtime labels; leaves room for precise diagnostics.

**Costs:** the framework must solve type names/nominal component identity,
lifetimes of captures, and dynamic branching/collections.

**Conclusion:** recommended direction.

### C. Source-location identity inferred at call sites

An API could use `std::source_location::current()` to identify component
calls.

**Advantages:** concise at first glance.

**Problems:** editing or moving source resets state; generated code and
wrappers have unclear identity; it confuses source position with semantic
identity and creates surprising refactoring behavior.

**Conclusion:** do not use source location as persistent identity. It may be
used only for diagnostics.

## Dynamic collections

Static C++ tuples cannot by themselves express a runtime-sized collection. A
separate collection expression is required:

```cpp
for_each(items, [](Item const& item) { return item.id; }, [](Item const& item) {
    return component<Row>([&] { return row(item); });
});
```

The collection expression owns sibling enumeration. It checks key uniqueness
within that parent for one evaluation, matches prior children by key and
component kind, and reports duplicate keys. Reordering keyed items preserves
their state; changing an item key resets it; moving an item under another
parent does not preserve local state in this first model.

Index-based keys are permitted only when insertion, deletion, and reorder do
not need to preserve per-item state. The API documentation should call this
out because it is a semantic, not performance-only, choice.

## State handles and invalidation: deferred decisions

The initial local-state handle should be valid only while its component body
is being evaluated. Retaining it across evaluations must either be impossible
by construction or clearly documented as invalid. Escaping handles creates
hard questions about removed components, threads, and scheduling.

Writing state should eventually schedule a new transaction, but the scheduler,
threading rules, batching behavior, and dependency tracking are intentionally
out of scope for this proposal. A first prototype may request evaluation
explicitly after a write.

## Recommended decision sequence

1. Adopt the semantic model and local-state rule in this proposal.
2. Design a lazy typed component-expression API, including a nominal
   component-kind mechanism suitable for ordinary C++ functions and types.
3. Design static composition and `for_each` together, so keys are represented
   by traversal rather than as a no-op wrapper.
4. Build a **disposable**, headless reconciliation spike only after steps 1–3
   are approved.
5. Use its results to decide the layout protocol.

## Questions requiring approval

1. Should a component kind be supplied explicitly (for example
   `component<Counter>(...)`), inferred from a C++ type, or represented by a
   user-defined component object? The recommendation is an explicit nominal
   type at first, because ordinary function identity is not sufficiently
   portable or semantic.
2. Is the stable evaluation-order rule for local state acceptable, or do you
   prefer a more explicit named-state API even if it is more verbose?
3. Should hiding a subtree destroy its local state immediately, as proposed,
   or should the framework offer an opt-in preservation/cache mechanism later?
4. Should an explicitly keyed item preserve state only within one parent, as
   proposed, or do you want cross-parent moves to preserve it? The latter is
   substantially more complex and should not be a first implementation goal.
