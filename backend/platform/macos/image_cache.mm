// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#ifdef __APPLE__

#include "stdui/rendering/image_cache.hpp"
#include <unordered_map>
#include <vector>

namespace stdui::rendering {

struct image_cache::impl {
    id<MTLDevice> device;
    std::unordered_map<uint32_t, id<MTLTexture>> textures;

    impl(id<MTLDevice> dev) : device(dev) {}

    ~impl() {
        for (auto& [id, texture] : textures) {
            [texture release];
        }
    }
};

image_cache::image_cache(id<MTLDevice> device)
    : impl_(std::make_unique<impl>(device))
{
}

image_cache::~image_cache() = default;

id<MTLTexture> image_cache::load_image(uint32_t image_id, const image_data& data) {
    // Check if already cached
    auto it = impl_->textures.find(image_id);
    if (it != impl_->textures.end()) {
        return it->second;
    }

    // Determine Metal pixel format
    MTLPixelFormat pixel_format;
    switch (data.format) {
        case image_format::rgba8:
            pixel_format = MTLPixelFormatRGBA8Unorm;
            break;
        case image_format::bgra8:
            pixel_format = MTLPixelFormatBGRA8Unorm;
            break;
        case image_format::rgb8:
            // Metal doesn't support RGB8, convert to RGBA8
            pixel_format = MTLPixelFormatRGBA8Unorm;
            break;
        case image_format::gray8:
            pixel_format = MTLPixelFormatR8Unorm;
            break;
        default:
            return nullptr;
    }

    // Create texture descriptor
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:pixel_format
        width:data.width
        height:data.height
        mipmapped:NO];

    descriptor.usage = MTLTextureUsageShaderRead;
    descriptor.storageMode = MTLStorageModeManaged;

    // Create texture
    id<MTLTexture> texture = [impl_->device newTextureWithDescriptor:descriptor];
    if (!texture) {
        return nullptr;
    }

    // Upload pixel data
    MTLRegion region = MTLRegionMake2D(0, 0, data.width, data.height);

    if (data.format == image_format::rgb8) {
        // Convert RGB8 to RGBA8
        size_t rgba_size = data.width * data.height * 4;
        std::vector<uint8_t> rgba_data(rgba_size);

        const uint8_t* src = static_cast<const uint8_t*>(data.pixels);
        uint8_t* dst = rgba_data.data();

        for (uint32_t i = 0; i < data.width * data.height; ++i) {
            dst[i * 4 + 0] = src[i * 3 + 0];  // R
            dst[i * 4 + 1] = src[i * 3 + 1];  // G
            dst[i * 4 + 2] = src[i * 3 + 2];  // B
            dst[i * 4 + 3] = 255;              // A
        }

        [texture replaceRegion:region
                   mipmapLevel:0
                     withBytes:rgba_data.data()
                   bytesPerRow:data.width * 4];
    } else {
        [texture replaceRegion:region
                   mipmapLevel:0
                     withBytes:data.pixels
                   bytesPerRow:data.stride];
    }

    // Cache the texture
    impl_->textures[image_id] = texture;

    return texture;
}

id<MTLTexture> image_cache::get_texture(uint32_t image_id) const {
    auto it = impl_->textures.find(image_id);
    return (it != impl_->textures.end()) ? it->second : nullptr;
}

void image_cache::unload_image(uint32_t image_id) {
    auto it = impl_->textures.find(image_id);
    if (it != impl_->textures.end()) {
        [it->second release];
        impl_->textures.erase(it);
    }
}

void image_cache::clear() {
    for (auto& [id, texture] : impl_->textures) {
        [texture release];
    }
    impl_->textures.clear();
}

}  // namespace stdui::rendering

#endif  // __APPLE__
