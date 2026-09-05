# Rendering Subsystem Design v2 - Technical Deep Dive

**Date:** 2026-09-04  
**Status:** Revised incorporating stakeholder feedback  
**Replaces:** rendering-research-report.md (v1)

---

## Executive Summary

This document presents the revised rendering architecture for stdui based on stakeholder review. Key changes from v1:

**Major Revisions:**

1. **All modern effects required** (blur, shadows, gradients) - "minimal" refers to style, not capabilities
2. **Viewport is a composable view** - participates in layout like any UI element
3. **GPU-based UI rendering recommended** - enables zero-copy composition with 3D content
4. **Platform-specific implementations** - no cross-platform abstraction penalty
5. **Separate threading for UI and viewport** - no interference between subsystems

**Architecture:** Two-tier rendering with complete independence:

- **Tier 1 (2D UI Layer):** Minimal GPU-accelerated primitives for panels, text, shapes, effects
- **Tier 2 (Application Content):** Direct native API access (Metal/Vulkan/D3D12), zero framework overhead

---

## 1. Requirements & Constraints

### 1.1 Functional Requirements

**2D UI Rendering Must Support:**

- ✅ Text rendering (platform-native quality)
- ✅ Basic shapes (rectangles, rounded corners, paths)
- ✅ Gradients (linear, radial, conical)
- ✅ Blur effects (Gaussian, dual-Kawase)
- ✅ Drop shadows and backdrop blur
- ✅ Opacity and color transformations
- ✅ Clipping with antialiasing
- ✅ Smooth animations (Phase 5)

**Viewport Integration Must Provide:**

- ✅ Direct Metal/Vulkan/D3D12 device access
- ✅ Zero-copy composition with UI layer
- ✅ Separate thread operation (no contention)
- ✅ Full control over rendering (no framework interference)
- ✅ Viewport-as-a-view (participates in layout)

### 1.2 Non-Functional Requirements

**Performance:**

- UI rendering: <1ms per frame (target 0.5ms)
- Zero GPU→CPU→GPU copies for composition
- 60+ FPS sustained under heavy 3D load
- Frame pacing decoupled (UI independent of 3D framerate)

**Quality:**

- Text rendering matches platform native appearance
- Effects are high-quality (not performance-compromised approximations)
- Antialiasing on all edges and curves
- Color-correct blending (gamma-aware)

**Architecture:**

- Platform-specific code in separate files (CMake-controlled)
- No `#ifdef` macros in shared implementation code
- Minimal public API surface
- Zero dependencies on macOS/Windows (system frameworks only)

---

## 2. Critical Architectural Decision: CPU vs GPU UI Rendering

### 2.1 The Problem

**v1 Report Recommendation:** CPU-based UI (Core Graphics/Cairo)  
**Why This Was Wrong:** Cannot achieve zero-copy composition with GPU 3D content.

**The Issue:**

```
CPU UI Rendering              GPU 3D Rendering
     ↓                              ↓
CPU memory buffer            GPU texture/framebuffer
     ↓                              ↓
Upload to GPU         ←→    No direct path!
     ↓
Compositor reads both
```

To composite:

1. 3D content renders to GPU texture
2. CPU UI renders to system memory
3. **Must upload UI pixels to GPU** (expensive copy)
4. GPU compositor blends them

**Cost:** ~2-4ms for a 1920×1080 buffer upload at 60Hz. This violates the <1ms UI rendering budget.

### 2.2 The Solution: GPU-Based UI Rendering

**Revised Recommendation:** GPU-based 2D rendering on all platforms.

**Architecture:**

```
GPU 2D UI Rendering          GPU 3D Rendering
     ↓                              ↓
GPU texture/layer            GPU texture/layer
     ↓                              ↓
        ↘                      ↙
         Compositor (GPU-side)
              ↓
         Display (zero CPU copy)
```

**Platforms:**

- **macOS:** Metal for UI + Metal for 3D → shared MTLDevice, zero-copy via CAMetalLayer composition
- **Windows:** Direct2D for UI + D3D12/11 for 3D → shared device, DirectComposition layers
- **Linux:** Vulkan for UI + Vulkan for 3D → shared VkDevice, compositor layers

### 2.3 Implications

**Benefits:**

- ✅ Zero-copy composition (UI and 3D on same GPU)
- ✅ Hardware-accelerated effects (blur, shadows, gradients)
- ✅ Efficient multithreading (separate command queues)
- ✅ Consistent architecture across platforms

**Tradeoffs:**

- More complex than CPU rendering (shader programming, resource management)
- Requires platform GPU API expertise
- No pure-software fallback on modern systems (acceptable - target is modern GPUs)

**Text Rendering Strategy:**
does text rendering work well on GPU, I'm not an expertise on this topic, is Direct2D and Core Text already on GPU?
how the font loaded into GPU at runtime, does this cost time/GPU memory?

