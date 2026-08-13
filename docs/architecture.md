# stdui Architecture

## 1. Purpose

`stdui` is an experimental modern declarative UI framework for C++. It aims to provide a small, orthogonal foundation from which both framework components and application-specific components can be composed, analogous in spirit to the relationship between language primitives, the C++ standard library, and user-defined abstractions.

The goal is not to reproduce SwiftUI literally in C++. SwiftUI is a useful source of ideas, but `stdui` should use abstractions that are natural and technically appropriate for modern C++.

## 2. High-level architecture

The current model is:

```text
Application
    |
    v
Strongly typed compositional C++ DSL
    |
    v
View expressions
    |
    | reconciliation / materialization
    v
Persistent UI representation
    |
    +----> Layout representation
    |
    +----> Interaction representation
    |
    +----> Semantics / accessibility representation
    |
    +----> Render representation
                  |
                  v
         Rendering primitives
                  |
                  v
             Render backend
```

The most important boundaries are:

- A view expression is not the persistent UI.
- A persistent UI representation is not the render representation.
- UI primitives are not rendering primitives.
- Component identity is not C++ object identity.
- A state handle is not the persistent state storage itself.

## 3. Public programming model

The public API should be a strongly typed compositional DSL. Users should be able to write code conceptually like:

```cpp
auto make_counter()
{
    return vstack(
        text("Count"),
        text(count),
        button("+", increment)
    );
}
```

The exact syntax is intentionally not finalized.

The expression returned by such a function is a view description. It may be represented using C++ types such as composed/template expressions, but the runtime persistent representation is an implementation detail.

We should not make a runtime `ui_node`/DOM-like object the primary API if doing so would force users to manually construct implementation-level trees.

## 4. Components

Framework components and user-defined components should use the same composition machinery.

Examples such as `Button`, `Table`, `List`, `Tree`, etc. may be shipped as standard components, but they should not be architecturally privileged widget classes. A standard component should preferably be implementable from the same primitives and mechanisms available to users.

Components are primarily compositional C++ functions/expressions. A C++ type may be used where a component needs richer configuration, state, or behavior. A framework-wide inheritance-based `Component` base class is not fundamental.

## 5. UI primitives vs rendering primitives

There are two distinct primitive layers.

### UI primitives

These are compositional framework-level concepts that participate in layout, interaction, state, semantics, or other UI behavior.

Examples may eventually include stacks, containers, scrolling, text, images, input, focus, gestures, and shapes. The final primitive set is not yet fixed.

### Rendering primitives

These are backend-independent drawing operations or objects such as:

- paths
- rectangles and rounded rectangles
- text/glyph runs
- images
- clipping
- transforms

A UI primitive does not have to map one-to-one to a rendering primitive. A `Button`, for example, should normally be a composition of UI behavior and visual primitives rather than a special renderer operation.

## 6. Specialized subsystems

The UI description participates in several specialized subsystems rather than becoming a monolithic `Widget` object:

```text
UI description
    |
    +---- Layout
    |
    +---- Interaction / events / focus
    |
    +---- Semantics / accessibility
    |
    +---- Rendering
```

The exact internal representations of these subsystems are open to implementation research.

## 7. State

Both application-owned state and framework-managed local UI state are supported.

Application-owned state represents application/domain data and should be observable by the UI system.

Framework-managed local state is associated with persistent UI identity. A conceptual API might look like:

```cpp
auto count = state(0);
```

but the `state<T>` object should be understood as a handle/reference to persistent framework-managed storage, not as ordinary local C++ storage.

The persistent state must survive repeated evaluation of view expressions.

## 8. Identity and reconciliation

View expressions may be recreated whenever application state changes. The framework therefore needs reconciliation between the previous and current expressions.

Identity should be structural by default, with explicit identity available when semantic identity differs from structural position.

For example, dynamic collections may need:

```cpp
row(item).id(item.id)
```

The exact identity model is not finalized.

