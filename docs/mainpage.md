# stdui {#mainpage}

`stdui` is an experimental modern declarative UI framework for C++.

The public API is header-only and targets C++20. The framework separates view
expressions from persistent runtime state, keeps layout generic, and treats
rendering, interaction, semantics, and text as specialized subsystems.

> **Warning:** stdui is under heavy development and is not ready for production use.

## Documentation

- @subpage getting_started - Installation and first steps
- @subpage tutorials - Step-by-step guides
- @subpage architecture - Design philosophy and system overview
- @subpage design_decisions - Architectural decisions and rationale
- @subpage roadmap - Development phases and timeline

## API Areas

- Expression DSL and components: @ref stdui::component
- Reconciliation and local state: @ref stdui::runtime
- Generic layout primitives: @ref stdui::layout_hstack, @ref stdui::layout_vstack,
  @ref stdui::layout_grid, @ref stdui::layout_overlay
- Logical geometry: @ref stdui::rect

## Quick Links

- [GitHub Repository](https://github.com/maidamai0/stdui)
- [CI Status](https://github.com/maidamai0/stdui/actions)

See the repository `README.md` for build, install, test, and coverage commands.
