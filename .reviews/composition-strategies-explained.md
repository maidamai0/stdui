# Composition Strategies Explained

## Your Questions

> "I dont quite understand this, is os-compositor something like desktop windows manager on windows? I thought it can only control the native windows, if we rendering all the components ourselves, how it will interface our ui rendering?"

> "what is texture-sharing, do you mean we modify the system texture? does all platform provide the API? it seems dangerous"

Let me explain both strategies clearly.

---

## Strategy 1: OS Compositor (RECOMMENDED)

### What It Actually Is

**Yes, you're right** - the OS compositor is like Desktop Window Manager (DWM) on Windows, or the compositor in macOS/Linux.

**But here's the key insight:** Modern compositors can blend **multiple GPU surfaces/layers within a single window**, not just multiple windows.

### How It Works

**Think of it like layers in Photoshop:**

```
Single OS Window
├─ Layer 1 (back):  3D Viewport (GPU texture)
└─ Layer 2 (front): 2D UI (GPU texture)

OS Compositor blends these layers together
```

**Technical Details Per Platform:**

#### macOS: CALayer Composition

```objc
NSWindow* window = ...;

// Window has a root CALayer
CALayer* root = window.contentView.layer;

// Add 3D layer (user's Metal rendering)
CAMetalLayer* viewportLayer = [CAMetalLayer layer];
viewportLayer.device = metalDevice;
viewportLayer.frame = CGRectMake(200, 0, 800, 600);  // Positioned by layout
[root insertSublayer:viewportLayer atIndex:0];  // Back layer

// Add UI layer (our 2D UI)
CAMetalLayer* uiLayer = [CAMetalLayer layer];
uiLayer.device = metalDevice;
uiLayer.frame = window.bounds;
[root addSublayer:uiLayer];  // Front layer

// Core Animation automatically composites these layers
// User renders 3D to viewportLayer
// We render UI to uiLayer
// Core Animation blends them on the GPU (zero copy!)
```

**Key:** Both layers are in the **same window**, compositor blends them automatically.

#### Windows: DirectComposition

```cpp
// Create DirectComposition device (wraps DWM compositor)
IDCompositionDevice* dcomp;
DCompositionCreateDevice(..., &dcomp);

// Create visual tree (like CALayer hierarchy)
IDCompositionVisual* root;
dcomp->CreateVisual(&root);

// 3D visual (user's D3D12 swapchain)
IDCompositionVisual* viewportVisual;
dcomp->CreateVisual(&viewportVisual);
viewportVisual->SetContent(userD3D12Swapchain);  // User's 3D rendering
viewportVisual->SetOffsetX(200);  // Position from layout
root->AddVisual(viewportVisual, FALSE, NULL);

// UI visual (our Direct2D surface)
IDCompositionVisual* uiVisual;
dcomp->CreateVisual(&uiVisual);
uiVisual->SetContent(ourD2DSurface);  // Our 2D UI
root->AddVisual(uiVisual, TRUE, NULL);  // On top

// Commit to DWM - it composites on GPU
dcomp->Commit();
```

**Key:** DWM compositor blends both surfaces **within your single window**.

#### Linux: Wayland Subsurfaces

```cpp
// Main window surface
wl_surface* mainSurface = ...;

// 3D subsurface (user's Vulkan rendering)
wl_surface* viewportSurface = wl_compositor_create_surface(...);
wl_subsurface* viewportSub = wl_subcompositor_get_subsurface(
    viewportSurface, mainSurface
);
wl_subsurface_set_position(viewportSub, 200, 0);  // Position

// UI subsurface (our Vulkan UI rendering)
wl_surface* uiSurface = wl_compositor_create_surface(...);
wl_subsurface* uiSub = wl_subcompositor_get_subsurface(
    uiSurface, mainSurface
);
wl_subsurface_place_above(uiSub, viewportSurface);  // Layering

// Wayland compositor blends both surfaces
```

### Why This Works