Transient C++ object addresses, lambda identity, or callback object identity must not become the basis for persistent UI identity.

A callback may be recreated while the persistent button node remains the same; the callback is a current property of the view expression, not the identity of the button.

## 9. Layout

Layout is generic. Layout algorithms should not contain concrete-type branches such as `if child is Text`.

Elements provide intrinsic sizing information, while layout algorithms decide how children are proposed space and placed.

The working conceptual model is:

```text
Constraints / proposal
        |
        v
     measure
        |
        v
       Size
        |
        v
      arrange
        |
        v
     Geometry
```

We expect proposal-based sizing, where a dimension can be bounded or unspecified/unbounded.

The final public/internal layout protocol is not finalized. In particular, `measure()` and `arrange()` should be operations of the layout system rather than forcing every user-facing component into a traditional virtual `Widget` interface.

Standard layouts such as `HStack`, `VStack`, `Grid`, etc. should be implementations of generic layout mechanisms, not fundamental special cases.

## 10. Geometry and coordinates

UI geometry uses logical/device-independent units. Platform/backend scale factors convert logical geometry to physical pixels.

Conceptually:

```text
logical UI coordinates
        |
        v
platform scale factor
        |
        v
physical pixels
```

The core UI layer should not depend on platform-specific coordinate units.

## 11. Text

Text is treated as a specialized intrinsic-sizing and shaping subsystem, but it is not a special case in the generic layout engine.

The intended architecture is:

```text
Text API
   |
   v
Text abstraction
   |
   v
Shaping / line breaking / measurement
   |
   v
Common glyph representation
   |
   v
Rendering system
```

A hybrid implementation is preferred: platform text engines may be used where appropriate (for example CoreText or DirectWrite), while a common implementation path can use technologies such as FreeType/HarfBuzz. The UI/layout layers should depend on a framework-level abstraction rather than directly on a platform text API.

## 12. Rendering

Rendering is downstream of layout. The render representation should consume geometry and produce backend-independent rendering primitives.

The intended separation is:

```text
UI description
    |
    v
Layout
    |
    v
Render representation
    |
    v
Rendering primitives
    |
    v
Backend
```

Potential backends include CPU/2D rendering libraries and platform GPU APIs. Blend2D, Metal, Direct2D, and other technologies are possibilities, not current commitments.

## 13. Persistent representation

The runtime may contain a persistent UI/node representation similar in spirit to a tree or graph. This representation should be hidden behind the typed public DSL.

A typical update is conceptually:

```text
old view expression       new view expression
        |                         |
        +-----------+-------------+
                    |
              reconciliation
                    |
                    v
          persistent UI nodes
                    |
                    v
                 layout
                    |
                    v
                rendering
```

The implementation should preserve persistent state where identity is unchanged rather than rebuilding the entire runtime state on every evaluation.

## 14. Current architectural goal

The framework should be small and orthogonal at the bottom and rich through composition at the top:

```text
Application components
        |
Standard framework components
        |
UI primitives
        |
Specialized subsystem machinery
        |
Rendering primitives
        |
Platform/backend
```

The fundamental design question is not "which widgets should be built in?" but "which minimal, orthogonal capabilities allow arbitrary useful UI components to be built?"

## 15. What is deliberately not fixed yet

The following are open design questions and should be investigated before committing to implementation:

- Exact public DSL syntax and naming.
- Whether/how a public `View` concept should exist.
- Exact C++ template/type-erasure strategy.
- Exact view-expression representation.
- Reconciliation algorithm and identity representation.
- State storage and dependency tracking.
- Layout protocol and proposal representation.
- Event propagation and gesture model.
- Focus model.
- Semantics/accessibility representation.
- Render tree structure and invalidation.
- Text abstraction details and backend strategy.
- Platform windowing abstraction.
- Threading model.
- Animation/timing model.
- Resource lifetime model.

These should be solved incrementally through small prototypes and documented design decisions rather than assumed prematurely.
