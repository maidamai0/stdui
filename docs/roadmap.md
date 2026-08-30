# stdui Roadmap

## Current milestone

The current implementation has validated:

- typed view expressions and headless reconciliation;
- persistent local state with structural and explicit identity;
- generic layout primitives for stacks, grids, and overlays;
- package installation and CMake `find_package` consumption.

## Next

1. Materialize a persistent layout tree from reconciled expressions.
2. Expose layout configuration through the expression DSL.
3. Add interaction and hit-testing as a specialized subsystem.

## Later

- semantics and accessibility representation;
- rendering primitives and a backend-independent render tree;
- text shaping and measurement abstraction;
- standard components built from the same composition machinery;
- platform windowing and device scale handling.

## Project management

The GitHub project should track this roadmap as `backlog`, `planned`,
`in progress`, and `done` columns. Each item should be a single complete
feature suitable for one review and one commit.
