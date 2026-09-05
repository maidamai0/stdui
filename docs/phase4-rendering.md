# Phase 4: Rendering Subsystem Design

**Status:** Final Design  
**Date:** 2026-09-04  
**Implementation:** Phase 4.1 - 4.6 (15 weeks estimated)

---

## Executive Summary

This document defines the rendering architecture for stdui Phase 4. The design enables high-quality 2D UI rendering that composes seamlessly with user-provided 3D content (Metal/Vulkan/D3D12) with zero CPU overhead.

### Core Architecture

**Two Independent Rendering Tiers:**

1. **UI Layer** - GPU-accelerated 2D rendering for panels, text, buttons, effects
2. **Content Canvas** - Direct native 3D API access for user rendering (viewport, scenes, video)

**Key Principles:**
- GPU-based UI rendering (enables zero-copy composition with 3D)
- OS compositor blends layers (CALayer/DirectComposition/Wayland)
- Platform-native backends (Metal/Direct2D/Skia-Vulkan)
- Separate threading (UI and 3D threads don't block each other)
- All modern effects (blur, shadows, gradients, rounded corners)

---

## 1. Architecture Overview

### 1.1 The Complete Picture

```
Application
    ↓
┌─────────────────────────────────────────────────┐
│  stdui View Tree (Declarative DSL)              │
│                                                  │
│  vstack(                                         │
│    toolbar(),                                    │
│    hstack(                                       │
│      sidebar(),                                  │
│      canvas([](vp) { /* User's 3D code */ }),   │
│      properties()                                │
│    )                                             │
│  )                                               │
└─────────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────────┐
│  Layout System                                   │
│  - Measure, arrange, build layout tree           │
└─────────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────────┐
│  Render Tree Builder                             │
│  - Convert layout → rendering primitives         │
└─────────────────────────────────────────────────┘
    ↓
┌──────────────────┬──────────────────────────────┐
│  UI Thread       │  Canvas Thread (User's)      │
│                  │                               │
│  UI Rendering    │  3D Rendering                │
│  (Our code)      │  (User's code)               │
│  ↓               │  ↓                            │
│  Metal/D2D/Skia  │  Metal/Vulkan/D3D12          │
│  ↓               │  ↓                            │
│  GPU Layer 1     │  GPU Layer 2                 │
└──────────────────┴──────────────────────────────┘
                   ↓
        ┌──────────────────────┐
        │  OS Compositor       │
        │  (CALayer/DComp/     │
        │   Wayland)           │
        └──────────────────────┘
                   ↓
              Display
```

### 1.2 Why GPU-Based UI?

**The Fundamental Constraint:**

Modern displays expect GPU textures. To composite UI with 3D content without expensive CPU copies, both must be GPU-resident.

```
❌ CPU UI Rendering:
UI (CPU) → System Memory → Upload to GPU (2-4ms!) → Compositor
3D (GPU) → GPU Memory ────────────────────────────→ Compositor
                         ↑ This copy is expensive!

✅ GPU UI Rendering:
UI (GPU) → GPU Memory ─────────────────────────────→ Compositor
3D (GPU) → GPU Memory ─────────────────────────────→ Compositor
                         ↑ Zero copy, instant!
```

**Performance Impact:**
- 1920×1080 RGBA buffer = 8MB
- CPU→GPU upload at 60 FPS = 480 MB/s bandwidth
- Upload time: ~2-4ms per frame
- Total UI budget: <1ms
- **Conclusion:** CPU rendering violates performance requirements

---

## 2. Platform Rendering Backends

### 2.1 Technology Selection

| Platform | UI Rendering | Text Engine | 3D Canvas | Compositor |
|----------|--------------|-------------|-----------|------------|
| macOS | Metal | Core Text | Metal | CALayer |
| Windows | Direct2D | DirectWrite | D3D12 | DirectComposition |
| Linux | Skia+Vulkan | HarfBuzz | Vulkan | Wayland/X11 |

**Why these choices:**
- ✅ All use GPU for UI (zero-copy requirement)
- ✅ Platform-native where available (best quality)
- ✅ Zero dependencies on macOS/Windows (system APIs)
- ✅ Proven production technology (Chrome, Firefox use same stack)

### 2.2 macOS: Metal + Core Text

**Metal for 2D UI:**
```swift
// Render UI to MTLTexture
let uiRenderPass = MTLRenderPassDescriptor()
uiRenderPass.colorAttachments[0].texture = uiLayerTexture

let encoder = commandBuffer.makeRenderCommandEncoder(descriptor: uiRenderPass)!

// Draw UI primitives
encoder.setRenderPipelineState(rectPipeline)
encoder.setVertexBuffer(rectVertices, offset: 0, index: 0)
encoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 6)

// Draw text (textured quads from glyph atlas)
encoder.setRenderPipelineState(textPipeline)
encoder.setFragmentTexture(glyphAtlas, index: 0)
encoder.drawPrimitives(type: .triangle, ...)

encoder.endEncoding()
commandBuffer.commit()
```

**Core Text for Quality:**
```objc
// Rasterize glyphs (CPU, once per glyph)
CTFontRef font = CTFontCreateWithName(CFSTR("SF Pro"), 14.0, NULL);
CGGlyph glyph = CTFontGetGlyphWithName(font, CFSTR("A"));

// Create bitmap context
CGContextRef ctx = CGBitmapContextCreate(..., kCGImageAlphaOnly);
CGContextSetGrayFillColor(ctx, 1.0, 1.0);

// Render glyph
CGPoint position = CGPointMake(0, 0);
CTFontDrawGlyphs(font, &glyph, &position, 1, ctx);

// Extract pixels
uint8_t* pixels = (uint8_t*)CGBitmapContextGetData(ctx);

// Upload to GPU atlas (Metal)
[glyphAtlasTexture replaceRegion:region 
                     mipmapLevel:0 
                       withBytes:pixels 
                     bytesPerRow:width];
```

**Why Core Text:**
- Native macOS text rendering (matches system appearance)
- Excellent quality (Apple-tuned for Retina displays)
- Handles all scripts (emoji, CJK, complex scripts)
- System font fallback

### 2.3 Windows: Direct2D + DirectWrite

**Direct2D for GPU UI:**
```cpp
// Create Direct2D device sharing D3D12 device
ID2D1Factory7* d2d_factory;
D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d_factory);

ID2D1Device6* d2d_device;
d2d_factory->CreateDevice(dxgi_device, &d2d_device);

ID2D1DeviceContext6* d2d_context;
d2d_device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2d_context);

// Create bitmap from D3D12 texture
ID3D12Resource* d3d_texture;  // Created by D3D12
IDXGISurface* dxgi_surface;
d3d_texture->QueryInterface(&dxgi_surface);

ID2D1Bitmap1* d2d_bitmap;
d2d_context->CreateBitmapFromDxgiSurface(dxgi_surface, &props, &d2d_bitmap);

// Render UI
d2d_context->SetTarget(d2d_bitmap);
d2d_context->BeginDraw();

// Draw shapes
D2D1_ROUNDED_RECT rounded = {{x, y, x+w, y+h}, radius, radius};
d2d_context->FillRoundedRectangle(&rounded, brush);

// Draw text (DirectWrite integration)
IDWriteTextLayout* text_layout;
dwrite_factory->CreateTextLayout(text, length, text_format, max_width, max_height, &text_layout);
d2d_context->DrawTextLayout(origin, text_layout, brush);

d2d_context->EndDraw();
```

**Why DirectWrite:**
- Native Windows text (ClearType for LCD displays)
- GPU-accelerated rendering
- Excellent Unicode support
- System integration (matches Windows UI)

### 2.4 Linux: Skia + Vulkan + HarfBuzz

**Why Skia for Linux:**

Linux has no unified system 2D API. Options:
- Cairo: CPU-only (violates zero-copy requirement)
- Qt: Too heavy (~50MB+, entire framework)
- Custom Vulkan: ~10,000 lines of complex code (path tessellation, BiDi text, etc.)

**Skia provides:**
- ✅ Production-tested (Chrome, Android, Flutter)
- ✅ Vulkan backend (GPU-accelerated)
- ✅ Complete 2D API (shapes, paths, text, effects)
- ✅ ~2MB compiled size
- ✅ Maintained by Google

**Implementation:**
```cpp
// Create Vulkan-backed Skia context
GrVkBackendContext vk_backend;
vk_backend.fInstance = vk_instance;
vk_backend.fDevice = vk_device;
vk_backend.fQueue = vk_queue;
// ... more setup

sk_sp<GrDirectContext> skia_context = GrDirectContext::MakeVulkan(vk_backend);

// Create surface from Vulkan image
GrVkImageInfo vk_image_info;
vk_image_info.fImage = vk_image;
vk_image_info.fImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
// ... more setup

GrBackendRenderTarget vk_render_target(width, height, sample_count, vk_image_info);
sk_sp<SkSurface> surface = SkSurface::MakeFromBackendRenderTarget(
    skia_context.get(), vk_render_target, ...
);

// Draw with Skia API
SkCanvas* canvas = surface->getCanvas();

// Shapes
SkPaint paint;
paint.setColor(SK_ColorBLUE);
canvas->drawRoundRect(SkRRect::MakeRectXY(rect, radius, radius), paint);

// Text (HarfBuzz integration)
SkFont font(typeface, 14.0);
canvas->drawString("Hello مرحبا", x, y, font, paint);  // BiDi works!

// Flush to Vulkan
skia_context->flush();
```

**HarfBuzz for shaping:**
- Industry-standard text shaping (used by Chrome, Firefox, Android)
- Excellent Unicode support (all scripts, complex layout)
- FreeType for rasterization
- fontconfig for font selection

---

## 3. Text Rendering Deep Dive

### 3.1 Dynamic Glyph Atlas System

**Overview:**

Glyphs are loaded **on-demand** as text is rendered. First use of a character takes 1-3ms (rasterize + upload), subsequent uses are instant (already in GPU).

**Atlas specifications:**
- Size: 2048×2048 pixels (~4MB GPU memory)
- Capacity: 500-1000 glyphs (depends on font size)
- Strategy: Dynamic allocation, LRU eviction when full
- Sufficient for typical applications (including CJK)

### 3.2 The Three-Stage Process

**Stage 1: Shaping (CPU, per string)**

Convert text string → positioned glyphs
```cpp
"Hello" + Font → [
    {glyph_id: 42, x: 0.0, y: 0.0},    // 'H'
    {glyph_id: 51, x: 8.5, y: 0.0},    // 'e'
    {glyph_id: 55, x: 14.2, y: 0.0},   // 'l'
    {glyph_id: 55, x: 18.0, y: 0.0},   // 'l'
    {glyph_id: 62, x: 21.8, y: 0.0},   // 'o'
]
```

**Stage 2: Rasterization (CPU, once per glyph)**

Glyph outline → bitmap
```cpp
Glyph 'H' at size 14 → 10×12 grayscale bitmap
┌──────────┐
│          │  0 = background
│ ██    ██ │  255 = foreground
│ ██    ██ │  128 = antialiased edge
│ ██████ ██│
│ ██    ██ │
│ ██    ██ │
└──────────┘

Upload to GPU atlas → Cache for reuse
```

**Stage 3: Compositing (GPU, every frame)**

Draw textured quads sampling from atlas
```glsl
// Vertex shader
layout(location = 0) in vec2 position;  // Quad corners
layout(location = 1) in vec2 texcoord;  // Atlas UV

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    frag_texcoord = texcoord;
}

// Fragment shader
layout(binding = 0) uniform sampler2D glyph_atlas;
layout(location = 0) in vec2 frag_texcoord;
layout(location = 0) out vec4 fragColor;

void main() {
    float coverage = texture(glyph_atlas, frag_texcoord).r;
    fragColor = vec4(text_color.rgb, text_color.a * coverage);
}
```

### 3.3 Glyph Atlas Implementation

**Dynamic Atlas:**
```cpp
class GlyphAtlas {
    MTLTexture* atlas_texture;  // 2048×2048 RGBA
    std::unordered_map<GlyphKey, AtlasRegion> cache;
    RectPacker packer;  // Bin-packing algorithm
    
    AtlasRegion get_or_create(GlyphKey key) {
        if (cache.contains(key)) {
            return cache[key];  // Already in atlas, instant!
        }
        
        // Rasterize with platform engine
        Bitmap bitmap = platform_rasterize_glyph(key);
        
        // Allocate space in atlas
        AtlasRegion region = packer.allocate(bitmap.width, bitmap.height);
        
        if (!region.valid()) {
            // Atlas full, evict least-recently-used glyphs
            evict_lru_glyphs(bitmap.width * bitmap.height);
            region = packer.allocate(bitmap.width, bitmap.height);
        }
        
        // Upload to GPU
        atlas_texture.replace_region(region.rect, bitmap.pixels);
        
        cache[key] = region;
        return region;
    }
};
```

**Performance Characteristics:**
- First use of glyph: 1-3ms (rasterize + upload) - **per character**
- Cached glyph: <0.01ms (texture lookup)
- Upload strategy: One character at a time, as needed
- Atlas size: 2048×2048 = ~4MB GPU memory
- Typical cache: 500-1000 glyphs (covers most UI text)
- Eviction: LRU when atlas fills (rare in practice)

**Example - rendering "Hello World":**
```
First frame:
- 'H' not cached → rasterize + upload (2ms)
- 'e' not cached → rasterize + upload (2ms)  
- 'l' not cached → rasterize + upload (2ms)
- 'l' cached → instant
- 'o' not cached → rasterize + upload (2ms)
- ... (8 unique chars × 2ms = ~16ms)

Second frame:
- All characters cached → <0.1ms total
```

**CJK Text Support:**
- Chinese "你好世界" → Load 4 characters on first use
- Don't pre-load all 20,000+ CJK characters
- Users typically see 500-1000 unique characters in UI
- Atlas capacity is sufficient

**Text Color & Gradients:**
- ✅ Solid colors supported
- ✅ Linear/radial gradients supported
- Glyph atlas stores coverage only (shape)
- Color applied at composite time (shader)
- Same glyph can be reused with different colors

**Flexibility:**
- ✅ Any font, any size, any character
- ✅ Unicode fully supported (emoji, CJK, complex scripts)
- ✅ Dynamic font changes at runtime
- ✅ No pre-population needed
- ✅ One atlas sufficient for typical applications

---

## 4. Threading & Synchronization

### 4.1 Two-Thread Architecture

**Why Separate Threads:**

```
Single Thread (Sequential):
├─ Build UI commands (5ms)
├─ Build 3D commands (10ms)
└─ Total: 15ms → 66 FPS max

Two Threads (Parallel):
Thread 1: Build UI commands (5ms)  ┐
Thread 2: Build 3D commands (10ms) ┘ Parallel!
Total: 10ms → 100 FPS possible
```

**Thread Responsibilities:**

```
UI Thread (stdui's code):          Canvas Thread (user's code):
├─ Layout system                   ├─ Scene updates
├─ State management                ├─ Physics
├─ Event handling                  ├─ Animation
├─ UI rendering commands           ├─ 3D rendering commands
└─ Metal/D2D/Vulkan UI queue       └─ Metal/D3D12/Vulkan 3D queue
```

### 4.2 GPU Command Queues

**On macOS (Metal):**
```swift
// Shared device
let device = MTLCreateSystemDefaultDevice()!

// Separate queues (thread-safe)
let ui_queue = device.makeCommandQueue()!      // UI thread
let scene_queue = device.makeCommandQueue()!   // Canvas thread

// UI thread
let ui_buffer = ui_queue.makeCommandBuffer()!
// ... encode UI commands ...
ui_buffer.commit()  // Submit to GPU

// Canvas thread (parallel!)
let scene_buffer = scene_queue.makeCommandBuffer()!
// ... encode 3D commands ...
scene_buffer.commit()  // Submit to GPU

// GPU executes both (may be sequential, but CPU is parallel!)
```

**Command Queue Thread Safety:**
- `MTLDevice`: ✅ Thread-safe
- `MTLCommandQueue`: ✅ Thread-safe
- `MTLCommandBuffer`: ✅ Thread-safe for creation/encoding
- `MTLRenderCommandEncoder`: ❌ NOT thread-safe (but each thread has its own)

**Key Insight:**
Even though GPU might execute commands sequentially, the CPU parallelism is the win:
- Both threads build commands simultaneously
- Faster to submit
- No thread blocks waiting for the other

### 4.3 OS Compositor Integration

**The Compositor's Job:**

```
Without Compositor (Manual):
UI thread renders → framebuffer A
Canvas thread renders → framebuffer B
UI thread reads B, blends with A → final
                ↑ UI waits for canvas!

With Compositor (Automatic):
UI thread → Layer 1 (submit)
Canvas thread → Layer 2 (submit)
            ↓
OS Compositor reads both layers
Blends them (GPU)
            ↓
Display

No waiting! Independent frame rates!
```

**Platform Implementation:**

**macOS - CALayer:**
```swift
// Window has layered content
window.contentView.wantsLayer = true

// Canvas layer (user's 3D)
let canvasLayer = CAMetalLayer()
canvasLayer.device = device
canvasLayer.frame = canvasRect
window.contentView.layer!.addSublayer(canvasLayer)

// UI layer (our 2D)
let uiLayer = CAMetalLayer()
uiLayer.device = device
uiLayer.frame = window.bounds
uiLayer.isOpaque = false  // Transparent where no UI
window.contentView.layer!.addSublayer(uiLayer)

// Core Animation compositor blends them automatically
// Each layer has independent drawable cycle
```

**Windows - DirectComposition:**
```cpp
IDCompositionDevice* dcomp;
IDCompositionTarget* target;

// Create visual tree
IDCompositionVisual* root;
dcomp->CreateVisual(&root);

// Canvas visual
IDCompositionVisual* canvas_visual;
dcomp->CreateVisual(&canvas_visual);
canvas_visual->SetContent(user_swapchain);  // User's D3D12

// UI visual
IDCompositionVisual* ui_visual;
dcomp->CreateVisual(&ui_visual);
ui_visual->SetContent(our_surface);  // Our Direct2D

// Compose
root->AddVisual(canvas_visual, FALSE, NULL);
root->AddVisual(ui_visual, TRUE, NULL);
target->SetRoot(root);
dcomp->Commit();

// DWM compositor blends automatically
```

**Benefits:**
- ✅ Zero-copy (compositor reads GPU memory)
- ✅ Decoupled frame rates (UI 60fps, canvas 120fps → both work)
- ✅ No synchronization overhead (no semaphores/fences needed)
- ✅ OS-optimized (hardware-accelerated composition)

---

## 5. Effects Implementation

### 5.1 Offscreen Rendering

**Why Offscreen Textures:**

Can't apply effects directly to screen - need intermediate storage for multi-pass rendering.

```
Screen (Final Display):
- Can only write once per pixel per frame
- Can't read while writing (hardware limitation)

Offscreen Texture:
- Can write, then read in next pass
- Enables multi-pass effects
```

**Example: Blur Effect**

```
Pass 1: Render content → Offscreen A
Pass 2: Horizontal blur A → Offscreen B  (can read A while writing B)
Pass 3: Vertical blur B → Offscreen C    (can read B while writing C)
Pass 4: Composite C → Screen
```

### 5.2 Dual-Kawase Blur (Production Quality)

**Better than Gaussian blur:**
- 5× faster
- Approximates Gaussian visually
- Used in KDE Plasma, Android, modern games

**Algorithm:**
```
Original (1024×1024)
    ↓ Downsample + blur (4 samples)
512×512
    ↓ Downsample + blur (4 samples)  
256×256
    ↓ Downsample + blur (4 samples)
128×128
    ↓ Upsample + blur (4 samples)
256×256
    ↓ Upsample + blur (4 samples)
512×512
    ↓ Upsample + blur (4 samples)
1024×1024 (blurred)
```

**Fragment Shader (Downsample):**
```glsl
vec4 dual_kawase_down(sampler2D source, vec2 uv, vec2 texel_size) {
    vec4 sum = vec4(0.0);
    
    // 4 diagonal samples
    sum += texture(source, uv + vec2(-1, -1) * texel_size);
    sum += texture(source, uv + vec2( 1, -1) * texel_size);
    sum += texture(source, uv + vec2(-1,  1) * texel_size);
    sum += texture(source, uv + vec2( 1,  1) * texel_size);
    
    return sum * 0.25;
}
```

**Performance:**
- Full-screen 1080p blur: ~1-2ms
- 6 passes total (3 down, 3 up)
- Each pass: ~0.3ms

### 5.3 Drop Shadow

**Complete Implementation:**

```glsl
// Pass 1: Render element to offscreen (RGBA)
// Result: Color + alpha mask

// Pass 2: Extract alpha channel
float shadow_mask = texture(element_texture, uv).a;

// Pass 3: Blur shadow mask (dual-Kawase)
float blurred_shadow = dual_kawase_blur(shadow_mask, uv);

// Pass 4: Offset and composite
vec2 shadow_uv = uv + shadow_offset / screen_size;
float shadow_alpha = texture(blurred_shadow_texture, shadow_uv).r;

vec4 shadow = vec4(0.0, 0.0, 0.0, shadow_alpha * shadow_opacity);
vec4 element = texture(element_texture, uv);

// Blend: shadow under, element on top
fragColor = mix(shadow, element, element.a);
```

### 5.4 Rounded Corners (SDF Method)

**Signed Distance Field for Quality:**

```glsl
// Compute distance to rounded rectangle
float rounded_rect_sdf(vec2 pos, vec2 size, float radius) {
    vec2 q = abs(pos - size * 0.5) - size * 0.5 + radius;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - radius;
}

// Fragment shader for canvas clipping
void main() {
    vec4 canvas_color = texture(canvas_texture, uv);
    
    // Distance to edge
    float dist = rounded_rect_sdf(frag_position, canvas_size, corner_radius);
    
    // Smooth antialiasing (key for quality!)
    float mask = 1.0 - smoothstep(-1.0, 1.0, dist);
    
    fragColor = canvas_color * mask;
}
```

**Quality vs Stencil:**
- Stencil: Hard edges, aliasing visible
- SDF: Smooth antialiasing, perfect quality
- Cost: +0.5ms for full-screen canvas (acceptable)

**Optimization:**
```glsl
// Only compute SDF near edges (center is definitely inside)
float dist_to_edge = min(min(uv.x, 1.0-uv.x), min(uv.y, 1.0-uv.y)) * viewport_size;

if (dist_to_edge < corner_radius + 2.0) {
    // Near edge: compute SDF
    mask = sdf_rounded_rect(...);
} else {
    // Center: definitely inside
    mask = 1.0;
}
// Reduces cost to ~0.2ms (only ~20% of pixels compute SDF)
```

---

## 6. Public API Design

### 6.1 Application Entry Point

**Simple and clean:**
```cpp
#include <stdui/stdui.hpp>

int main() {
    stdui::run(
        stdui::vstack(
            stdui::text("Hello, stdui!"),
            stdui::text("Phase 4 Rendering")
        )
    );
}
```

**With configuration:**
```cpp
int main() {
    stdui::app_config config;
    config.title = "My Application";
    config.window_size = {1280, 720};
    config.background_color = stdui::color{0.1, 0.1, 0.1, 1.0};
    
    stdui::run(make_my_ui(), config);
}
```

**What `stdui::run()` does:**
1. Initialize platform (Metal/D3D/Vulkan)
2. Create window
3. Set up UI renderer
4. Run event loop
5. Cleanup on exit

User never creates `platform`, `window`, or `application` objects directly.

### 6.2 Scene View Integration

**Scene View = View for user-controlled 3D rendering**

Following SwiftUI's naming convention, we use `scene_view` instead of `canvas` (which is too generic and could mean 2D or 3D).

```cpp
#include <stdui/stdui.hpp>
#include <stdui/scene_view.hpp>  // Optional header

auto make_3d_app() {
    return stdui::hstack(
        make_sidebar(),
        
        // Scene view participates in layout like any view
        stdui::scene_view([](stdui::scene_context& ctx) {
            // This callback runs on canvas thread
            
            #ifdef __APPLE__
            id<MTLDevice> device = ctx.metal_device();
            id<MTLCommandQueue> queue = ctx.metal_queue();
            
            // User's rendering code
            id<MTLCommandBuffer> cmd = [queue commandBuffer];
            id<MTLRenderCommandEncoder> encoder = 
                [cmd renderCommandEncoderWithDescriptor:ctx.render_pass()];
            
            // ... encode rendering commands ...
            
            [encoder endEncoding];
            [cmd presentDrawable:ctx.current_drawable()];
            [cmd commit];
            #endif
        }),
        
        make_properties()
    );
}
```

**Scene view as first-class view:**
```cpp
// Apply modifiers like any view
stdui::scene_view(render_callback)
    .frame({800, 600})              // Size constraint
    .corner_radius(8.0)             // Rounded corners (SDF shader)
    .shadow({0, 4}, 8.0, 0.3)       // Drop shadow
    .background(stdui::color::black())
```

**Scene context API - Platform-Specific Headers:**

To avoid `#ifdef` in user code and provide type safety, each platform has its own scene context header:

```cpp
// stdui/scene_view.hpp (base interface)
namespace stdui {
    class scene_context {
    public:
        virtual ~scene_context() = default;
        virtual size scene_size() const = 0;
    };
}

// stdui/scene_view_metal.hpp (macOS only)
#include <Metal/Metal.h>

namespace stdui {
    class metal_scene_context : public scene_context {
    public:
        id<MTLDevice> device() const;
        id<MTLCommandQueue> queue() const;
        MTLRenderPassDescriptor* render_pass() const;
        id<CAMetalDrawable> current_drawable() const;
        
    };
}

// stdui/scene_view_vulkan.hpp (Linux only)
#include <vulkan/vulkan.h>

namespace stdui {
    class vulkan_scene_context : public scene_context {
    public:
        VkDevice device() const;
        VkQueue queue() const;
        VkRenderPass render_pass() const;
        VkImage target_image() const;
    };
}

// stdui/scene_view_d3d12.hpp (Windows only)
#include <d3d12.h>

namespace stdui {
    class d3d12_scene_context : public scene_context {
    public:
        ID3D12Device* device() const;
        ID3D12CommandQueue* queue() const;
        IDXGISwapChain* swapchain() const;
    };
}
```

**User code (platform-specific):**

```cpp
// my_renderer_macos.mm (macOS implementation)
#include <stdui/scene_view_metal.hpp>

void render_scene(stdui::scene_context& ctx) {
    auto& metal = static_cast<stdui::metal_scene_context&>(ctx);
    id<MTLDevice> device = metal.device();
    id<MTLCommandQueue> queue = metal.queue();
    
    // Metal rendering code...
}

// my_renderer_linux.cpp (Linux implementation)
#include <stdui/scene_view_vulkan.hpp>

void render_scene(stdui::scene_context& ctx) {
    auto& vulkan = static_cast<stdui::vulkan_scene_context&>(ctx);
    VkDevice device = vulkan.device();
    VkQueue queue = vulkan.queue();
    
    // Vulkan rendering code...
}
```

**Benefits:**
- ✅ No `#ifdef` in user code (platform-specific headers instead)
- ✅ Type-safe (no void* casts)
- ✅ Natural usage (3D code is inherently platform-specific)
- ✅ Extensible (easy to add WebGPU later)
```

### 6.3 Header Organization

**Core (always included):**
```cpp
#include <stdui/stdui.hpp>  // Umbrella header

// Includes:
// - expressions.hpp (DSL primitives)
// - layout.hpp (measurement, arrangement)
// - state.hpp (component state)
// - component.hpp (stateful components)
// - geometry.hpp (rect, size, point)
```

**Optional (when needed):**
```cpp
#include <stdui/scene_view.hpp>         // Base scene view API
#include <stdui/scene_view_metal.hpp>   // macOS Metal (platform-specific)
#include <stdui/scene_view_vulkan.hpp>  // Linux Vulkan (platform-specific)
#include <stdui/scene_view_d3d12.hpp>   // Windows D3D12 (platform-specific)
#include <stdui/effects.hpp>            // Blur, shadows, gradients
#include <stdui/animation.hpp>          // Phase 5 (future)
```

---

## 7. Code Organization

### 7.1 Directory Structure

```
stdui/
├── include/
│   └── stdui/
│       ├── stdui.hpp                  # Umbrella header
│       ├── scene_view.hpp             # Base scene view API
│       ├── scene_view_metal.hpp       # Optional: macOS Metal
│       ├── scene_view_vulkan.hpp      # Optional: Linux Vulkan
│       ├── scene_view_d3d12.hpp       # Optional: Windows D3D12
│       ├── effects.hpp                # Optional: effects API
│       └── rendering.hpp              # Platform-agnostic interface
│
├── src/
│   ├── rendering/
│   │   ├── render_tree.cpp            # Common render tree builder
│   │   └── glyph_atlas.cpp            # Common glyph atlas
│   │
│   └── platform/
│       ├── macos/
│       │   ├── metal_renderer.mm      # Metal 2D renderer
│       │   ├── metal_scene_view.mm    # Metal scene context
│       │   └── core_text_shaper.mm    # Text shaping
│       │
│       ├── windows/
│       │   ├── direct2d_renderer.cpp  # Direct2D renderer
│       │   ├── d3d12_scene_view.cpp   # D3D12 scene context
│       │   └── directwrite_shaper.cpp # Text shaping
│       │
│       └── linux/
│           ├── skia_renderer.cpp      # Skia+Vulkan renderer
│           ├── vulkan_scene_view.cpp  # Vulkan scene context
│           └── harfbuzz_shaper.cpp    # Text shaping
│
├── tests/
│   ├── rendering/
│   │   ├── render_tree_tests.cpp
│   │   ├── glyph_atlas_tests.cpp
│   │   └── effects_tests.cpp
│   │
│   └── visual/
│       ├── golden/                    # Reference images
│       │   ├── macos/
│       │   ├── windows/
│       │   └── linux/
│       └── visual_tests.cpp
│
└── examples/
    ├── 01_hello_ui.cpp
    ├── 02_with_canvas.cpp
    └── 03_full_application.cpp
```

### 7.2 CMake Configuration

**Main CMakeLists.txt (clean and simple):**

```cmake
cmake_minimum_required(VERSION 3.24)
project(stdui VERSION 0.1.0 LANGUAGES CXX)

# Core library (header-only)
add_library(stdui INTERFACE)
add_library(stdui::stdui ALIAS stdui)

target_include_directories(stdui INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(stdui INTERFACE cxx_std_20)

# Platform-specific rendering (dispatched to separate files)
include(cmake/rendering.cmake)

# Testing, docs, install
include(CTest)
if(BUILD_TESTING)
    include(cmake/tests.cmake)
endif()
include(cmake/docs.cmake)
include(cmake/install.cmake)
```

**cmake/rendering.cmake (platform dispatch):**

```cmake
# Rendering implementation library
add_library(stdui_rendering STATIC
    src/rendering/render_tree.cpp
    src/rendering/glyph_atlas.cpp
)

target_link_libraries(stdui_rendering PUBLIC stdui)

# Platform-specific configuration (dispatched)
if(APPLE)
    include(cmake/platform/macos.cmake)
elseif(WIN32)
    include(cmake/platform/windows.cmake)
elseif(UNIX)
    include(cmake/platform/linux.cmake)
endif()

# Link rendering to main target
target_link_libraries(stdui INTERFACE stdui_rendering)
```

**cmake/platform/macos.cmake:**

```cmake
target_sources(stdui_rendering PRIVATE
    src/platform/macos/metal_renderer.mm
    src/platform/macos/metal_scene_view.mm
    src/platform/macos/core_text_shaper.mm
)

target_link_libraries(stdui_rendering PRIVATE
    "-framework Metal"
    "-framework MetalKit"
    "-framework CoreText"
    "-framework QuartzCore"
)
```

**cmake/platform/windows.cmake:**

```cmake
target_sources(stdui_rendering PRIVATE
    src/platform/windows/direct2d_renderer.cpp
    src/platform/windows/d3d12_scene_view.cpp
    src/platform/windows/directwrite_shaper.cpp
)

target_link_libraries(stdui_rendering PRIVATE
    d2d1.lib
    dwrite.lib
    d3d12.lib
    dxgi.lib
)
```

**cmake/platform/linux.cmake:**

```cmake
find_package(Vulkan REQUIRED)
find_package(Skia REQUIRED)
find_package(HarfBuzz REQUIRED)

target_sources(stdui_rendering PRIVATE
    src/platform/linux/skia_renderer.cpp
    src/platform/linux/vulkan_scene_view.cpp
    src/platform/linux/harfbuzz_shaper.cpp
)

target_link_libraries(stdui_rendering PRIVATE
    Vulkan::Vulkan
    Skia::Skia
    HarfBuzz::HarfBuzz
)
```

**User perspective (single target):**

```cmake
find_package(stdui CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE stdui::stdui)
# Done! Backend selected automatically by platform
```

**Key principles:**
- ✅ Clean main CMakeLists.txt
- ✅ Platform dispatch in separate cmake/ files
- ✅ One target for users: `stdui::stdui`
- ✅ No `#ifdef` in source code

---

## 8. Implementation Plan

### Phase 4.1: Render Tree Construction (2 weeks)

**Deliverables:**
- `render_node` structure (rectangles, text, paths, effects)
- `render_tree_builder` (layout tree → render tree)
- Culling (offscreen elements)
- Transform and clip computation
- Unit tests (headless)

### Phase 4.2: Metal Backend - macOS (3 weeks)

**Deliverables:**
- `metal_renderer` implementation
- Core Text integration
- Glyph atlas system
- Basic shapes (rects, rounded rects, paths)
- Text rendering
- Blur and shadow effects
- Visual regression tests

### Phase 4.3: Direct2D Backend - Windows (3 weeks)

**Deliverables:**
- `direct2d_renderer` implementation
- DirectWrite integration
- D3D11/12 interop
- DirectComposition integration
- Visual parity with Metal
- Cross-platform test suite

### Phase 4.4: Skia/Vulkan Backend - Linux (3 weeks)

**Deliverables:**
- `skia_vulkan_renderer` implementation
- HarfBuzz integration
- Wayland and X11 support
- Visual parity tests
- Complete cross-platform coverage

### Phase 4.5: Scene View Integration (2 weeks)

**Deliverables:**
- `scene_view` view implementation
- Scene context API (Metal/Vulkan/D3D12)
- Platform-specific headers (no #ifdef in user code)
- OS compositor integration
- Thread management
- Example: Spinning cube with UI chrome

### Phase 4.6: Polish & Performance (2 weeks)

**Deliverables:**
- VSync and frame scheduling
- Performance profiling and optimization
- GPU/CPU microbenchmarks (Google Benchmark)
- Frame timing measurements (Metal/Vulkan timestamps)
- HiDPI/Retina support verification
- Window resize handling
- Production documentation
- Code coverage >90% verified

**Total: 15 weeks (3.75 months)**

---

## 9. Success Criteria

### Functional Requirements

✅ **Visual Demo Application:**
- 3D canvas (spinning mesh/scene)
- UI chrome (toolbar, sidebar, properties, status bar)
- Text rendering (labels, values, monospace)
- Effects (blur, shadows, rounded corners)
- 60+ FPS sustained

✅ **Cross-Platform:**
- Identical rendering on macOS, Windows, Linux
- Platform-native text appearance
- All effects functional

✅ **Performance:**
- UI rendering <1ms per frame
- Zero GPU→CPU→GPU copies
- UI and canvas threads don't block
- Measured and verified via profiling

### Quality Requirements

✅ **Text Quality:**
- Matches platform native appearance
- Supports all Unicode (emoji, CJK, complex scripts)
- Smooth antialiasing
- Any font, any size

✅ **Effects Quality:**
- Smooth gradients
- Clean blur (no artifacts)
- Sharp rounded corners with AA
- Production-ready visual quality

### Code Quality

✅ **Architecture:**
- Platform code in separate files (no `#ifdef` pollution)
- Clean separation of concerns
- Minimal public API surface

✅ **Testing:**
- Unit tests (render tree, glyph atlas)
- Visual regression tests (golden images)
- Cross-platform test suite
- >90% code coverage

✅ **Documentation:**
- Complete API documentation
- Implementation guides
- Example applications
- Performance characteristics documented

---

## 10. Summary

### Key Architectural Decisions

1. **GPU-based UI rendering** - Required for zero-copy composition
2. **Platform-native backends** - Best quality per platform (Metal/Direct2D/Skia)
3. **OS compositor** - CALayer/DirectComposition/Wayland for layer blending
4. **Separate threading** - UI and canvas threads operate independently
5. **Dynamic glyph atlas** - Flexible text rendering, any font/size
6. **SDF rounded corners** - High quality antialiasing
7. **Dual-Kawase blur** - Production-quality effects
8. **Canvas-as-view** - Participates in layout like any UI element

### What's Different from Other Frameworks

**vs Dear ImGui:**
- ✅ Declarative DSL (not immediate mode)
- ✅ Production-quality text (platform engines, not bitmap fonts)
- ✅ Full effects support (blur, shadows, gradients)

**vs Qt:**
- ✅ Lighter weight (~2MB vs 50MB+)
- ✅ Modern C++20 (not C++03)
- ✅ Direct 3D API access (no abstraction overhead)

**vs Native Frameworks (Cocoa/Win32):**
- ✅ Declarative (not imperative)
- ✅ Cross-platform
- ✅ Integrated 3D canvas support

### Next Phase (Phase 5)

After Phase 4 completion:
- Animation system (interpolation, timing, spring physics)
- Design token system (theming, dark/light mode)
- Standard component library (Button, TextField, etc.)
- Accessibility support

---

**End of Design Document**

This design provides a complete, implementable architecture for Phase 4 rendering. All technical questions have been addressed, and the path forward is clear.
