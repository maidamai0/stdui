# Detailed Step-by-Step Plan: Building stdui - A Modern UI Framework in C++

## Executive Summary

This plan builds upon your existing Phase 1 foundation (typed expressions, reconciliation, layout primitives, and persistent state) and provides a concrete, testable roadmap to a production-ready SwiftUI-inspired UI framework in C++.

**Current Status:** Phase 1 ~85% complete (layout config DSL integration in progress)

**Timeline:** 18-24 months to feature-complete v1.0

**Key Differentiators:**
- Strongly typed C++ DSL (not runtime widget trees)
- Composition over inheritance
- Persistent UI representation separate from view expressions
- Generic, extensible layout system
- Backend-agnostic rendering

---

## Phase 1: Foundation (Current - Completing)

**Goal:** Validate the core architectural boundaries before adding rendering/backend complexity.

### ✅ Completed Items
- [x] Typed view expression DSL (`text`, `vstack`, `hstack`, `overlay`, `flex`, `grow`, `fill`)
- [x] Expression concepts and composition (`view_expression`, `layout_element`)
- [x] Persistent layout tree materialization
- [x] Generic layout algorithms (stacks, grids, overlays)
- [x] Proposal-based layout system (bounded/unbounded constraints)
- [x] Structural and explicit identity (`identified`, `dynamic_list`)
- [x] Component model with deferred evaluation
- [x] CMake packaging and installation
- [x] CI/CD pipeline with coverage

### 🔄 In Progress (Complete This Sprint)
- [ ] **1.1:** Expose all layout options through DSL (current branch: feat/layout-config-dsl)
  - **Test:** Create stacks with padding, spacing, alignment, direction via DSL
  - **Acceptance:** `vstack({.spacing=10, .padding={5,5,5,5}}, child1, child2)` compiles and applies options
  
- [ ] **1.2:** Add grid expressions to DSL
  - **Implementation:** Create `grid_expression<T...>` wrapper
  - **Test:** `grid({.columns=3}, item1, item2, ...item9)` lays out 3x3 grid
  - **File:** `include/stdui/expressions.hpp` (add grid types)

- [ ] **1.3:** Framework-managed local state
  - **Implementation:** `state<T>` handle with persistent storage keyed by component identity
  - **API:** `auto count = ctx.state(0); count.set(count.get() + 1);`
  - **Test:** Counter component maintains state across expression re-evaluation
  - **Files:** New `include/stdui/state.hpp`, extend `runtime.hpp`

### Deliverable: Phase 1 Completion
- **Release:** v0.1.0 - "Headless Foundation"
- **Documentation:** Architecture validation article
- **Demo:** Headless test showing counter with persistent state surviving reconciliation

---

## Phase 2: Measurement & Text Integration (Months 2-4)

**Goal:** Complete the layout system with real content measurement.

### 2.1: Text Measurement Bridge (Month 2)
**Testable Milestone:** Text nodes report accurate intrinsic size

#### Step 2.1.1: Define text abstraction interface
- **File:** `include/stdui/text_bridge.hpp`
- **Interface:**
  ```cpp
  struct text_metrics {
    size intrinsic_size;
    double baseline;
    double line_height;
  };
  
  class text_engine {
    virtual auto measure(std::string_view text, 
                         text_attributes const& attrs,
                         double max_width) -> text_metrics = 0;
  };
  ```
- **Test:** Mock text engine returns predictable metrics

#### Step 2.1.2: Platform text engine implementations
- **macOS:** CoreText wrapper in `src/platform/macos/core_text_engine.mm`
- **Linux:** FreeType/HarfBuzz in `src/platform/linux/harfbuzz_engine.cpp`
- **Windows:** DirectWrite in `src/platform/windows/directwrite_engine.cpp`
- **Test:** Each platform measures "Hello World" in Arial 12pt to ~60x15 logical units (±10%)

#### Step 2.1.3: Integrate text measurement into layout
- **File:** Extend `text_expression` in `expressions.hpp`
- **Add:** `measure()` method that delegates to text engine
- **Test:** `text("Multi\nline").measure(proposal::bounded(50, unbounded))` wraps at 50 units

#### Step 2.1.4: Text attributes and styling
- **File:** `include/stdui/text_attributes.hpp`
- **Attributes:** font family, size, weight, style, color, line spacing
- **API:** `text("Bold", {.weight=font_weight::bold, .size=16})`
- **Test:** Bold text measures wider than regular text