**You are NOT modifying system textures.** Each layer:
1. Your code renders to a GPU texture/surface
2. You tell the OS "this texture is a layer in my window"
3. OS compositor reads both textures and blends them (on GPU, zero copy)

**Benefits:**
- ✅ **Zero CPU involvement** - GPU→Compositor→Display
- ✅ **No framebuffer copies** - Compositor reads GPU memory directly
- ✅ **Independent frame rates** - 3D can run at 120Hz, UI at 60Hz
- ✅ **Hardware-optimized** - OS compositor is highly optimized

**Drawbacks:**
- ❌ Cannot apply UI effects that blend across layers (e.g., "blur the 3D content behind this panel")
- ❌ Slightly more complex setup

---

## Strategy 2: Texture Sharing

### What It Actually Is

**NOT modifying system textures!** Here's what actually happens:

```
3D Thread                          UI Thread
    ↓                                  ↓
Render to GPU texture A         Read texture A
    ↓                           ↓
Signal: "frame done"            Draw A at viewport position
                                Draw UI on top
                                    ↓
                                Final framebuffer
                                    ↓
                                Display
```

### How It Works

**On the same GPU device, threads can share textures:**

#### Metal (macOS)

```objc
// 3D Thread (user's code)
id<MTLTexture> viewportTexture = [device newTextureWithDescriptor:...];

// Render 3D scene to viewportTexture
id<MTLRenderCommandEncoder> encoder = ...;
[encoder setRenderPipelineState:scenePipeline];
// ... draw 3D scene into viewportTexture ...
[encoder endEncoding];

// Signal completion
id<MTLSharedEvent> frameReady = [device newSharedEvent];
[commandBuffer encodeSignalEvent:frameReady value:frameNumber];
[commandBuffer commit];

// UI Thread (our code)
// Wait for 3D frame
[commandBuffer encodeWaitForEvent:frameReady value:frameNumber];

// Draw UI, sample viewportTexture where viewport should be
[uiEncoder setFragmentTexture:viewportTexture atIndex:0];
// Draw a quad textured with the 3D content
[uiEncoder drawPrimitives:...];
// Draw UI on top
[uiEncoder endEncoding];
```

**Key:** Same MTLDevice, both threads can access the same texture safely with synchronization.

#### Vulkan (Linux)

```cpp
// 3D Thread (user's code)
VkImage viewportImage;
vkCreateImage(device, ..., &viewportImage);

// Render 3D to viewportImage
VkCommandBuffer sceneCmd = ...;
// ... record 3D rendering commands ...
vkEndCommandBuffer(sceneCmd);

// Submit with semaphore
VkSemaphore frameReady;
VkSubmitInfo submitInfo = {
    .commandBufferCount = 1,
    .pCommandBuffers = &sceneCmd,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &frameReady,  // Signal when done
};
vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

// UI Thread (our code)
VkCommandBuffer uiCmd = ...;
// Sample viewportImage as a texture
// ... record UI rendering commands ...
vkEndCommandBuffer(uiCmd);

// Submit, wait for frameReady semaphore
VkSubmitInfo uiSubmit = {
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &frameReady,  // Wait for 3D
    .commandBufferCount = 1,
    .pCommandBuffers = &uiCmd,
};
vkQueueSubmit(graphicsQueue, 1, &uiSubmit, VK_NULL_HANDLE);
```

#### D3D12 (Windows)

```cpp
// 3D Thread (user's code)
ID3D12Resource* viewportTexture;
device->CreateCommittedResource(..., &viewportTexture);

// Render 3D to viewportTexture
ID3D12GraphicsCommandList* sceneList = ...;
// ... record 3D rendering commands ...
sceneList->Close();
sceneQueue->ExecuteCommandLists(1, ...);

// Signal fence
ID3D12Fence* frameReady;
sceneQueue->Signal(frameReady, frameNumber);

// UI Thread (our code)
// Wait for 3D frame
uiQueue->Wait(frameReady, frameNumber);

// Draw UI, use viewportTexture as shader resource
ID3D12GraphicsCommandList* uiList = ...;
uiList->SetGraphicsRootDescriptorTable(0, viewportTextureSRV);  // Bind as texture
// Draw quad with 3D content
// Draw UI on top
uiList->Close();
uiQueue->ExecuteCommandLists(1, ...);
```

