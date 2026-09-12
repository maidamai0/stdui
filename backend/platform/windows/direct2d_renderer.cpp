// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#ifdef _WIN32

#include "stdui/rendering/direct2d_renderer.hpp"
#include <d2d1_1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <stdexcept>
#include <algorithm>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace stdui::rendering {

struct direct2d_renderer::impl {
    // Direct2D resources
    ID2D1Factory* factory = nullptr;
    ID2D1HwndRenderTarget* render_target = nullptr;
    IDWriteFactory* write_factory = nullptr;

    // Current rendering state
    ID2D1SolidColorBrush* solid_brush = nullptr;

    // Viewport
    size viewport;
    HWND hwnd;

    impl(size viewport_size, HWND window)
        : viewport(viewport_size)
        , hwnd(window)
    {
        HRESULT hr;

        // Create D2D factory
        hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory);
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create Direct2D factory");
        }

        // Create DWrite factory
        hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(&write_factory)
        );
        if (FAILED(hr)) {
            factory->Release();
            throw std::runtime_error("Failed to create DirectWrite factory");
        }

        // Create render target
        create_render_target();
    }

    ~impl() {
        if (solid_brush) solid_brush->Release();
        if (render_target) render_target->Release();
        if (write_factory) write_factory->Release();
        if (factory) factory->Release();
    }

    void create_render_target() {
        if (render_target) {
            render_target->Release();
            render_target = nullptr;
        }

        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();

        D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_props = D2D1::HwndRenderTargetProperties(
            hwnd,
            D2D1::SizeU(
                static_cast<UINT32>(viewport.width),
                static_cast<UINT32>(viewport.height)
            )
        );

        HRESULT hr = factory->CreateHwndRenderTarget(
            props,
            hwnd_props,
            &render_target
        );

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create HWND render target");
        }

        // Create solid brush
        if (solid_brush) {
            solid_brush->Release();
        }
        render_target->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::White),
            &solid_brush
        );
    }

    D2D1_COLOR_F to_d2d_color(const color& c) {
        return D2D1::ColorF(c.red, c.green, c.blue, c.alpha);
    }

    D2D1_RECT_F to_d2d_rect(const rect& r) {
        return D2D1::RectF(
            static_cast<FLOAT>(r.origin.x),
            static_cast<FLOAT>(r.origin.y),
            static_cast<FLOAT>(r.origin.x + r.extent.width),
            static_cast<FLOAT>(r.origin.y + r.extent.height)
        );
    }

    D2D1_POINT_2F to_d2d_point(const point& p) {
        return D2D1::Point2F(
            static_cast<FLOAT>(p.x),
            static_cast<FLOAT>(p.y)
        );
    }
};

direct2d_renderer::direct2d_renderer(size viewport_size, void* hwnd)
    : impl_(std::make_unique<impl>(viewport_size, static_cast<HWND>(hwnd)))
{
}

direct2d_renderer::~direct2d_renderer() = default;

void direct2d_renderer::begin_frame() {
    impl_->render_target->BeginDraw();
    impl_->render_target->Clear(D2D1::ColorF(D2D1::ColorF::White));
}

void direct2d_renderer::render(const render_tree& tree) {
    if (tree.root()) {
        render_node_internal(*tree.root());
    }
}

void direct2d_renderer::end_frame() {
    HRESULT hr = impl_->render_target->EndDraw();

    if (hr == D2DERR_RECREATE_TARGET) {
        // Device lost, recreate render target
        impl_->create_render_target();
    }
}

void direct2d_renderer::resize(size new_size) {
    impl_->viewport = new_size;

    if (impl_->render_target) {
        impl_->render_target->Resize(
            D2D1::SizeU(
                static_cast<UINT32>(new_size.width),
                static_cast<UINT32>(new_size.height)
            )
        );
    }
}

size direct2d_renderer::viewport_size() const {
    return impl_->viewport;
}

