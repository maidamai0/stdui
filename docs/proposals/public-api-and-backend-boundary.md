# Proposal: public declarative API and backend boundary

**Status:** Proposal — discussion draft only; it authorizes no implementation.

## Goal

Design `stdui` first as a pleasant, strongly typed C++ language for describing
UI. Desktop window systems, native-control hosts, software renderers, and GPU
renderers must remain interchangeable consumers of that description.

The public API must not expose a renderer, a platform window handle, a GPU
resource, or a retained native widget as the normal way to express UI.

## The user-facing mental model

A component is a C++ function or value that answers one question:

> Given application data and its component-local state, what should this part
> of the UI look like now?

```cpp
auto welcome(User const& user)
{
    return vstack(
        text("Welcome, " + user.name),
        button("Sign out", [] { /* application action */ })
    );
}
```

The returned value is a **view expression**: a lightweight description, not a
window, widget, graphics object, or persistent UI instance. A framework
runtime later evaluates and reconciles it with persistent composition state.

## Initial expression vocabulary

The first API should stay small. It needs only enough vocabulary to prove
composition and identity:

- leaf content: `text`, `image`, and a basic drawing/shape vocabulary;
- containers: `vstack`, `hstack`, and, only if justified, `overlay`; 
- composition: ordinary C++ components plus a component-expression wrapper;
- dynamic content: `when`/conditional composition and `for_each` with an
  explicit item key;
- configuration: value-returning modifiers such as `.padding(8)`,
  `.background(...)`, `.id(...)`, and `.on_action(...)`.

The spelling is deliberately provisional. What matters is that every operation
returns another value expression, so a component remains ordinary composable
C++.

```cpp
auto user_row(User const& user)
{
    return hstack(
        avatar(user.photo),
        vstack(text(user.name), text(user.email))
    )
    .padding(8)
    .on_action(action::activate, [id = user.id] { open_profile(id); });
}
```

There is no `new QWidget`, no mandatory `Component` base class, and no public
`paint()` virtual method in this model.

### What `overlay` means

`overlay` is a *stacking* container. Its children occupy the same rectangular
area rather than being arranged next to or below one another. The first child
is behind the next child:

```cpp
overlay(
    image(photo),
    text("New").aligned(top_right)
)
```

It can express a badge over an avatar, an icon inside a text field, a loading
indicator over content, a tooltip, or a modal scrim and dialog. It is a generic
layout/composition concept, not a GPU or renderer feature: a headless backend
can represent the same stacking order without drawing pixels.

It is not essential for the first milestone. The first headless API can begin
with only `vstack` and `hstack`; `overlay` should be added only after we define
and test its sizing, alignment, hit-testing, and accessibility rules.

## What an "ordinary C++ component" means

It means application authors make reusable UI with normal C++ declarations,
not with a framework inheritance hierarchy or a special code generator.

The simplest component is a free function:

```cpp
auto user_row(User const& user)
{
    return hstack(avatar(user.photo), text(user.name));
}
```

It accepts regular C++ values and returns a regular C++ view-expression value.
It can call other component functions just as any normal function can.

There are three candidate forms for custom components:

1. **Stateless free function** — best for most components. It accepts data and
   returns expressions, as in `user_row` above.
2. **Function with framework context** — used only when a component needs
   framework-managed local state, actions, or other runtime services.
3. **Value component type** — a C++ struct holding configuration values and a
   `body(...)` member. This may be useful for components with a rich public
   configuration API, but it is not a required base class and should not
   become a Qt-style widget hierarchy.

Later, a separate low-level extension mechanism may let a library author add a
new primitive or a custom layout. That is intentionally different from making
an everyday `UserRow` or `SettingsPanel` component.

## Components, local state, and `component_context`

The leading C++-native option is a component-local state struct:

```cpp
struct SearchState {
    std::string query;
    bool options_visible = false;
};

auto search_panel(component_context& cx, Catalog const& catalog)
{
    auto& state = cx.local_state<SearchState>();

    return vstack(
        text_field("Search", state.query),
        when(state.options_visible, [&] { return search_options(catalog); })
    );
}
```

`Catalog` in this example is ordinary application data—perhaps a class holding
searchable products. It is not part of `stdui`; it could just as easily be a
`Document`, `Settings`, or `std::vector<User>`.

`component_context` is a small framework object that the runtime supplies when
it evaluates a component. The component author does not construct or store it.
At minimum it gives that particular component instance access to its persistent
local state through `cx.local_state<SearchState>()`. Later it may offer narrow,
explicit services such as scheduling an update or obtaining an environment
value. It must not become a hidden grab-bag of platform/renderer APIs.

The runtime, not the temporary `SearchState` expression, owns the persistent
`SearchState` object for one identified `search_panel` instance. The mechanism
must be lazy: reconciliation first establishes an identity and only then
invokes the component body.