### 2.2: Scroll Container Primitive (Month 3)
**Testable Milestone:** Scrollable content with viewport clipping

#### Step 2.2.1: Scroll geometry model
- **File:** `include/stdui/scroll.hpp`
- **Concepts:**
  - `viewport`: visible rect
  - `content_size`: total scrollable extent
  - `scroll_offset`: current position (x, y)
- **Test:** Content 1000x2000, viewport 300x400, offset (100, 500) → visible rect is {100,500,300,400}

#### Step 2.2.2: Scroll layout algorithm
- **Behavior:** Propose unbounded to content, then clip to viewport
- **Implementation:** `measure_scroll()` and `arrange_scroll()` in `layout.hpp`
- **Test:** Child measures at intrinsic 500x800, viewport is 200x300 → extent reports 200x300 to parent

#### Step 2.2.3: Scroll expression and DSL
- **API:** `scroll(vstack(item1, item2, ...item100))`
- **State:** Scroll offset as framework-managed state
- **Test:** Scroll expression materializes with correct viewport bounds

#### Step 2.2.4: Scroll offset control
- **API:** `ctx.scroll_to(offset)` or `scroll_expression.offset(x, y)`
- **Test:** Setting offset updates materialized layout

### 2.3: Enhanced Layout Features (Month 4)
**Testable Milestone:** Complex layouts match design specs

#### Step 2.3.1: Alignment guides and custom alignment
- **File:** `include/stdui/alignment.hpp`
- **API:** Define custom alignment anchors (e.g., text baseline)
- **Test:** Align multiple text elements by baseline across stack

#### Step 2.3.2: Aspect ratio constraints
- **API:** `aspect_ratio(16.0/9.0, image_view)`
- **Test:** Image with 16:9 ratio in 400pt width measures at 225pt height

#### Step 2.3.3: Size modifiers
- **API:** `frame(child, {.width=100, .height=50})` - fixed size
- **API:** `frame(child, {.max_width=200})` - constrained size
- **Test:** Fixed frame overrides child's intrinsic size

### Deliverable: Phase 2 Completion
- **Release:** v0.2.0 - "Layout Complete"
- **Documentation:** Layout system guide with examples
- **Demo:** Headless layout of article with text, images, scrolling

---

## Phase 3: Interaction & State Management (Months 5-8)

**Goal:** UI responds to input and application data changes.

### 3.1: Hit Testing (Month 5)
**Testable Milestone:** Pointer events reach correct UI elements

#### Step 3.1.1: Hit test infrastructure
- **File:** `include/stdui/hit_test.hpp`
- **Algorithm:** Traverse materialized layout tree, test point-in-rect
- **Test:** Point (50, 50) hits second button in vstack at y=40-80

#### Step 3.1.2: Z-order and overlay hit testing
- **Behavior:** Test front-to-back, first hit wins
- **Test:** Overlapping buttons, click in overlap region hits front button

#### Step 3.1.3: Hit test metadata
- **Data:** Node path, local coordinates, element type
- **Test:** Hit test returns full path to leaf element

### 3.2: Event System (Month 6)
**Testable Milestone:** Events propagate and trigger handlers

#### Step 3.2.1: Event types and representation
- **File:** `include/stdui/events.hpp`
- **Events:** pointer_down, pointer_up, pointer_move, key_down, key_up, scroll
- **Data:** position, button/key, modifiers, timestamp
- **Test:** Create and inspect event objects

#### Step 3.2.2: Event propagation model
- **Strategy:** Bubbling from target to root (like DOM)
- **Handlers:** Can consume event to stop propagation
- **Test:** Click button in nested vstack, both stack and button handlers fire

#### Step 3.2.3: Event handler attachment
- **API:** `on_click(button_expr, [](){ /* handle */ })`
- **Storage:** Handlers stored in materialized tree
- **Test:** Click fires attached lambda

#### Step 3.2.4: Gesture recognition foundation
- **File:** `include/stdui/gestures.hpp`
- **Recognizers:** TapRecognizer, LongPressRecognizer, PanRecognizer
- **State machine:** Track pointer down → move → up sequence
- **Test:** Tap recognizer fires only on down+up within threshold