void direct2d_renderer::render_node_internal(const render_node& node) {
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

void direct2d_renderer::render_rectangle(const rectangle_node& node) {
    const auto& props = node.properties();

    impl_->solid_brush->SetColor(impl_->to_d2d_color(props.fill_color));

    D2D1_RECT_F rect = impl_->to_d2d_rect(props.bounds);

    if (props.corner_radius > 0.0f) {
        D2D1_ROUNDED_RECT rounded_rect = D2D1::RoundedRect(
            rect,
            props.corner_radius,
            props.corner_radius
        );
        impl_->render_target->FillRoundedRectangle(rounded_rect, impl_->solid_brush);

        if (props.stroke_color.has_value()) {
            impl_->solid_brush->SetColor(impl_->to_d2d_color(*props.stroke_color));
            impl_->render_target->DrawRoundedRectangle(
                rounded_rect,
                impl_->solid_brush,
                props.stroke_width
            );
        }
    } else {
        impl_->render_target->FillRectangle(rect, impl_->solid_brush);

        if (props.stroke_color.has_value()) {
            impl_->solid_brush->SetColor(impl_->to_d2d_color(*props.stroke_color));
            impl_->render_target->DrawRectangle(rect, impl_->solid_brush, props.stroke_width);
        }
    }
}

void direct2d_renderer::render_text(const text_node& node) {
    const auto& props = node.properties();

    // Create text format
    IDWriteTextFormat* text_format = nullptr;

    std::wstring family_wide(props.font_family.begin(), props.font_family.end());

    HRESULT hr = impl_->write_factory->CreateTextFormat(
        family_wide.c_str(),
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        props.font_size,
        L"en-us",
        &text_format
    );

    if (SUCCEEDED(hr)) {
        impl_->solid_brush->SetColor(impl_->to_d2d_color(props.text_color));

        std::wstring text_wide(props.content.begin(), props.content.end());

        D2D1_RECT_F layout_rect = D2D1::RectF(
            static_cast<FLOAT>(props.position.x),
            static_cast<FLOAT>(props.position.y),
            static_cast<FLOAT>(props.position.x + 1000),
            static_cast<FLOAT>(props.position.y + 100)
        );

        impl_->render_target->DrawText(
            text_wide.c_str(),
            static_cast<UINT32>(text_wide.length()),
            text_format,
            layout_rect,
            impl_->solid_brush
        );

        text_format->Release();
    }
}

void direct2d_renderer::render_path(const path_node& node) {
    const auto& props = node.properties();

    if (props.points.empty()) {
        return;
    }

    // Create path geometry
    ID2D1PathGeometry* path_geometry = nullptr;
    HRESULT hr = impl_->factory->CreatePathGeometry(&path_geometry);

    if (SUCCEEDED(hr)) {
        ID2D1GeometrySink* sink = nullptr;
        hr = path_geometry->Open(&sink);

        if (SUCCEEDED(hr)) {
            // Simple path: just connect the points
            sink->BeginFigure(
                impl_->to_d2d_point(props.points[0]),
                D2D1_FIGURE_BEGIN_FILLED
            );

            for (size_t i = 1; i < props.points.size(); ++i) {
                sink->AddLine(impl_->to_d2d_point(props.points[i]));
            }

            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            sink->Close();

            // Fill
            impl_->solid_brush->SetColor(impl_->to_d2d_color(props.fill_color));
            impl_->render_target->FillGeometry(path_geometry, impl_->solid_brush);

            // Stroke
            if (props.stroke_color.has_value()) {
                impl_->solid_brush->SetColor(impl_->to_d2d_color(*props.stroke_color));
                impl_->render_target->DrawGeometry(
                    path_geometry,
                    impl_->solid_brush,
                    props.stroke_width
                );
            }

            sink->Release();
        }

        path_geometry->Release();
    }
}

void direct2d_renderer::render_image(const image_node& node) {
    // TODO: Implement image rendering with WIC
}

void direct2d_renderer::render_group(const group_node& node) {
    for (const auto& child : node.children()) {
        if (child) {
            render_node_internal(*child);
        }
    }
}

void direct2d_renderer::render_effect(const effect_node& node) {
    // TODO: Implement effects
    if (node.child()) {
        render_node_internal(*node.child());
    }
}

void direct2d_renderer::render_scene_view(const scene_view_node& node) {
    // Render border
    impl_->solid_brush->SetColor(D2D1::ColorF(0.8f, 0.8f, 0.8f, 1.0f));
    impl_->render_target->DrawRectangle(
        impl_->to_d2d_rect(node.viewport()),
        impl_->solid_brush,
        2.0f
    );
}

}  // namespace stdui::rendering

#endif  // _WIN32
