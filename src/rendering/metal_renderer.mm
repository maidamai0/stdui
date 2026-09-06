// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#ifdef __APPLE__

#include "stdui/rendering/metal_renderer.hpp"
#include "stdui/rendering/glyph_atlas.hpp"
#include <simd/simd.h>
#include <vector>

namespace stdui::rendering {

// Vertex structure for Metal shaders
struct vertex {
    simd_float2 position;
    simd_float4 color;
    simd_float2 texcoord;
};

// Uniforms passed to shaders
struct uniforms {
    simd_float4x4 projection_matrix;
};

struct metal_renderer::impl {
    // Metal objects
    id<MTLDevice> device;
    id<MTLCommandQueue> command_queue;
    CAMetalLayer* metal_layer;

    // Current frame state
    id<MTLCommandBuffer> command_buffer;
    id<MTLRenderCommandEncoder> render_encoder;
    id<CAMetalDrawable> current_drawable;
    MTLRenderPassDescriptor* render_pass_descriptor;

    // Pipeline states
    id<MTLRenderPipelineState> rectangle_pipeline;
    id<MTLRenderPipelineState> text_pipeline;

    // Buffers
    id<MTLBuffer> vertex_buffer;
    id<MTLBuffer> uniform_buffer;
    std::vector<vertex> vertices;

    // Text rendering
    std::unique_ptr<glyph_atlas> atlas;

    // Viewport
    size viewport;

    impl(size viewport_size, CAMetalLayer* layer)
        : metal_layer(layer)
        , viewport(viewport_size)
    {
        // Get Metal device
        device = MTLCreateSystemDefaultDevice();
        if (!device) {
            throw std::runtime_error("Failed to create Metal device");
        }

        // Create command queue
        command_queue = [device newCommandQueue];

        // Configure metal layer
        metal_layer.device = device;
        metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        metal_layer.framebufferOnly = YES;
        metal_layer.drawableSize = CGSizeMake(viewport_size.width, viewport_size.height);

        // Create render pass descriptor
        render_pass_descriptor = [MTLRenderPassDescriptor new];
        render_pass_descriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        render_pass_descriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        render_pass_descriptor.colorAttachments[0].clearColor = MTLClearColorMake(1.0, 1.0, 1.0, 1.0);

        // Create pipeline states
        create_pipelines();

        // Create uniform buffer
        uniform_buffer = [device newBufferWithLength:sizeof(uniforms)
                                             options:MTLResourceStorageModeShared];

        // Initialize glyph atlas (2048x2048 texture)
        atlas = std::make_unique<glyph_atlas>(2048, 2048);
        atlas->set_metal_device(device);

        // Initialize projection matrix
        update_projection_matrix();
    }

    ~impl() {
        // Metal objects are reference counted (ARC), no manual cleanup needed
    }

    void create_pipelines() {
        // Create shader library
        NSError* error = nil;
        NSString* shader_source = @R"(
            #include <metal_stdlib>
            using namespace metal;

            struct VertexIn {
                float2 position [[attribute(0)]];
                float4 color [[attribute(1)]];
                float2 texcoord [[attribute(2)]];
            };

            struct VertexOut {
                float4 position [[position]];
                float4 color;
                float2 texcoord;
            };

            struct Uniforms {
                float4x4 projection_matrix;
            };

            vertex VertexOut vertex_main(
                VertexIn in [[stage_in]],
                constant Uniforms& uniforms [[buffer(1)]]
            ) {
                VertexOut out;
                out.position = uniforms.projection_matrix * float4(in.position, 0.0, 1.0);
                out.color = in.color;
                out.texcoord = in.texcoord;
                return out;
            }

            fragment float4 fragment_main(VertexOut in [[stage_in]]) {
                return in.color;
            }
        )";

        id<MTLLibrary> library = [device newLibraryWithSource:shader_source
                                                     options:nil
                                                       error:&error];
        if (!library) {
            NSLog(@"Failed to create shader library: %@", error);
            throw std::runtime_error("Failed to create shader library");
        }

        id<MTLFunction> vertex_function = [library newFunctionWithName:@"vertex_main"];
        id<MTLFunction> fragment_function = [library newFunctionWithName:@"fragment_main"];

        // Create vertex descriptor
        MTLVertexDescriptor* vertex_descriptor = [MTLVertexDescriptor new];
        vertex_descriptor.attributes[0].format = MTLVertexFormatFloat2;
        vertex_descriptor.attributes[0].offset = 0;
        vertex_descriptor.attributes[0].bufferIndex = 0;

        vertex_descriptor.attributes[1].format = MTLVertexFormatFloat4;
        vertex_descriptor.attributes[1].offset = sizeof(simd_float2);
        vertex_descriptor.attributes[1].bufferIndex = 0;

        vertex_descriptor.attributes[2].format = MTLVertexFormatFloat2;
        vertex_descriptor.attributes[2].offset = sizeof(simd_float2) + sizeof(simd_float4);
        vertex_descriptor.attributes[2].bufferIndex = 0;

