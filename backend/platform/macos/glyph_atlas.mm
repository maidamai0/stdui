// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#ifdef __APPLE__

#include "stdui/render/glyph_atlas.hpp"
#include <vector>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>

namespace stdui::rendering {

struct glyph_atlas::impl {
    id<MTLDevice> device;
    id<MTLTexture> texture;
    std::vector<uint8_t> atlas_data;
    bool needs_update = false;

    impl(id<MTLDevice> dev, uint32_t width, uint32_t height)
        : device(dev)
        , atlas_data(width * height, 0)
    {
        // Create Metal texture
        MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm
            width:width
            height:height
            mipmapped:NO];
        descriptor.usage = MTLTextureUsageShaderRead;
        descriptor.storageMode = MTLStorageModeManaged;

        texture = [device newTextureWithDescriptor:descriptor];
    }

    void upload_region(uint32_t x, uint32_t y, uint32_t width, uint32_t height, const uint8_t* data) {
        // Copy data into atlas_data
        for (uint32_t row = 0; row < height; ++row) {
            uint32_t atlas_offset = (y + row) * [texture width] + x;
            const uint8_t* src = data + row * width;
            std::copy(src, src + width, atlas_data.begin() + atlas_offset);
        }
        needs_update = true;
    }

    void update_texture_if_needed() {
        if (!needs_update) {
            return;
        }

        MTLRegion region = MTLRegionMake2D(0, 0, [texture width], [texture height]);
        [texture replaceRegion:region
                   mipmapLevel:0
                     withBytes:atlas_data.data()
                   bytesPerRow:[texture width]];

        needs_update = false;
    }
};

glyph_atlas::glyph_atlas(uint32_t width, uint32_t height)
    : width_(width)
    , height_(height)
{
    // Device will be set when first used
    impl_ = std::make_unique<impl>(nullptr, width, height);
}

glyph_atlas::~glyph_atlas() = default;

const glyph_info* glyph_atlas::get_glyph(
    uint32_t codepoint,
    const std::string& font_family,
    float font_size
) {
    font_key key{font_family, font_size, codepoint};

    // Check cache
    auto it = glyph_cache_.find(key);
    if (it != glyph_cache_.end()) {
        return &it->second;
    }

    // Rasterize new glyph
    glyph_info info;
    if (!rasterize_glyph(codepoint, font_family, font_size, info)) {
        return nullptr;
    }

    // Store in cache
    auto [inserted_it, _] = glyph_cache_.insert({key, info});
    return &inserted_it->second;
}

bool glyph_atlas::rasterize_glyph(
    uint32_t codepoint,
    const std::string& font_family,
    float font_size,
    glyph_info& info
) {
    // Create font
    CFStringRef family_name = CFStringCreateWithCString(
        kCFAllocatorDefault,
        font_family.c_str(),
        kCFStringEncodingUTF8
    );

    CTFontRef font = CTFontCreateWithName(family_name, font_size, nullptr);
    CFRelease(family_name);

    if (!font) {
        return false;
    }

    // Get glyph
    UniChar character = static_cast<UniChar>(codepoint);
    CGGlyph glyph;
    if (!CTFontGetGlyphsForCharacters(font, &character, &glyph, 1)) {
        CFRelease(font);
        return false;
    }

    // Get glyph metrics
    CGRect bbox;
    CTFontGetBoundingRectsForGlyphs(font, kCTFontOrientationDefault, &glyph, &bbox, 1);

    CGSize advance;
    CTFontGetAdvancesForGlyphs(font, kCTFontOrientationDefault, &glyph, &advance, 1);

    // Calculate glyph dimensions
    uint32_t glyph_width = static_cast<uint32_t>(std::ceil(bbox.size.width)) + 2;  // 1px padding
    uint32_t glyph_height = static_cast<uint32_t>(std::ceil(bbox.size.height)) + 2;

    // Check if we need to move to next row
    if (cursor_x_ + glyph_width > width_) {
        cursor_x_ = 0;
        cursor_y_ += row_height_;
        row_height_ = 0;
    }

    // Check if atlas is full
    if (cursor_y_ + glyph_height > height_) {
        CFRelease(font);
        return false;  // Atlas full
    }

    // Create bitmap context for rasterization
    std::vector<uint8_t> glyph_data(glyph_width * glyph_height, 0);

    CGColorSpaceRef color_space = CGColorSpaceCreateDeviceGray();
    CGContextRef context = CGBitmapContextCreate(
        glyph_data.data(),
        glyph_width,
        glyph_height,
        8,  // bits per component
        glyph_width,  // bytes per row
        color_space,
        kCGImageAlphaNone
    );

    CGColorSpaceRelease(color_space);

    if (!context) {
        CFRelease(font);
        return false;
    }

    // Set up rendering
    CGContextSetGrayFillColor(context, 1.0, 1.0);  // White text
    CGContextSetShouldAntialias(context, true);
    CGContextSetShouldSmoothFonts(context, true);

    // Position and render glyph
    CGPoint position = CGPointMake(1.0 - bbox.origin.x, 1.0 - bbox.origin.y);
    CTFontDrawGlyphs(font, &glyph, &position, 1, context);

    CGContextRelease(context);
    CFRelease(font);

    // Upload to atlas
    if (impl_->device) {
        impl_->upload_region(cursor_x_, cursor_y_, glyph_width, glyph_height, glyph_data.data());
    }

    // Store glyph info
    info.atlas_rect = rect{
        {static_cast<double>(cursor_x_), static_cast<double>(cursor_y_)},
        {static_cast<double>(glyph_width), static_cast<double>(glyph_height)}
    };
    info.offset = point{bbox.origin.x, bbox.origin.y};
    info.advance = advance.width;

    // Update cursor
    cursor_x_ += glyph_width;
    row_height_ = std::max(row_height_, glyph_height);

    return true;
}

id<MTLTexture> glyph_atlas::metal_texture() const {
    return impl_->texture;
}

void glyph_atlas::update_texture() {
    impl_->update_texture_if_needed();
}

void glyph_atlas::set_metal_device(id<MTLDevice> device) {
    impl_->device = device;

    // Recreate texture with the new device
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm
        width:width_
        height:height_
        mipmapped:NO];
    descriptor.usage = MTLTextureUsageShaderRead;
    descriptor.storageMode = MTLStorageModeManaged;

    impl_->texture = [device newTextureWithDescriptor:descriptor];
}

}  // namespace stdui::rendering

#endif  // __APPLE__