### 3.3: Focus Management (Month 7)
**Testable Milestone:** Keyboard navigation works correctly

#### Step 3.3.1: Focus model
- **Concepts:** 
  - `focusable` - element can receive focus
  - `focus_scope` - subtree with focus containment
  - Global focus path
- **File:** `include/stdui/focus.hpp`
- **Test:** Set focus to element, focus path reflects it

#### Step 3.3.2: Focus navigation
- **Tab order:** Depth-first traversal of focusable elements
- **API:** `focusable(input_field)`, `next_focus()`, `prev_focus()`
- **Test:** Tab moves through 3 input fields in order

#### Step 3.3.3: Focus events and visual state
- **Events:** focus_gained, focus_lost
- **State:** Track per-element focus state for rendering
- **Test:** Focus events fire on navigation

### 3.4: Observable Application State (Month 8)
**Testable Milestone:** UI updates when app state changes

#### Step 3.4.1: Observable property model
- **File:** `include/stdui/observable.hpp`
- **API:** 
  ```cpp
  observable<int> counter{0};
  counter.observe([](int val){ /* notify */ });
  counter.set(5); // triggers observers
  ```
- **Test:** Setting observable value fires observers

#### Step 3.4.2: Dependency tracking
- **Mechanism:** Track observables read during expression evaluation
- **Implementation:** Thread-local "current tracking context"
- **Test:** Expression reading observable automatically subscribes

#### Step 3.4.3: Invalidation and reconciliation trigger
- **Flow:** Observable change → mark expressions dirty → trigger reconciliation
- **Test:** Changing counter observable re-evaluates dependent expression

#### Step 3.4.4: Binding syntax
- **API:** `text_field(binding(&model.name))`
- **Behavior:** Two-way binding between UI and model
- **Test:** Typing in field updates model; model change updates field

### Deliverable: Phase 3 Completion
- **Release:** v0.3.0 - "Interactive Foundation"
- **Documentation:** Event handling and state management guide
- **Demo:** Counter app (headless test with simulated events)

---

## Phase 4: Rendering System (Months 9-12)

**Goal:** Prove backend pluggability with two working implementations.

### 4.1: Render Tree Architecture (Month 9)
**Testable Milestone:** Render tree accurately represents visual output

#### Step 4.1.1: Backend-independent rendering primitives
- **File:** `include/stdui/render_primitives.hpp`
- **Primitives:**
  ```cpp
  struct fill_rect { rect bounds; color fill; };
  struct stroke_rect { rect bounds; color stroke; double width; };
  struct fill_path { path_data path; color fill; };
  struct draw_text { point origin; glyph_run glyphs; color fill; };
  struct draw_image { rect bounds; image_handle image; };
  struct clip { rect bounds; render_list children; };
  struct transform { affine_transform matrix; render_list children; };
  ```
- **Test:** Construct render primitives programmatically

#### Step 4.1.2: Render tree generation
- **File:** `include/stdui/render_tree.hpp`
- **Process:** Walk materialized layout tree → emit render primitives
- **Test:** vstack with 2 colored rects → render tree has 2 fill_rect primitives

#### Step 4.1.3: Render tree invalidation
- **Strategy:** Track dirty regions, rebuild subtrees
- **Optimization:** Reuse unchanged subtrees
- **Test:** Changing one leaf re-renders only that branch

### 4.2: Backend Abstraction (Month 10)
**Testable Milestone:** Render backend interface is well-defined

#### Step 4.2.1: Backend interface definition
- **File:** `include/stdui/render_backend.hpp`
- **Interface:**
  ```cpp
  class render_backend {
    virtual void begin_frame() = 0;
    virtual void fill_rect(rect const&, color const&) = 0;
    virtual void stroke_rect(rect const&, color const&, double width) = 0;
    virtual void draw_text(point const&, glyph_run const&, color const&) = 0;
    virtual void clip_rect(rect const&) = 0;
    virtual void restore_clip() = 0;
    virtual void end_frame() = 0;
  };
  ```
- **Test:** Mock backend records calls

#### Step 4.2.2: Color and image representation
- **File:** `include/stdui/color.hpp`, `include/stdui/image.hpp`
- **Color:** RGBA float or sRGB byte representation
- **Image:** Handle + metadata (size, format)
- **Test:** Create colors, verify RGBA values