        vertex_descriptor.layouts[0].stride = sizeof(vertex);
        vertex_descriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

        // Create pipeline state
        MTLRenderPipelineDescriptor* pipeline_descriptor = [MTLRenderPipelineDescriptor new];
        pipeline_descriptor.vertexFunction = vertex_function;
        pipeline_descriptor.fragmentFunction = fragment_function;
        pipeline_descriptor.vertexDescriptor = vertex_descriptor;
        pipeline_descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        pipeline_descriptor.colorAttachments[0].blendingEnabled = YES;
        pipeline_descriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        pipeline_descriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        pipeline_descriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        pipeline_descriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
        pipeline_descriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pipeline_descriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

        rectangle_pipeline = [device newRenderPipelineStateWithDescriptor:pipeline_descriptor error:&error];
        if (!rectangle_pipeline) {
            NSLog(@"Failed to create pipeline state: %@", error);
            throw std::runtime_error("Failed to create pipeline state");
        }

        text_pipeline = rectangle_pipeline;  // Use same pipeline for now
    }

    void update_projection_matrix() {
        // Create orthographic projection matrix
        float left = 0.0f;
        float right = static_cast<float>(viewport.width);
        float bottom = static_cast<float>(viewport.height);
        float top = 0.0f;
        float near_z = -1.0f;
        float far_z = 1.0f;

        simd_float4x4 projection = {
            simd_make_float4(2.0f / (right - left), 0, 0, 0),
            simd_make_float4(0, 2.0f / (top - bottom), 0, 0),
            simd_make_float4(0, 0, 1.0f / (far_z - near_z), 0),
            simd_make_float4(
                (left + right) / (left - right),
                (top + bottom) / (bottom - top),
                near_z / (near_z - far_z),
                1.0f
            )
        };

        uniforms* uniform_data = static_cast<uniforms*>([uniform_buffer contents]);
        uniform_data->projection_matrix = projection;
    }

    void add_rectangle(const rect& bounds, const color& fill_color, float corner_radius) {
        // Simple rectangle without rounded corners for now
        float x = static_cast<float>(bounds.origin.x);
        float y = static_cast<float>(bounds.origin.y);
        float w = static_cast<float>(bounds.extent.width);
        float h = static_cast<float>(bounds.extent.height);

        simd_float4 color_vec = simd_make_float4(fill_color.r, fill_color.g, fill_color.b, fill_color.a);

        // Two triangles for rectangle
        vertices.push_back({{x, y}, color_vec, {0, 0}});
        vertices.push_back({{x + w, y}, color_vec, {1, 0}});
        vertices.push_back({{x, y + h}, color_vec, {0, 1}});

        vertices.push_back({{x + w, y}, color_vec, {1, 0}});
        vertices.push_back({{x + w, y + h}, color_vec, {1, 1}});
        vertices.push_back({{x, y + h}, color_vec, {0, 1}});
    }

    void flush_vertices() {
        if (vertices.empty()) {
            return;
        }

        // Create or update vertex buffer
        size_t buffer_size = vertices.size() * sizeof(vertex);
        if (!vertex_buffer || [vertex_buffer length] < buffer_size) {
            vertex_buffer = [device newBufferWithLength:buffer_size
                                                options:MTLResourceStorageModeShared];
        }

        memcpy([vertex_buffer contents], vertices.data(), buffer_size);

        // Draw
        [render_encoder setRenderPipelineState:rectangle_pipeline];
        [render_encoder setVertexBuffer:vertex_buffer offset:0 atIndex:0];
        [render_encoder setVertexBuffer:uniform_buffer offset:0 atIndex:1];
        [render_encoder drawPrimitives:MTLPrimitiveTypeTriangle
                           vertexStart:0
                           vertexCount:vertices.size()];

        vertices.clear();
    }
};

metal_renderer::metal_renderer(size viewport_size, CAMetalLayer* metal_layer)
    : impl_(std::make_unique<impl>(viewport_size, metal_layer))
{
}

metal_renderer::~metal_renderer() = default;

void metal_renderer::begin_frame() {
    impl_->vertices.clear();

    // Get next drawable
    impl_->current_drawable = [impl_->metal_layer nextDrawable];
    if (!impl_->current_drawable) {
        return;
    }

    // Create command buffer
    impl_->command_buffer = [impl_->command_queue commandBuffer];

    // Setup render pass
    impl_->render_pass_descriptor.colorAttachments[0].texture = impl_->current_drawable.texture;

    // Create render encoder
    impl_->render_encoder = [impl_->command_buffer renderCommandEncoderWithDescriptor:impl_->render_pass_descriptor];
}

void metal_renderer::render(const render_tree& tree) {
    if (!impl_->render_encoder) {
        return;
    }

    if (tree.root()) {
        render_node_internal(*tree.root());
    }

    // Flush any remaining vertices
    impl_->flush_vertices();
}