Rasterize with platform engine, does this mean they work on GPU?
you say upload glyphs to GPU, can I read it as uplod all the small image into GPU, sounds not flexiable, we need iterate on this

- **Rasterize with platform engine** (Core Text, DirectWrite, HarfBuzz)
- **Upload glyphs to GPU atlas**
- **Composite on GPU** (preserves quality, enables effects)

This is the approach used by Chrome, Firefox, Skia, and Flutter for exactly this reason.

---

## 3. Threading Model

### 3.1 Requirements

**From Review:** "we want viewport rendering separate with UI rendering, separate threads"

**Goal:** 3D rendering thread and UI rendering thread operate independently without blocking each other.

### 3.2 Modern Graphics API Threading Support

#### Metal (macOS)

does this mean we dont need two threads for ui and 3d rendering? just one commandqueue for ui thread for ui rendering and one queue for each 3d viewport?
all these command queue in one thread, does they eventualy executed sequencely on gpu? since there is only one gpu device?
**Thread Safety:**

- `MTLDevice`: Thread-safe
- `MTLCommandQueue`: Thread-safe
- `MTLCommandBuffer`: Thread-safe for encoding (one encoder per thread)
- `MTLRenderCommandEncoder`: NOT thread-safe (one per buffer)
**Pattern:**

```swift
// UI Thread
let uiQueue = device.makeCommandQueue()!
let uiBuffer = uiQueue.makeCommandBuffer()!
// ... encode UI commands ...
uiBuffer.commit()

// 3D Thread (user's code)
let sceneQueue = device.makeCommandQueue()!
let sceneBuffer = sceneQueue.makeCommandBuffer()!
// ... encode 3D commands ...
sceneBuffer.commit()

// Synchronization via MTLSharedEvent if needed
```

**Key:** Separate command queues, no cross-thread interference.

#### Vulkan (Linux)

**Thread Safety:**

- `VkDevice`: Thread-safe
- `VkQueue`: NOT thread-safe (must externally synchronize submissions)
- `VkCommandBuffer`: Thread-safe for recording (with per-thread command pools)

workflow: commandbuffer1 ->queue ->device?
          commandbuffer2 |
          ommandbuffer 3 |
ui thread and every 3d view port create a commandbuffer, and only one queue and device?

**Pattern:**

```cpp
// Per-thread command pools
VkCommandPool ui_pool;    // UI thread
VkCommandPool scene_pool; // 3D thread

// UI Thread
VkCommandBuffer ui_cmd;
vkBeginCommandBuffer(ui_cmd, ...);
// ... record UI commands ...
vkEndCommandBuffer(ui_cmd);

// 3D Thread
VkCommandBuffer scene_cmd;
vkBeginCommandBuffer(scene_cmd, ...);
// ... record 3D commands ...
vkEndCommandBuffer(scene_cmd);

// Submit (queues need locking)
std::lock_guard lock(queue_mutex);
vkQueueSubmit(graphics_queue, ...);
```

**Key:** Per-thread command pools, external synchronization on queue submission.

#### DirectX 12 (Windows)

**Thread Safety:**

- `ID3D12Device`: Thread-safe
- `ID3D12CommandQueue`: Thread-safe
- `ID3D12CommandList`: NOT thread-safe (close before submitting)
explain the workflow for metal/vulkan and d3d, seems simialr
**Pattern:**

```cpp
// UI Thread
ComPtr<ID3D12GraphicsCommandList> ui_list;
device->CreateCommandList(..., &ui_list);
// ... record UI commands ...
ui_list->Close();
ui_queue->ExecuteCommandLists(1, (ID3D12CommandList**)&ui_list);

// 3D Thread
ComPtr<ID3D12GraphicsCommandList> scene_list;
device->CreateCommandList(..., &scene_list);
// ... record 3D commands ...
scene_list->Close();
scene_queue->ExecuteCommandLists(1, (ID3D12CommandList**)&scene_list);
```

**Key:** Separate command allocators per thread, queue submissions are thread-safe.

### 3.3 Recommended Architecture

**Two-Thread Model:**

```
┌─────────────────┐         ┌─────────────────┐
│   UI Thread     │         │  3D Thread      │
│                 │         │  (User Code)    │
├─────────────────┤         ├─────────────────┤
│ • Layout        │         │ • Scene update  │
│ • UI rendering  │         │ • 3D rendering  │
│ • Event input   │         │ • Physics       │
│ • State update  │         │ • Animation     │
└────────┬────────┘         └────────┬────────┘
         │                           │
         └───────────┬───────────────┘
                     ↓
            ┌─────────────────┐
            │  Compositor     │
            │  (OS/GPU)       │
            └─────────────────┘
```

the above workflow seems only commandbffers are different, they all went into the same queue/device, why we need compotior here, explain this to me like Im a primary student
**Synchronization:**