### 4.3: Reference CPU Backend (Month 11)
**Testable Milestone:** First visual output

#### Step 4.3.1: Choose CPU rasterizer
- **Options:** Blend2D, Cairo, AGG, Skia CPU
- **Recommendation:** Blend2D (modern, C++, permissive license)
- **Integration:** `src/backends/blend2d/blend2d_backend.cpp`
- **Test:** Render red rect, verify output pixels

#### Step 4.3.2: Implement all primitive operations
- **Coverage:** Rects, paths, text, images, clipping, transforms
- **Test suite:** Render each primitive, compare to reference image (pixel diff <1%)

#### Step 4.3.3: Frame scheduling
- **File:** `include/stdui/frame_scheduler.hpp`
- **Strategy:** vsync-aligned updates (60fps target)
- **Test:** Render 60 frames, verify timing consistency

### 4.4: Second Platform Backend (Month 12)
**Testable Milestone:** Same demo renders identically on two backends

#### Step 4.4.1: Choose platform backend
- **macOS:** Metal (via MetalKit)
- **Linux:** Vulkan or OpenGL
- **Windows:** Direct2D or Direct3D
- **Implementation:** `src/backends/{platform}/{platform}_backend.cpp`

#### Step 4.4.2: Implement platform backend
- **Parity:** All primitives from 4.3.2
- **Test:** Same test suite as CPU backend

#### Step 4.4.3: Backend selection at runtime
- **API:** `create_backend(backend_type::metal)` or `backend_type::cpu`
- **Test:** Switch backends, verify identical visual output

### Deliverable: Phase 4 Completion
- **Release:** v0.4.0 - "First Render"
- **Documentation:** Backend implementation guide
- **Demo:** Counter app with visible buttons (runs on 2+ backends)
- **Acceptance:** Screenshot comparison tool shows <1% diff between backends

---

## Phase 5: Standard Components (Months 13-16)

**Goal:** Batteries-included component library built via composition.

### 5.1: Text Shaping and Rendering (Month 13)
**Testable Milestone:** Rich text renders correctly

#### Step 5.1.1: Complete text abstraction
- **Features:** Multi-line, word wrap, alignment, truncation
- **File:** Extend `text_bridge.hpp`
- **Test:** Long text wraps correctly at boundaries

#### Step 5.1.2: Attributed text
- **API:** Ranges with different attributes
- **Example:** "Hello **world**" → bold range
- **Test:** Styled text renders with correct formatting

### 5.2: Core Input Components (Month 13-14)

#### Step 5.2.1: Button
- **File:** `include/stdui/components/button.hpp`
- **Composition:** Overlay of background rect + text + gesture recognizer
- **States:** normal, hovered, pressed, disabled
- **API:** `button("Click Me", on_click_handler)`
- **Test:** Click fires handler, visual state changes on hover/press

#### Step 5.2.2: Toggle/Checkbox
- **Composition:** Button + boolean state + checkmark overlay
- **API:** `toggle("Enable", binding(&enabled))`
- **Test:** Click toggles state, visual shows check/uncheck

#### Step 5.2.3: Slider
- **Composition:** Track rect + thumb circle + pan gesture
- **API:** `slider(binding(&volume), {.min=0, .max=100})`
- **Test:** Drag thumb updates bound value proportionally

#### Step 5.2.4: Text Field
- **Composition:** Rect + text + caret + focus + keyboard events
- **API:** `text_field(binding(&text), {.placeholder="Enter name"})`
- **Test:** Type text updates binding, displays correctly

### 5.3: Environment and Theming (Month 14)

#### Step 5.3.1: Environment value system
- **File:** `include/stdui/environment.hpp`
- **Mechanism:** Inherited context values (like React context)
- **Values:** theme, locale, scale factor, color scheme
- **API:** 
  ```cpp
  environment_value<color_scheme> scheme = color_scheme::dark;
  with_environment(scheme, child_expr);
  auto current = ctx.environment<color_scheme>();
  ```
- **Test:** Child reads parent's environment value

#### Step 5.3.2: Semantic colors
- **API:** `color::primary`, `color::secondary`, `color::background`, `color::text`
- **Theme:** Dark and light variants
- **Test:** Switching color scheme updates all semantic colors

### 5.4: Layout Components (Month 15)