### Why This Works

**Shared GPU memory:** Both threads render to the same GPU device, textures live in GPU memory, no CPU copy needed.

**Synchronization primitives (events/semaphores/fences)** ensure:
1. UI thread doesn't read texture while 3D thread is writing
2. 3D thread doesn't overwrite texture UI thread is reading

**Benefits:**
- ✅ **Can apply UI effects that blend with 3D** (e.g., rounded corners on viewport, blur over 3D)
- ✅ **Single final framebuffer** (simpler mental model)

**Drawbacks:**
- ❌ **Synchronization overhead** (semaphore/fence latency ~0.1-0.5ms)
- ❌ **Coupled frame rates** (UI must wait for 3D frame if using latest)
- ❌ **More complex if 3D lags** (need ring buffer of textures)

---

## Which Should We Use?

### Recommendation: **Strategy 1 (OS Compositor) as Default**

**Why:**
- ✅ Better performance (no sync overhead)
- ✅ Decoupled frame rates (3D drops don't affect UI)
- ✅ Simpler (OS handles composition)
- ✅ All platforms support it (CALayer, DirectComposition, Wayland subsurfaces)

**Use Strategy 2 (Texture Sharing) only when:**
- User wants rounded corners on the 3D viewport
- User wants blur/shadow effects on the viewport itself
- User needs to blend UI and 3D content (advanced use case)

### API Design

```cpp
// Default: OS compositor (separate layers)
auto viewport = stdui::viewport([](auto& vp) {
    // User's 3D rendering
    // Renders to separate layer
    // OS compositor blends with UI
});

// Advanced: Texture sharing (explicit)
auto viewport = stdui::viewport_texture([](auto& vp) -> id<MTLTexture> {
    // User renders and returns the texture
    return render_3d_scene_to_texture(vp.device());
});
// We sample this texture in UI rendering
```

---

## SwiftUI Equivalent

**You asked: "what does SwiftUI call this?"**

SwiftUI doesn't expose 3D viewport integration at this level because:
1. SwiftUI focuses on UI, not 3D graphics
2. For 3D, you use **SceneKit** or **RealityKit** views

**SwiftUI Example:**
```swift
struct ContentView: View {
    var body: some View {
        HStack {
            Sidebar()
            
            // This is like our viewport
            SceneView(scene: scene)  // Metal-accelerated 3D
            
            PropertiesPanel()
        }
    }
}
```

**Behind the scenes:** `SceneView` is a `UIView`/`NSView` with a `CAMetalLayer`, composed by Core Animation with SwiftUI's own Metal rendering.

**So our equivalent would be:**
```cpp
stdui::hstack(
    make_sidebar(),
    stdui::metal_view(render_callback),  // or scene_view?
    make_properties()
)
```

**Better names than "viewport":**
- `metal_view` (explicit about technology)
- `scene_view` (matches SwiftUI)
- `render_view` (generic)
- `canvas` (familiar to developers)

**My recommendation:** `stdui::canvas()` - clear, concise, familiar from HTML Canvas and other frameworks.

---

## Decision Summary

**Composition Strategy:**
- ✅ **Default: OS Compositor** (CALayer/DirectComposition/Wayland subsurfaces)
- ✅ **Advanced: Texture Sharing** (when user needs UI effects over 3D)
- ✅ **Support both**, API makes the choice explicit

**Naming:**
- ✅ `stdui::canvas(callback)` instead of `stdui::viewport(callback)`

**Entry Point:**
- ✅ `int main() { stdui::run(ui); }` - No macros

**Render Tree & Text Atlas:**
- ✅ Follow Flutter/SwiftUI best practices (research needed)
- ✅ Dynamic text atlas with eviction (not pre-populated)

---

Ready to update the design doc with these clarifications?