- **No shared frame state** - each thread renders to its own buffer
- **Pacing decoupled** - 3D can run at 120Hz, UI at 60Hz (or vice versa)
- **Composition async** - OS compositor reads latest available frame from each

so the rendering result/image has to be downloded in to cpu anyway for composition then upload to GPU for display
what is the offscrren rendering part in this architecture? for blur shadow effects?

**Implementation:**

```cpp
// stdui provides
namespace stdui {
    class viewport {
    public:
        // Runs on separate thread, user-supplied
        void set_render_callback(std::function<void(native_api_handles)> callback);
        
        // stdui manages threading, calls callback at appropriate rate
        void start(); 
        void stop();
    };
}

// User writes
viewport.set_render_callback([](auto& vp) {
    // This runs on 3D thread
    id<MTLDevice> device = vp.metal_device();
    // ... render scene ...
});
```

**stdui's Responsibility:**

- Create and manage 3D thread
- Provide rate limiting (target FPS)
- Handle vsync coordination
- Expose native API handles safely

explain vsync like Im a primary student

**User's Responsibility:**

- Encode 3D rendering commands
- Manage scene state thread-safely
- Handle asset loading

---

## 4. Composition Strategies

### 4.1 Option A: OS Compositor (Recommended)

**Concept:** Each subsystem renders to its own layer; OS compositor blends them.

#### macOS: CAMetalLayer Composition

```swift
// Window has multiple CAMetalLayers
window.contentView.layer = CALayer()  // Root

// UI layer (front)
let uiLayer = CAMetalLayer()
uiLayer.device = metalDevice
uiLayer.frame = window.bounds
window.contentView.layer.addSublayer(uiLayer)

// 3D layer (back)
let sceneLayer = CAMetalLayer()
sceneLayer.device = metalDevice
sceneLayer.frame = viewportRect  // Positioned by layout
window.contentView.layer.insertSublayer(sceneLayer, at: 0)

// Each layer has independent drawable cycle
```

**Composition:** Core Animation compositor blends layers on GPU. Zero copy.

#### Windows: DirectComposition

```cpp
// Create DirectComposition visual tree
IDCompositionDevice* comp_device;
IDCompositionTarget* target;
IDCompositionVisual* root_visual;

// 3D layer
IDCompositionVisual* scene_visual;
comp_device->CreateVisual(&scene_visual);
scene_visual->SetContent(d3d12_swapchain);  // User's swapchain

// UI layer
IDCompositionVisual* ui_visual;
comp_device->CreateVisual(&ui_visual);
ui_visual->SetContent(d2d_surface);  // Direct2D UI surface

// Compose
root_visual->AddVisual(scene_visual, ...);
root_visual->AddVisual(ui_visual, ...);
comp_device->Commit();
```

**Composition:** DWM (Desktop Window Manager) compositor. Zero copy.

#### Linux: Compositor Protocol

**Wayland:**

```cpp
// Multiple wl_surface objects
wl_surface* ui_surface = wl_compositor_create_surface(...);
wl_surface* scene_surface = wl_compositor_create_surface(...);

// Subsurface composition
wl_subsurface* sub = wl_subcompositor_get_subsurface(scene_surface, ui_surface);
wl_subsurface_set_position(sub, viewport_x, viewport_y);
```

**X11:**

- Use XComposite extension
- Each layer is a separate window
- Compositor blends them

**Pros:**

- ✅ True zero-copy (no framebuffer reads)
- ✅ OS-optimized composition
- ✅ Can apply effects to individual layers
- ✅ Separate vsync for each layer

**Cons:**

- Cannot apply UI effects that blend across 3D content (e.g., backdrop blur over 3D)
- Slightly more complex setup

### 4.2 Option B: Texture Sharing

**Concept:** 3D renders to a texture, UI samples it and composites.

```cpp
// 3D Thread
MTLTexture* sceneTexture = makeTexture(viewportSize);
renderSceneToTexture(sceneTexture);

// UI Thread
// Viewport view in layout tree says "draw sceneTexture here"
ui_renderer.draw_texture(sceneTexture, viewportRect);
```

**Synchronization:**

- Metal: `MTLSharedEvent` signals when 3D frame completes
- Vulkan: `VkSemaphore` (timeline semaphores for cross-queue)
- D3D12: `ID3D12Fence`

**Pros:**

- ✅ Can apply UI effects over 3D (rounded corners, shadows on viewport)
- ✅ Simpler to reason about (single final framebuffer)

**Cons:**

- Must synchronize carefully (fence/semaphore overhead)
- 3D frame latency affects UI (if UI waits for 3D)

### 4.3 Recommendation

**Use Option A (OS Compositor) by default** for:

- Maximum performance
- Decoupled frame rates
- Simpler synchronization

**Support Option B (Texture Sharing) for advanced use cases:**