#### Step 5.4.1: Spacer
- **Composition:** Empty element with fill flex policy
- **API:** `spacer()` - fills available space
- **Test:** `hstack(item, spacer(), item)` pushes items to edges

#### Step 5.4.2: Divider
- **Composition:** Thin rect spanning cross-axis
- **API:** `divider()` in vstack → horizontal line
- **Test:** Divider renders as 1px line between items

#### Step 5.4.3: Picker/Dropdown
- **Composition:** Button + overlay popup + list
- **API:** `picker(binding(&selected), options)`
- **Test:** Click opens menu, selecting option updates binding

### 5.5: Collection Components (Month 16)

#### Step 5.5.1: List
- **Composition:** Scroll + vstack + dynamic_list
- **Virtualization:** Render only visible items
- **API:** `list(items, [](auto& item){ return list_row(item); })`
- **Test:** Scroll through 10,000 items smoothly (only ~20 rendered)

#### Step 5.5.2: Table
- **Composition:** Grid + headers + scrollable content
- **API:** 
  ```cpp
  table(data, {
    column("Name", &Item::name),
    column("Age", &Item::age),
  })
  ```
- **Test:** Display tabular data with sortable columns

### 5.6: Semantics and Accessibility (Month 16)

#### Step 5.6.1: Semantic node tree
- **File:** `include/stdui/semantics.hpp`
- **Attributes:** role, label, value, actions
- **Generation:** Parallel to render tree
- **Test:** Generate semantic tree for button, verify role="button"

#### Step 5.6.2: Accessibility properties
- **API:** `accessible(button, {.label="Submit form", .hint="Press to submit"})`
- **Test:** Semantic tree includes accessibility metadata

### Deliverable: Phase 5 Completion
- **Release:** v0.5.0 - "Component Library"
- **Documentation:** Component catalog with examples
- **Demo:** Form UI with all input types, themed light/dark

---

## Phase 6: Animation & Transitions (Months 17-18)

**Goal:** Smooth, declarative animations.

### 6.1: Animation Primitives (Month 17)

#### Step 6.1.1: Animatable value types
- **File:** `include/stdui/animation.hpp`
- **Types:** double, color, point, size, transform
- **Interpolation:** Linear, ease-in/out, spring
- **Test:** Interpolate 0→100 over 1 second, sample at 0.5s = 50

#### Step 6.1.2: Animation timeline
- **Concepts:** start time, duration, curve, completion
- **Implementation:** Time-based interpolation
- **Test:** Start animation, advance time, verify interpolated value

#### Step 6.1.3: Animation API
- **API:** 
  ```cpp
  animated(opacity, duration(0.3), ease_in_out)
  with_animation(duration(0.5), [&]{ state.set(new_value); })
  ```
- **Test:** Changing animated property triggers smooth transition

### 6.2: Transition System (Month 17)

#### Step 6.2.1: Transition types
- **Types:** opacity, scale, slide, rotate
- **API:** `transition(child, fade_in())`
- **Test:** Adding element fades in over default duration

#### Step 6.2.2: Implicit animations
- **Mechanism:** Detect state changes, animate affected properties
- **Test:** Button background color change animates automatically

### 6.3: Advanced Animation (Month 18)

#### Step 6.3.1: Spring physics
- **Model:** Damped spring simulation
- **API:** `spring(stiffness=300, damping=0.7)`
- **Test:** Spring animation reaches target with natural motion

#### Step 6.3.2: Gesture-driven animation
- **Pattern:** Pan gesture → animated offset tracks touch
- **API:** `draggable(child)` with momentum
- **Test:** Drag and release, element coasts to stop

### Deliverable: Phase 6 Completion
- **Release:** v0.6.0 - "Animations"
- **Demo:** Animated list with item insertion/deletion transitions

---

## Phase 7: Platform Integration (Months 19-22)

**Goal:** Full platform lifecycle and integration.

### 7.1: Window Management (Month 19)

#### Step 7.1.1: Platform window abstraction
- **File:** `include/stdui/window.hpp`
- **API:**
  ```cpp
  window_config cfg{.title="App", .size={800,600}};
  auto win = create_window(cfg);
  win.set_content(root_expression);
  win.show();
  ```
- **Platforms:** macOS (NSWindow), Linux (X11/Wayland), Windows (Win32)
- **Test:** Create window, verify visible on screen

