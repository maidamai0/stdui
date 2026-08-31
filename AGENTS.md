# AGENTS.md

## Project

`stdui` is an experimental modern declarative UI framework for C++. The architectural goal is a compositional, value-oriented UI system with a strongly typed C++ public API, persistent runtime state, generic layout, specialized subsystems, and backend-independent rendering.

## Architectural principles

- Prefer composition over inheritance. A framework-wide `Component` base class is not fundamental.
- The public API should be a strongly typed compositional C++ DSL. Do not make the runtime node representation the primary user-facing abstraction.
- View expressions are descriptions; they are not the persistent UI itself.
- Runtime/persistent UI representation is an implementation detail and may be reconciled from view expressions.
- Framework-provided components and user-defined components should use the same composition machinery. Standard components are batteries, not privileged widget types.
- Keep UI primitives distinct from rendering primitives.
- Keep layout, interaction, semantics/accessibility, and rendering as specialized subsystems rather than putting all behavior into a monolithic Widget object.
- Support both application-owned state and framework-managed local UI state.
- Use structural identity by default and explicit identity where semantic identity is required.
- Do not use transient C++ object identity or callback/lambda identity as persistent UI identity.
- Use logical/device-independent geometry units in the UI layer; platform scale converts to physical pixels.
- Layout should be generic. Elements provide intrinsic sizing information; layout algorithms should not contain concrete-type special cases.
- Text should have a framework-independent abstraction with a hybrid platform/common shaping implementation and a common glyph representation.

## Engineering principles

- This is an architecture-first project. Do not prematurely commit to a large widget hierarchy or template-heavy abstraction without documenting the reason.
- Prefer small, orthogonal primitives and explicit subsystem boundaries.
- Keep current architectural decisions distinguishable from open questions; record significant changes in `docs/design-decisions.md`.
- Favor modern, idiomatic C++ and clear APIs over reproducing SwiftUI syntax literally.
- Avoid unnecessary allocations and runtime polymorphism, but do not sacrifice architectural clarity merely for micro-optimization.
- Tests should accompany foundational behavior.
- Keep platform/rendering dependencies out of the core UI model wherever practical.
- Code comments describe current code behavior, not roadmap or future plans. Record milestone and future-work notes in `docs/`, never leave them in headers or implementation files.
- Workflow: make changes, wait for user approval, commit the approved changes, then continue with new uncommitted work. Repeat this loop.
- One approval authorizes only the changes present when that approval is given. Never use an earlier approval to commit later work.
- Commit messages follow [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/).
- Pull request titles use a conventional commit prefix because pull requests are squash-merged.

## Current status

The repository is at the architecture/design stage. Before implementing substantial framework code, read `docs/architecture.md` and `docs/design-decisions.md`.