- When user wants UI effects blending with 3D
- When viewport needs rounded corners with smooth edges
- When implementing picture-in-picture or thumbnail views

**API:**

```cpp
// Option A (default)
auto viewport = stdui::viewport([]( auto& vp) {
    // 3D rendering callback
});

// Option B (explicit)
auto viewport = stdui::viewport_textured([](auto& vp) {
    // Render to vp.output_texture()
    return vp.output_texture();
});
```

---

## 5. Text Rendering Strategy

### 5.1 Requirements

**From Review:** "we want the best quality then best performance priority"

**Quality Factors:**

1. **Hinting:** Grid-fitting at small sizes
2. **Gamma-correct blending:** Linear-space RGB blending
3. **Subpixel positioning:** Fractional glyph placement
4. **Subpixel antialiasing:** RGB subpixel coverage (ClearType on Windows)
5. **Stem darkening:** Thickening stems at small sizes to maintain weight

### 5.2 Platform Text Engines

#### macOS: Core Text

**Current State (as of macOS 14):**

- Removed subpixel antialiasing in Mojave (10.14)
- Now uses grayscale antialiasing with gamma correction
- Renders to CPU buffer via Core Graphics

**Quality:** Excellent. Apple's text rendering is specifically tuned for Retina displays where subpixel AA is unnecessary.

**Integration:**

```objc
CTFontRef font = CTFontCreateWithName(CFSTR("SF Pro"), 14.0, NULL);
CTLineRef line = CTLineCreateWithAttributedString(attrString);
CGContextSetTextPosition(cgContext, x, y);
CTLineDraw(line, cgContext);
```

**For GPU:** Rasterize glyphs to bitmaps, upload to texture atlas.

#### Windows: DirectWrite + ClearType

**Features:**

- Subpixel antialiasing (ClearType) for LCD displays
- Y-direction antialiasing only (grayscale vertical)
- Gamma-correct blending