#### Step 7.1.2: Window events
- **Events:** resize, close, move, focus change
- **Test:** Resize window, content re-layouts correctly

#### Step 7.1.3: Multi-window support
- **API:** Create multiple windows
- **Test:** Two windows run independently

### 7.2: Application Lifecycle (Month 20)

#### Step 7.2.1: Application object
- **File:** `include/stdui/application.hpp`
- **API:**
  ```cpp
  int main() {
    application app;
    app.set_root(make_root_view());
    return app.run();
  }
  ```
- **Test:** App launches, runs event loop, exits cleanly

#### Step 7.2.2: Device scale handling
- **Mechanism:** Logical → physical pixel conversion
- **Test:** On 2x display, 100pt rect renders at 200px

### 7.3: Accessibility Platform Bridges (Month 21)

#### Step 7.3.1: Screen reader integration
- **Platforms:** macOS (NSAccessibility), Linux (AT-SPI), Windows (UIA)
- **Mapping:** Semantic tree → platform accessibility tree
- **Test:** VoiceOver/NVDA can navigate UI and read elements

#### Step 7.3.2: Keyboard navigation
- **Standard shortcuts:** Tab, Space, Enter work correctly
- **Test:** Navigate form using only keyboard

### 7.4: Advanced Platform Features (Month 22)

#### Step 7.4.1: IME/Text input
- **Integration:** Native text input for CJK languages
- **Test:** Type Chinese characters in text field

#### Step 7.4.2: Clipboard
- **API:** `clipboard::set_text()`, `clipboard::get_text()`
- **Test:** Copy/paste text between app and system

#### Step 7.4.3: Drag and drop
- **API:** `draggable(item)`, `drop_target(handler)`
- **Test:** Drag file from desktop into app

### Deliverable: Phase 7 Completion
- **Release:** v0.9.0 - "Platform Complete"
- **Demo:** Multi-window app with full accessibility

---

## Phase 8: Polish & v1.0 (Months 23-24)

**Goal:** Production-ready framework.

### 8.1: Performance Optimization

#### Step 8.1.1: Profiling and benchmarking
- **Targets:** Layout <16ms for 1000 nodes, render <16ms
- **Tools:** Instruments, perf, Tracy profiler
- **Test:** Stress test with complex layouts

#### Step 8.1.2: Memory optimization
- **Focus:** Reduce allocations, object pooling
- **Test:** Memory usage stable over time

### 8.2: Error Handling and Diagnostics

#### Step 8.2.1: Concept diagnostics
- **Improve:** Template error messages
- **Technique:** Requires clauses with clear names
- **Test:** Verify error messages are readable

#### Step 8.2.2: Runtime assertions
- **Validation:** Detect invalid state early
- **Test:** Trigger assertions in debug builds

### 8.3: Documentation and Examples

#### Step 8.3.1: API documentation
- **Tool:** Doxygen with examples
- **Coverage:** 100% of public API

#### Step 8.3.2: Tutorial series
- **Topics:** Hello World, layouts, state, components, custom components
- **Format:** Step-by-step with runnable code

#### Step 8.3.3: Example applications
- **Apps:** Todo list, calculator, image viewer, chat UI
- **Purpose:** Demonstrate real-world usage

### 8.4: Testing and Quality

#### Step 8.4.1: Expand test coverage
- **Target:** 90%+ line coverage
- **Test types:** Unit, integration, visual regression

#### Step 8.4.2: Cross-platform CI
- **Platforms:** macOS, Linux, Windows
- **Test:** All platforms pass all tests

### Deliverable: v1.0.0 Release
- **Release:** v1.0.0 - "Production Ready"
- **Documentation:** Complete API docs + tutorials
- **Stability:** Semantic versioning commitment
- **Announcement:** Blog post, HN launch

---

## Success Metrics

### Technical Metrics
- **Performance:** 60fps on 2015 hardware for complex UIs
- **Test Coverage:** >85% line coverage
- **Build Time:** Full rebuild <2 minutes
- **Binary Size:** Hello World app <5MB
- **Platform Support:** macOS 10.15+, Windows 10+, Linux (X11/Wayland)

### API Quality Metrics
- **Compile-time Errors:** Readable messages for common mistakes
- **Lines of Code:** Simple UI <50 lines, complex form <200 lines
- **Learning Curve:** Productive in <1 day for experienced C++ dev