This remains an open API detail. The important promise is that ordinary fields
inside `SearchState` do not depend on a fragile call order.

### Alternatives to an explicit context parameter

An explicit `component_context&` is the clearest first option: dependencies
are visible in the function signature, it works in tests, and it does not rely
on thread-local/global state. It may be slightly noisy.

Other possibilities are:

- **Implicit current context:** `local_state<SearchState>()` finds a hidden
  current evaluation context. This is terser, but makes testing, concurrency,
  and calling rules less obvious. It is not recommended as the initial API.
- **Component object receiver:** a framework-created component object exposes
  `this->local_state<T>()`. This resembles a view class, but risks encouraging
  retained-widget thinking and requires a more complicated lifetime model.
- **State passed in from the parent:** the parent owns all state and gives
  children references/bindings. This is excellent for application/domain state
  but inconvenient for purely local UI details.

The leading option is therefore an explicit context for stateful components,
while stateless components remain simple functions.

## How the framework knows how to render a custom component

A normal custom component does not teach every renderer a new drawing command.
It expands into framework primitives the renderer already understands:

```text
user_row(user)
    -> hstack(avatar(...), text(...))
    -> measured layout boxes + semantic/action information
    -> backend-neutral render primitives
    -> headless tree, software renderer, GPU renderer, or native host
```

For example, `user_row` above returns an `hstack`, an avatar, and text. The
layout system knows how to arrange an `hstack`; the text system knows how to
measure text; the rendering layer knows how to represent an image and glyphs.
The backend only receives those lower-level, stable concepts—not a callback to
the application's `user_row` function.

This is the same reason an ordinary Qt composite widget can be built from
existing widgets and layouts without every platform style needing a new
`UserRow` implementation. The declarative version differs only in where the
persistent runtime representation lives.

A genuinely new primitive—say a chart, map, or 3D viewport—needs a separate,
explicit extension contract later. It may supply intrinsic measurement,
semantics, event handling, and a backend-neutral rendering representation.
That is a future design topic, not the default component mechanism.

## Dynamic collections are a distinct expression

Runtime-sized data cannot be represented solely by a C++ tuple of children.
It needs an explicit collection expression:

```cpp
for_each(users,
         [](User const& user) { return user.id; },
         [](User const& user) { return user_row(user); });
```

`for_each` owns enumeration and sibling keys. It must diagnose duplicate keys
in a single evaluation. A row's key is stable only within that list parent;
moving it to another parent is not initially guaranteed to retain local state.

## Non-negotiable backend boundary

```text
application components
        |
        v
typed view expressions              public C++ API
        |
        v
persistent composition + state      framework runtime
        |
        +---- layout representation
        +---- interaction/semantics representation
        +---- render representation
                         |
                         v
              backend-neutral primitives
                         |
                         v
  desktop host / native host / software / GPU backend
```

The render representation is downstream of composition and layout. A backend
may create native controls, issue drawing commands, or translate primitives to
Metal, Vulkan, Direct3D, OpenGL, Skia, or a software rasterizer. None of those
choices changes the component API.

Likewise, layout asks expressions for framework-level intrinsic information;
it does not ask a backend widget for geometry during normal composition.

## What a backend may provide

A backend adapter may provide:

- window/surface creation and platform event delivery;
- device scale, input methods, clipboard, cursor, drag-and-drop, and timers;
- text shaping and font access behind a framework text abstraction;
- an implementation of render primitives; and
- optional native-control interoperation as an explicit escape hatch.

The first backend is in scope for this API discussion: it will be a
**backend-neutral headless backend**. It creates no window and draws no pixels.
Instead, it accepts the same composition/layout/render representations that a
future real backend will accept, and exposes them for deterministic tests and
inspection. This lets us prove one layer at a time:

1. expressions compose correctly;
2. identity and local state behave correctly;
3. layout produces correct rectangles;
4. render primitives are correct;
5. only then, connect a windowing/drawing backend.

## Decisions deferred until their layer is ready

- Whether the first windowed/drawing backend is custom GPU rendering, software
  rendering, or a desktop/native-control host.
- Exact graphics API or abstraction layer, after headless render primitives
  have been validated.
- Threading and render-loop policy.
- The final primitive set and modifier names.
- Precise type-erasure boundaries needed for conditionals and collections.
- The layout proposal/measurement protocol.

## Questions for review

1. Does the proposed public style—ordinary C++ functions returning value
   expressions—feel like the right direction for your users? **Yes: agreed.**
2. The first backend is a backend-neutral headless backend, used to validate
   each framework layer before a real drawing/window backend. **Agreed.**
3. Should stateful components use the explicit `component_context&` approach,
   or should we keep evaluating another spelling before committing?
4. Should `overlay` remain a later, separately designed container as proposed,
   rather than part of the first minimal API?
