# stdui Design Decisions

This document records decisions made during the initial architecture discussion. These are intentionally separated from implementation details. A decision can later be revised, but changes should be explicit.

## Status vocabulary

- **Decided:** current architectural direction should be followed unless new evidence requires a deliberate change.
- **Working hypothesis:** preferred direction, but should be validated by prototypes/research.
- **Open:** intentionally unresolved.

## Decisions

### D001 — Public programming model: strongly typed compositional DSL

**Status:** Decided

The public API should be a strongly typed C++ compositional DSL. We should not make a runtime `ui_node` tree the primary programming abstraction, and we should not simply reproduce SwiftUI's type system or syntax.

The C++ type system may encode static composition information, while runtime values/state remain dynamic.

### D002 — Persistent runtime representation is separate from view expressions

**Status:** Decided

A view expression is a description of UI, not the persistent UI itself. View expressions may be recreated and reconciled against persistent runtime state.

This distinction allows state, identity, geometry, and other long-lived data to survive expression recreation.

### D003 — UI primitives and rendering primitives are separate layers

**Status:** Decided

UI primitives participate in composition and UI semantics. Rendering primitives describe backend-independent drawing. They do not need one-to-one correspondence.

### D004 — Framework components and user components share the same composition machinery

**Status:** Decided

Standard components such as Button, Table, List, and Tree are batteries provided by the framework, not architecturally privileged widget classes. They should use the same mechanisms available to users to the greatest practical extent.

### D005 — Specialized subsystem model rather than a monolithic Widget

**Status:** Decided

Do not build the core around a giant object with virtual layout, paint, event, and accessibility methods. Layout, interaction, semantics, and rendering are specialized interpretations/representations of the UI description.

### D006 — State has application-owned and framework-managed forms

**Status:** Decided

Application/domain state and UI-local state are both first-class concepts.

Framework-local state is persistent storage associated with UI identity. A state handle created during expression evaluation must not be confused with ordinary local C++ storage.

### D007 — Structural identity by default, explicit identity when needed

**Status:** Decided

The framework should derive identity from composition structure in common cases and allow explicit semantic identity for dynamic collections or other cases where structural position is insufficient.

Transient C++ object addresses and callback/lambda object identity must not determine persistent UI identity.

### D008 — Components are primarily functions/expressions; no mandatory Component base class

**Status:** Decided

Simple components should be naturally expressible as C++ functions returning composed view expressions. Richer components may be represented by C++ types. A framework-wide inheritance hierarchy is not fundamental.

### D009 — Generic layout with intrinsic sizing

**Status:** Decided

The layout engine should be generic. Text, images, and other elements can provide specialized intrinsic sizing information, but generic layout algorithms should not contain concrete-type special cases.

### D010 — Proposal-based layout

**Status:** Working hypothesis

Measurement should be driven by constraints/proposals, with dimensions potentially bounded or unspecified/unbounded. Arrangement assigns final geometry.

The exact layout protocol is intentionally unresolved.

### D011 — Logical/device-independent geometry

**Status:** Decided

UI geometry is expressed in logical units. Platform scale factors convert logical geometry to physical pixels at the rendering/platform boundary.

### D012 — Hybrid text stack

**Status:** Working hypothesis

Expose a framework-level text abstraction. Use platform text engines where advantageous and/or a common stack such as FreeType/HarfBuzz. Produce a common glyph representation for downstream layout/rendering.

### D013 — Architecture-first implementation

**Status:** Decided

Do not start with a large widget library or elaborate template machinery. Validate foundational concepts with small prototypes and preserve the distinction between decisions, hypotheses, and open questions.

## Important consequences

These decisions intentionally imply that the framework is not a traditional retained-mode widget toolkit with a class hierarchy like:

```text
Widget
  |
  +-- Button
  +-- TextBox
  +-- List
  +-- Table
```

Instead, the intended direction is:

```text
Typed composition
      |
      v
View expression
      |
      v
Persistent representation
   /       |        \
layout  interaction  rendering
                         |
                         v
                rendering primitives
```

The standard component library sits on top of the same composition machinery used by applications.

## Known risks to investigate

1. C++ template-heavy DSLs can produce poor diagnostics and excessive compile times.
2. Runtime reconciliation can become expensive or overly complex.
3. Identity semantics can become surprising around conditional composition and dynamic collections.
4. Persistent UI state needs clear ownership and lifetime rules.
5. Separating subsystem representations can increase implementation complexity and synchronization/invalidation cost.
6. Text shaping, measurement, and platform interoperability are intrinsically complex.
7. A too-small primitive set may make common components cumbersome; a too-large set recreates a widget toolkit.
8. Premature optimization of allocations/type erasure could make the architecture harder to evolve.

These risks should be evaluated experimentally.