### Adoption Metrics (Post-v1.0)
- **GitHub Stars:** Target 1000+ in first year
- **Production Use:** 3+ public projects using stdui
- **Contributors:** 10+ external contributors

---

## Risk Mitigation

### Risk 1: Template Complexity → Slow Compiles
**Mitigation:**
- Use explicit instantiation for common types
- Profile compile times continuously (target: <1s for single TU)
- Consider type erasure for dynamic polymorphism needs

### Risk 2: Reconciliation Performance
**Mitigation:**
- Benchmark with 10,000 node trees
- Implement dirty tracking and incremental updates
- Profile and optimize hot paths

### Risk 3: Platform Divergence
**Mitigation:**
- Automated visual regression tests
- Pixel-perfect screenshot comparisons
- Shared test suite runs on all platforms

### Risk 4: Accessibility Complexity
**Mitigation:**
- Start with semantic tree early (Phase 5)
- Collaborate with accessibility experts for review
- Test with actual screen readers

### Risk 5: Animation Performance
**Mitigation:**
- GPU-accelerated rendering where possible
- Compositor thread for animations
- Profiling with realistic content

---

## Development Workflow

### Daily Development
1. Work in feature branches (`feat/`, `fix/`)
2. Write tests first (TDD approach)
3. Ensure CI passes before merging
4. Update documentation with code changes

### Code Review Standards
- All changes require review
- Tests must pass
- Coverage must not decrease
- API changes need documentation update

### Release Process
1. Feature freeze 1 week before release
2. RC testing period
3. Update CHANGELOG.md
4. Tag release (semantic versioning)
5. Deploy documentation
6. Publish announcement

---

## Resources Needed

### Development Tools
- CMake 3.20+
- C++20 compiler (GCC 11+, Clang 14+, MSVC 2022+)
- Platform SDKs (Xcode, Windows SDK, Linux dev packages)
- Rendering libraries (Blend2D, or platform APIs)
- Text engines (HarfBuzz/FreeType)

### Testing Tools
- Doctest (already integrated)
- Visual regression: pixelmatch or similar
- Profilers: Instruments (Mac), perf (Linux), Tracy

### Documentation Tools
- Doxygen for API docs
- MkDocs or similar for tutorials
- GitHub Pages for hosting

---

## Next Actions (This Week)

### Immediate (Days 1-3)
1. ✅ Complete layout config DSL exposure (finish current branch)
2. Merge feat/layout-config-dsl to main
3. Add grid expressions to DSL
4. Write integration test for complete Phase 1

### Short-term (Days 4-7)
5. Implement `state<T>` persistent storage
6. Create counter component example
7. Write Phase 1 completion article
8. Cut v0.1.0 release

### Planning
9. Review this plan with stakeholders
10. Create GitHub issues for Phase 2 tasks
11. Set up project board with Phase 2 milestones

---

## Appendix A: Architecture Validation Checkpoints

After each phase, validate these architectural principles:

✓ **View expressions ≠ persistent UI:** Can re-evaluate expressions without losing state
✓ **Composition over inheritance:** New components built without base classes
✓ **Generic layout:** Layout algorithms work with any content type
✓ **Backend independence:** Same UI code runs on different backends
✓ **Type safety:** Common mistakes caught at compile time

---

## Appendix B: API Design Principles

1. **Zero-cost abstractions:** Pay only for what you use
2. **Compile-time checking:** Catch errors early
3. **Readable errors:** Template diagnostics are comprehensible
4. **Familiar patterns:** Learn from SwiftUI but stay idiomatic to C++
5. **Progressive disclosure:** Simple things simple, complex things possible

---

## Appendix C: Testing Strategy

### Test Pyramid
- **Unit tests (70%):** Individual functions, layout algorithms
- **Integration tests (20%):** Component composition, reconciliation
- **End-to-end tests (10%):** Full application scenarios

### Test Categories
- **Functional:** Correctness of behavior
- **Performance:** Layout/render under 16ms
- **Visual:** Screenshot comparison
- **Accessibility:** Screen reader compatibility
- **Platform:** Cross-platform parity

---

**Last Updated:** 2026-09-03
**Version:** 1.0
**Status:** Ready for Phase 2 kickoff