void metal_renderer::end_frame() {
    if (!impl_->render_encoder) {
        return;
    }

    [impl_->render_encoder endEncoding];
    [impl_->command_buffer presentDrawable:impl_->current_drawable];
    [impl_->command_buffer commit];

    impl_->render_encoder = nil;
    impl_->current_drawable = nil;
    impl_->command_buffer = nil;
}

void metal_renderer::resize(size new_size) {
    impl_->viewport = new_size;
    impl_->metal_layer.drawableSize = CGSizeMake(new_size.width, new_size.height);
    impl_->update_projection_matrix();
}

size metal_renderer::viewport_size() const {
    return impl_->viewport;
}

void metal_renderer::render_node_internal(const render_node& node) {
    if (!node.is_visible()) {
        return;
    }

    switch (node.type()) {
        case render_node_type::rectangle:
            render_rectangle(static_cast<const rectangle_node&>(node));
            break;
        case render_node_type::text:
            render_text(static_cast<const text_node&>(node));
            break;
        case render_node_type::path:
            render_path(static_cast<const path_node&>(node));
            break;
        case render_node_type::image:
            render_image(static_cast<const image_node&>(node));
            break;
        case render_node_type::group:
            render_group(static_cast<const group_node&>(node));
            break;
        case render_node_type::effect:
            render_effect(static_cast<const effect_node&>(node));
            break;
        case render_node_type::scene_view:
            render_scene_view(static_cast<const scene_view_node&>(node));
            break;
    }
}

void metal_renderer::render_rectangle(const rectangle_node& node) {
    const auto& props = node.properties();
    impl_->add_rectangle(props.bounds, props.fill_color, props.corner_radius);
}

void metal_renderer::render_text(const text_node& node) {
    const auto& props = node.properties();

    // Rasterize glyphs and add to vertex buffer
    float x = props.position.x;
    float y = props.position.y;

    for (char c : props.content) {
        const glyph_info* glyph = impl_->atlas->get_glyph(
            static_cast<uint32_t>(c),
            props.font_family,
            props.font_size
        );

        if (!glyph) {
            continue;
        }

        // Create quad for glyph with texture coordinates
        float gx = x + glyph->offset.x;
        float gy = y + glyph->offset.y;
        float gw = glyph->atlas_rect.extent.width;
        float gh = glyph->atlas_rect.extent.height;

        // Texture coordinates in atlas
        size atlas_size = impl_->atlas->atlas_size();
        float u0 = glyph->atlas_rect.origin.x / atlas_size.width;
        float v0 = glyph->atlas_rect.origin.y / atlas_size.height;
        float u1 = (glyph->atlas_rect.origin.x + gw) / atlas_size.width;
        float v1 = (glyph->atlas_rect.origin.y + gh) / atlas_size.height;

        simd_float4 color_vec = simd_make_float4(
            props.text_color.r,
            props.text_color.g,
            props.text_color.b,
            props.text_color.a
        );

        // Two triangles for glyph quad
        impl_->vertices.push_back({{gx, gy}, color_vec, {u0, v0}});
        impl_->vertices.push_back({{gx + gw, gy}, color_vec, {u1, v0}});
        impl_->vertices.push_back({{gx, gy + gh}, color_vec, {u0, v1}});

        impl_->vertices.push_back({{gx + gw, gy}, color_vec, {u1, v0}});
        impl_->vertices.push_back({{gx + gw, gy + gh}, color_vec, {u1, v1}});
        impl_->vertices.push_back({{gx, gy + gh}, color_vec, {u0, v1}});

        x += glyph->advance;
    }

    // Update atlas texture if needed
    impl_->atlas->update_texture();
}

void metal_renderer::render_path(const path_node& node) {
    // TODO: Implement path rendering
}

void metal_renderer::render_image(const image_node& node) {
    // TODO: Implement image rendering
}

void metal_renderer::render_group(const group_node& node) {
    // Render all children
    for (const auto& child : node.children()) {
        if (child) {
            render_node_internal(*child);
        }
    }
}

void metal_renderer::render_effect(const effect_node& node) {
    // TODO: Implement effects (blur, shadow, etc.)
    // For now, just render the child
    if (node.child()) {
        render_node_internal(*node.child());
    }
}

void metal_renderer::render_scene_view(const scene_view_node& node) {
    // Scene view is a placeholder for user's 3D content
    // Render a border rectangle to indicate the viewport
    const auto& viewport = node.viewport();
    impl_->add_rectangle(viewport, color{0.8f, 0.8f, 0.8f, 1.0f}, 0.0f);
}

// Factory function for renderer_factory
std::unique_ptr<renderer> create_metal_renderer(size viewport_size, void* native_handle) {
    CAMetalLayer* metal_layer = static_cast<CAMetalLayer*>(native_handle);
    return std::make_unique<metal_renderer>(viewport_size, metal_layer);
}

}  // namespace stdui::rendering

#endif  // __APPLE__