**Challenge:** ClearType requires knowing background color (it's a 3-channel blend):

```
output.r = glyph_coverage.r * foreground.r + (1 - glyph_coverage.r) * background.r
output.g = glyph_coverage.g * foreground.g + (1 - glyph_coverage.g) * background.g  
output.b = glyph_coverage.b * foreground.b + (1 - glyph_coverage.b) * background.b
```

**Workaround for transparent backgrounds:**

1. **Grayscale fallback:** Disable ClearType for transparent text
2. **Dual-source blending:** GPU extension allows outputting coverage separately
3. **Opaque tile:** Render text to opaque tile, composite tile

**DirectWrite can render directly to Direct2D** (GPU target), which is ideal.

#### Linux: HarfBuzz + FreeType

**Why:** No single native text stack on Linux. Options:

- Pango (too heavy, pulls in GTK dependencies)
- Qt text (too heavy)
- HarfBuzz + FreeType (industry standard, used by Chrome/Firefox)

**HarfBuzz:** Text shaping (Unicode → glyph indices + positions)
**FreeType:** Glyph rasterization

**Quality:** Good. Not quite platform-native but high-quality and tunable.

### 5.3 Recommended Approach: Hybrid

**Shaping:**

- macOS: Core Text (for system font fallback, emoji, CJK)
- Windows: DirectWrite
- Linux: HarfBuzz

**Rasterization:**

- All platforms: Platform engine → CPU bitmap

**Compositing:**

- Upload to GPU glyph atlas (texture array or single large texture)
- Render glyphs as textured quads on GPU

**This is what Chrome, Firefox, and Skia do** for GPU-accelerated text while preserving platform quality.

---

## 6. Effects Implementation

### 6.1 Gaussian Blur

**Naïve Gaussian:** O(N²) per pixel (too expensive)

**Separable Gaussian:** O(2N) per pixel (vertical + horizontal pass)

```
1. Render content to texture
2. Horizontal blur pass: sample N pixels horizontally
3. Vertical blur pass: sample N pixels vertically  
```

**Dual Kawase Blur (Preferred):**

- Downsamples image iteratively
- Each level is a 4-sample downsample with blur
- Upsample with 4-sample blur
- Approximates Gaussian, ~5× faster than true Gaussian

```
Original (1024×1024)
  ↓ downsample + blur
512×512
  ↓ downsample + blur
256×256
  ↓ downsample + blur
128×128
  ↑ upsample + blur
256×256
  ↑ upsample + blur
512×512
  ↑ upsample + blur
Final (1024×1024, blurred)
```

LGTM
**Used by:** KDE, Android, modern game engines

### 6.2 Drop Shadow

**Algorithm:**

1. Render content to RGBA texture (color + alpha)
2. Extract alpha channel
3. Blur alpha channel (Dual Kawase)
4. Composite: blurred alpha underneath, content on top

# curious about this shadow implementatins, explain this to me

does this looks nice?
do we only need to blur around the edges, since the inner part will be overriden by the real content eventually

```
glsl
// Fragment shader (simplified)
vec4 shadow = texture(blurred_alpha, uv) * shadow_color;
vec4 content = texture(content_texture, uv);
fragColor = mix(shadow, content, content.a);  // Alpha blend
```

### 6.3 Backdrop Blur

**Requirements:** Blur content BEHIND a UI element (e.g., translucent panel).

**Algorithm:**

1. Render everything behind the panel to texture
2. Blur that texture (Dual Kawase)
3. Render panel with blurred background

is it possible to use a much lower resolution rendering for the background to gain better peerformance, do modern ui frameowrk use this method, is this worth it?

**Challenge:** Must clip blur region to panel shape.

**Implementation:**

```
1. Render background → texture A
2. Render panel shape to stencil buffer
3. Blur texture A with stencil mask → texture B
4. Render panel with texture B as background
```

I'm always confused about the stencil buffer, explain it to me like a primary student:
- what it is
- what is its use cases, why it is so special that deserve a new type of buffer

### 6.4 Rounded Corners

**For UI elements:** Trivial (rectangle with rounded corner shader/geometry)

**For 3D viewport with rounded corners:** More complex.

**Option A: Stencil Mask**

```
1. Render rounded rectangle to stencil buffer
2. Render 3D content with stencil test enabled
```

**Option B: SDF Mask in Shader**

```glsl
float rounded_rect_sdf(vec2 pos, vec2 size, float radius) {
    vec2 q = abs(pos) - size + radius;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - radius;
}

// Sample 3D texture, apply mask
vec4 scene = texture(scene_texture, uv);
float mask = 1.0 - smoothstep(0.0, 1.0, rounded_rect_sdf(...));
fragColor = scene * mask;
```

**Option B gives smoother antialiasing** and is preferred for high-quality rounded corners on the viewport.

what is the perf cost of option B since it sounds like a better qualiy method, it does has a perf cost, right?
---

## 7. Platform Backends

### 7.1 macOS: Metal

**Why Metal (not Core Graphics):**

- GPU-accelerated (required for zero-copy composition)
- Excellent text rendering (rasterize with Core Text, composite on Metal)
- Native to macOS (zero dependencies)
- Well-documented, stable API

**Architecture:**

```
Core Text → glyph atlas (MTLTexture)
           ↓
Metal rendering (MTLRenderCommandEncoder)
           ↓
CAMetalLayer drawable
           ↓
Compositor (with user's 3D CAMetalLayer)
```

**Text Pipeline:**

```objc
// Rasterize glyph
CTFontRef font = ...;
CGGlyph glyph = ...;
CGRect bounds = CTFontGetBoundingRectsForGlyphs(font, ...);
CGContextRef bitmap_ctx = CGBitmapContextCreate(...);
CTFontDrawGlyphs(font, &glyph, &position, 1, bitmap_ctx);
void* pixels = CGBitmapContextGetData(bitmap_ctx);

// Upload to GPU
MTLTexture* atlas = ...;
[atlas replaceRegion:region withBytes:pixels bytesPerRow:...];

// Render
// Draw textured quad with glyph atlas texture
```

**Effects:** MSL (Metal Shading Language) shaders for blur, shadows, etc.

### 7.2 Windows: Direct2D + DirectWrite

**Why Direct2D:**

- GPU-accelerated
- DirectWrite integration (best Windows text)
- System-provided (zero dependencies)
- Works with D3D11/D3D12 interop

**Architecture:**

```
DirectWrite → glyph run
            ↓
Direct2D (ID2D1DeviceContext)
            ↓
ID2D1Bitmap backed by DXGI surface
            ↓
DirectComposition (with user's D3D12 swapchain)
```

**D3D12 Interop:**

```cpp
// Create D2D device sharing D3D12 device
ID2D1Factory* d2d_factory;
ID2D1Device* d2d_device;
ID2D1DeviceContext* d2d_context;

// Create DXGI surface backed by D3D12 texture
ID3D12Resource* d3d12_texture;
IDXGISurface* dxgi_surface;
d3d12_texture->QueryInterface(&dxgi_surface);

// Create D2D bitmap from DXGI surface
ID2D1Bitmap1* d2d_bitmap;
d2d_context->CreateBitmapFromDxgiSurface(dxgi_surface, ..., &d2d_bitmap);

// Render UI to d2d_bitmap
d2d_context->BeginDraw();
// ... Direct2D drawing commands ...
d2d_context->EndDraw();

// d3d12_texture now contains rendered UI, can be composed with 3D
```

**Effects:** Direct2D built-in effects (GaussianBlur, Shadow, etc.) or custom pixel shaders.

### 7.3 Linux: Vulkan + Skia

**Why Vulkan:**

- Cross-platform GPU API
- Well-supported on Linux
- Direct path to Wayland/X11 surfaces

**Why Skia (for Linux):**

- Mature 2D rendering library
- Vulkan backend
- High-quality text (HarfBuzz integration)
- Used by Chrome, Android, Flutter
why we need skia, we do all the custom 2d rendering on windows and mac already, right? do it with vulkan just another copy? you are an AI, you are good at translating/copy code from one platform to another
**Alternative:** Custom Vulkan renderer (more work, more control)

**Architecture:**

```
HarfBuzz + FreeType → shaped glyph runs
                    ↓
Skia (SkCanvas, Vulkan backend)
                    ↓
VkImage (render target)
                    ↓
Vulkan surface (with user's Vulkan 3D rendering)
```

**Skia Vulkan Setup:**

```cpp
// Create Skia Vulkan context
GrContextOptions options;
sk_sp<GrDirectContext> skia_context = GrDirectContext::MakeVulkan(
    vulkan_backend, options
);

// Create surface from Vulkan image
GrVkImageInfo vk_info = { vk_image, ... };
GrBackendRenderTarget vk_rt(width, height, sample_count, vk_info);
sk_sp<SkSurface> surface = SkSurface::MakeFromBackendRenderTarget(
    skia_context.get(), vk_rt, ...
);

// Render
SkCanvas* canvas = surface->getCanvas();
canvas->drawRect(...);
canvas->drawTextBlob(...);

// Flush to Vulkan
skia_context->flush();
```

**Dependency:** Skia (~2MB compiled). Acceptable for Linux where system libraries vary.

---

## 8. Visual Testing

### 8.1 Golden Image Testing

**Approach:** Render a scene, compare output image to a "golden" reference.

**Tools:**

- Skia: Gold (<https://skia.org/docs/dev/testing/skiagold/>)
- Flutter: `matchesGoldenFile()` (uses Skia Gold internally)
- Chrome: `cc/test/pixel_test.cc`

**Tolerance:**

```cpp
struct CompareResult {
    int max_channel_delta;  // Max per-channel difference (0-255)
    int num_differing_pixels;
    float percent_differing;
};

bool images_match(Image a, Image b, int tolerance = 1, float fuzz_percent = 0.01) {
    auto diff = compare(a, b);
    return diff.max_channel_delta <= tolerance 
        && diff.percent_differing <= fuzz_percent;
}
```

**Why tolerance needed:**

- Floating-point rounding
- GPU driver differences
- Antialiasing variations

### 8.2 Platform Text Challenge

**Problem:** Text rasterization differs per-platform (Core Text vs DirectWrite vs FreeType).

**Solutions:**

**Option A: Per-Platform Goldens**

```
tests/
  goldens/
    macos/
      test_case_1.png
    windows/
      test_case_1.png
    linux/
      test_case_1.png
```

**Option B: Deterministic Test Font**

- Use bitmap font or fixed-rasterization font
- Ensures identical output across platforms
- Loses platform-native appearance testing

**Option C: Display List Testing**

- Don't test pixels, test rendering commands

```cpp
assert(display_list == {
    FillRect { bounds: {0, 0, 100, 50}, color: red },
    DrawText { text: "Hello", position: {10, 30}, font: {...} },
});
```

- Platform-agnostic
- Doesn't test actual visual output

**Recommendation:** Option A for critical UI, Option C for cross-platform tests.

### 8.3 CI/CD Integration

**Headless Testing:**

- macOS: Not truly headless, use `xvfb` or vnc
- Linux: `Xvfb` (virtual X server) or headless Vulkan
- Windows: Mesa3D software renderer or headless D3D

**Automated Diff Review:**

- On golden mismatch, generate diff image
- Upload to review system (e.g., Gerrit, GitHub PR)
- Reviewer approves or rejects

do we have this in the roadmap, add the content that are not in the roadmap
---

## 9. Implementation Plan

### 9.1 Phase Organization

#### Phase 4.1: Render Tree Construction (2 weeks)

**Goal:** Backend-independent render primitives.

**Deliverables:**

- `render_node` structure (rectangles, text, paths, effects)
- `render_tree_builder` (layout tree → render tree)
- Tests (headless, no actual rendering)

---

#### Phase 4.2: Metal Backend (macOS) (3 weeks)

**Goal:** First complete backend, proves architecture.

**Deliverables:**

- `metal_renderer` implementing `ui_renderer`
- Core Text glyph atlas
- Basic shapes (rectangles, paths)
- Text rendering
- Blur and shadow effects
- CAMetalLayer integration
- Visual tests (golden images)

---

#### Phase 4.3: Direct2D Backend (Windows) (3 weeks)

**Goal:** Second backend, confirms portability.

**Deliverables:**

- `direct2d_renderer` implementing `ui_renderer`
- DirectWrite text integration
- D3D11/D3D12 interop
- DirectComposition integration
- Visual parity tests with Metal backend

---

#### Phase 4.4: Vulkan/Skia Backend (Linux) (3 weeks)

**Goal:** Complete cross-platform coverage.

**Deliverables:**

- `vulkan_skia_renderer` implementing `ui_renderer`
- HarfBuzz text integration
- Wayland/X11 surface support
- Visual parity tests

---

#### Phase 4.5: Viewport Integration (2 weeks)

**Goal:** Expose native 3D API, enable composition.

**Deliverables:**

- `viewport` as composable view
- Metal viewport (macOS)
- Vulkan viewport (Linux)
- D3D12 viewport (Windows)
- Example: spinning cube + UI chrome
- Threading model (separate 3D thread)
- Synchronization (fences/semaphores)

---

#### Phase 4.6: Frame Scheduling & Polish (2 weeks)

**Goal:** Production-ready vsync and performance.

**Deliverables:**

- Frame scheduler (vsync-aligned)
- Performance profiling
- Window resize handling
- HiDPI support verification
- Documentation

**Total: 15 weeks (3.75 months)**

---

## 10. Code Organization

### 10.1 Directory Structure

```
stdui/
  include/
    stdui/
      stdui.hpp              // Umbrella header
      rendering.hpp          // Platform-agnostic interface
      viewport.hpp           // Viewport view (optional header)
      effects.hpp            // Effect APIs (optional header)
      
  src/
    rendering/
      render_tree.cpp        // Common render tree builder
      
    platform/
      macos/
        metal_renderer.mm
        metal_viewport.mm
        core_text_shaper.mm
        
      windows/
        direct2d_renderer.cpp
        d3d12_viewport.cpp
        directwrite_shaper.cpp
        
      linux/
        vulkan_skia_renderer.cpp
        vulkan_viewport.cpp
        harfbuzz_shaper.cpp
```

### 10.2 CMake Integration

```cmake
# Detect platform
if(APPLE)
    set(STDUI_PLATFORM "macos")
elseif(WIN32)
    set(STDUI_PLATFORM "windows")
elseif(UNIX)
    set(STDUI_PLATFORM "linux")
endif()

# Add platform-specific sources
add_library(stdui_rendering
    src/rendering/render_tree.cpp
    src/platform/${STDUI_PLATFORM}/renderer_impl.cpp
    src/platform/${STDUI_PLATFORM}/viewport_impl.cpp
    src/platform/${STDUI_PLATFORM}/text_impl.cpp
)

# Platform-specific dependencies
if(STDUI_PLATFORM STREQUAL "macos")
    target_link_libraries(stdui_rendering PRIVATE
        "-framework Metal"
        "-framework CoreText"
        "-framework QuartzCore"
    )
elseif(STDUI_PLATFORM STREQUAL "windows")
    target_link_libraries(stdui_rendering PRIVATE
        d2d1.lib
        dwrite.lib
        dxgi.lib
    )
elseif(STDUI_PLATFORM STREQUAL "linux")
    find_package(Vulkan REQUIRED)
    find_package(Skia REQUIRED)
    target_link_libraries(stdui_rendering PRIVATE
        Vulkan::Vulkan
        Skia::Skia
    )
endif()
```

**NO #ifdef in shared headers or implementation files.**

### 10.3 Public API

**Core Header:**

```cpp
// stdui.hpp
#pragma once

// Core primitives
#include <stdui/expressions.hpp>
#include <stdui/layout.hpp>
#include <stdui/state.hpp>
#include <stdui/component.hpp>
#include <stdui/geometry.hpp>

// Rendering (platform-agnostic interface)
#include <stdui/rendering.hpp>

// Application entry point
namespace stdui {
    void run(/* root view expression */);
}
```

**Optional Headers:**

```cpp
#include <stdui/effects.hpp>   // Blur, shadows, gradients
#include <stdui/viewport.hpp>  // Metal/Vulkan/D3D12 integration
```

---

## 11. API Examples

### 11.1 Entry Point (Revised)

**Simple:**

```cpp
#include <stdui/stdui.hpp>

int main() {
    stdui::run(
        stdui::vstack(
            stdui::text("Hello, stdui"),
            stdui::text("Rendering v2")
        )
    );
}
```

**With Configuration:**

```cpp
int main() {
    stdui::app_config config;
    config.title = "My Application";
    config.window_size = {1280, 720};
    
    stdui::run(make_my_ui(), config);
}
```

**stdui::run() handles:**

- Platform initialization
- Window creation
- Event loop
- Cleanup

User NEVER creates `application` or `window` directly.

### 11.2 Viewport Integration

**Basic 3D Viewport:**

```cpp
#include <stdui/stdui.hpp>
#include <stdui/viewport.hpp>

auto make_3d_app() {
    return stdui::vstack(
        make_toolbar(),
        stdui::hstack(
            make_sidebar(),
            
            // Viewport participates in layout
            stdui::viewport([](auto& vp) {
                // This callback runs on separate thread
                #ifdef __APPLE__
                id<MTLDevice> device = vp.metal_device();
                id<MTLCommandQueue> queue = vp.metal_queue();
                
                // User's 3D rendering code
                id<MTLCommandBuffer> cmd = [queue commandBuffer];
                // ... encode rendering commands ...
                [cmd present:vp.current_drawable()];
                [cmd commit];
                #endif
            }),
            
            make_properties_panel()
        ),
        make_status_bar()
    );
}

int main() {
    stdui::run(make_3d_app());
}
```

**Viewport as First-Class View:**

- Participates in layout (sized by parent)
- Can have modifiers (rounded corners, shadows)
- Positioned like any other view

do we have a better name for viewport? what does swiftui call such things

```cpp
stdui::viewport(render_callback)
    .frame({800, 600})           // Size constraint
    .corner_radius(8.0)          // Rounded corners
    .shadow({0, 4}, 8.0, 0.3)   // Drop shadow
```

---

## 12. Open Questions & Decisions Needed

### 12.1 API Shape

**Question 1:** Entry point style?

**Option A:** Macro

```cpp
STDUI_MAIN(make_my_ui())
```

**Option B:** Function

```cpp
int main() {
    stdui::run(make_my_ui());
}
```

**Recommendation:** Option B (explicit, no magic, C++ idiomatic).

---

**Question 2:** Viewport callback style?

**Option A:** Lambda in constructor

```cpp
stdui::viewport([](auto& vp) { /* render */ })
```

**Option B:** Builder pattern

```cpp
stdui::viewport("my_viewport")
    .on_render([](auto& vp) { /* render */ })
```

**Recommendation:** Option A (concise, matches SwiftUI style).

---

### 12.2 Implementation Choices

**Question 3:** Compositor vs Texture Sharing?

**Decision:** Support BOTH. Default to compositor (Option A), allow texture sharing for advanced use cases.

---

**Question 4:** Render tree invalidation strategy?

**Options:**

- Full rebuild every frame (immutable display list)
- Incremental with dirty tracking
- Damage regions

**Recommendation:** Start with full rebuild (simplest), optimize later if needed. Modern GPUs are fast enough for typical UI complexity.
LGTM
---

**Question 5:** Text atlas strategy?

**Options:**

- Pre-populate common glyphs
- Dynamic population as needed
- Separate atlas per font size

**Recommendation:** Dynamic + caching. Evict unused glyphs after N frames.
sounds good, we can always optimize this later, right?
---

## 13. Success Criteria

### Phase 4 Acceptance

✅ **Visual Demo:**

- Application with 3D viewport (spinning cube/mesh)
- UI chrome (toolbar, sidebar, properties panel, status bar)
- Text rendering (labels, values, code)
- Effects (rounded corners on viewport, drop shadow on panel, blur)
- All running at 60+ FPS

✅ **Visual Parity:**

- Same demo renders identically on macOS, Windows, Linux
- Text rendering matches platform native appearance
- Effects are high-quality (smooth gradients, clean blur)

✅ **Performance:**

- UI rendering <1ms per frame (measured)
- No GPU→CPU→GPU copies (validated via profiler)
- 3D and UI threads don't block each other

✅ **Code Quality:**

- Platform code in separate files (no #ifdef in shared code)
- Comprehensive tests (render tree, effects, text)
- Visual regression tests (golden images)
- Documentation complete

✅ **Completeness:**

- All three platforms working
- Viewport integration proven
- Examples runnable out-of-box

---

## 14. Summary

### Key Architectural Decisions

1. **GPU-based UI rendering** (enables zero-copy composition)
2. **Platform-native backends** (best quality per-platform)
3. **OS compositor for composition** (zero-copy, decoupled frame rates)
4. **Viewport as composable view** (participates in layout)
5. **Separate threading** (UI and 3D don't interfere)
6. **All modern effects** (blur, shadows, gradients)
7. **Platform code isolation** (CMake-controlled, no #ifdef pollution)

### What Changed from v1

- ❌ Removed: CPU-based UI rendering
- ❌ Removed: stdui-provided 3D abstractions
- ✅ Added: GPU-based UI rendering
- ✅ Added: Viewport-as-view in DSL
- ✅ Added: Threading model details
- ✅ Added: Full effects implementation
- ✅ Added: Composition strategies
- ✅ Clarified: Text rendering approach
- ✅ Clarified: Platform backend choices

### Timeline

**Phase 4 Complete:** 15 weeks from start

**Next Phase (Phase 5):**

- Animation system
- Theming with design tokens
- Standard component library

---

**End of Report**

This comprehensive revision incorporates all stakeholder feedback and provides concrete technical direction for Phase 4 implementation.
