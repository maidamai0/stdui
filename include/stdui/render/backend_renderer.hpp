// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#pragma once

#include "stdui/render/node.hpp"
#include "stdui/render/tree_builder.hpp"
#include "stdui/core/geometry.hpp"

#include <memory>
#include <cstdint>

namespace stdui::rendering {

/// Abstract renderer interface for backend implementations
class renderer {
public:
    virtual ~renderer() = default;

    /// Begin a new frame
    virtual void begin_frame() = 0;

    /// Render a render tree to the current frame
    virtual void render(const render_tree& tree) = 0;

    /// End the current frame and present
    virtual void end_frame() = 0;

    /// Resize the renderer's output surface
    virtual void resize(size new_size) = 0;

    /// Get the current viewport size
    virtual size viewport_size() const = 0;
};

/// Renderer factory - creates platform-specific renderers
class renderer_factory {
public:
    /// Create a renderer for the current platform
    /// @param viewport_size Initial viewport dimensions
    /// @param native_handle Platform-specific handle (CAMetalLayer*, ID3D12Device*, etc.)
    /// @return Platform-specific renderer instance
    static std::unique_ptr<renderer> create(
        size viewport_size,
        void* native_handle = nullptr
    );
};

}  // namespace stdui::rendering
