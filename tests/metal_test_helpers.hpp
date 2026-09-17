// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#ifdef __APPLE__

#import <Metal/Metal.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ImageIO/ImageIO.h>
#include <vector>
#include <string>
#include <filesystem>

namespace stdui::rendering::test {

/// Helper class to render to off-screen Metal texture and capture output
class metal_test_renderer {
public:
    metal_test_renderer(size_t width, size_t height)
        : width_(width), height_(height) {

        device_ = MTLCreateSystemDefaultDevice();
        if (!device_) {
            throw std::runtime_error("Failed to create Metal device");
        }

        // Create texture descriptor for rendering target
        MTLTextureDescriptor* texDesc = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
            width:width
            height:height
            mipmapped:NO];
        texDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        texDesc.storageMode = MTLStorageModeManaged;

        texture_ = [device_ newTextureWithDescriptor:texDesc];
        if (!texture_) {
            throw std::runtime_error("Failed to create render texture");
        }

        command_queue_ = [device_ newCommandQueue];
    }

    ~metal_test_renderer() {
        [texture_ release];
        [command_queue_ release];
        [device_ release];
    }

    id<MTLDevice> device() const { return device_; }
    id<MTLTexture> texture() const { return texture_; }
    id<MTLCommandQueue> command_queue() const { return command_queue_; }

    size_t width() const { return width_; }
    size_t height() const { return height_; }

    /// Capture rendered texture to raw RGBA8 pixel data
    std::vector<uint8_t> capture_pixels() {
        std::vector<uint8_t> pixels(width_ * height_ * 4);

        MTLRegion region = MTLRegionMake2D(0, 0, width_, height_);
        [texture_ getBytes:pixels.data()
                bytesPerRow:width_ * 4
                 fromRegion:region
                mipmapLevel:0];

        return pixels;
    }

    /// Save captured pixels as PNG file
    bool save_png(const std::string& filepath) {
        auto pixels = capture_pixels();

        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(
            pixels.data(),
            width_,
            height_,
            8,  // bits per component
            width_ * 4,  // bytes per row
            colorSpace,
            kCGImageAlphaPremultipliedLast
        );

        if (!context) {
            CGColorSpaceRelease(colorSpace);
            return false;
        }

        CGImageRef image = CGBitmapContextCreateImage(context);
        CGContextRelease(context);
        CGColorSpaceRelease(colorSpace);

        if (!image) {
            return false;
        }

        CFURLRef url = CFURLCreateFromFileSystemRepresentation(
            kCFAllocatorDefault,
            (const UInt8*)filepath.c_str(),
            filepath.length(),
            false
        );

        CGImageDestinationRef destination = CGImageDestinationCreateWithURL(
            url,
            CFSTR("public.png"),
            1,
            nullptr
        );

        bool success = false;
        if (destination) {
            CGImageDestinationAddImage(destination, image, nullptr);
            success = CGImageDestinationFinalize(destination);
            CFRelease(destination);
        }

        CFRelease(url);
        CGImageRelease(image);

        return success;
    }

    /// Load reference PNG file
    static std::vector<uint8_t> load_png(const std::string& filepath, size_t& width, size_t& height) {
        CFURLRef url = CFURLCreateFromFileSystemRepresentation(
            kCFAllocatorDefault,
            (const UInt8*)filepath.c_str(),
            filepath.length(),
            false
        );

        CGImageSourceRef source = CGImageSourceCreateWithURL(url, nullptr);
        CFRelease(url);

        if (!source) {
            return {};
        }

        CGImageRef image = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
        CFRelease(source);

        if (!image) {
            return {};
        }

        width = CGImageGetWidth(image);
        height = CGImageGetHeight(image);

        std::vector<uint8_t> pixels(width * height * 4);

        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(
            pixels.data(),
            width,
            height,
            8,
            width * 4,
            colorSpace,
            kCGImageAlphaPremultipliedLast
        );

        CGContextDrawImage(context, CGRectMake(0, 0, width, height), image);

        CGContextRelease(context);
        CGColorSpaceRelease(colorSpace);
        CGImageRelease(image);

        return pixels;
    }

    /// Compare two pixel buffers, return percentage difference
    static double compare_pixels(
        const std::vector<uint8_t>& pixels1,
        const std::vector<uint8_t>& pixels2,
        double tolerance = 1.0) {

        if (pixels1.size() != pixels2.size()) {
            return 100.0;  // Completely different
        }

        size_t different_pixels = 0;
        size_t total_pixels = pixels1.size() / 4;

        for (size_t i = 0; i < pixels1.size(); i += 4) {
            int r_diff = std::abs(int(pixels1[i]) - int(pixels2[i]));
            int g_diff = std::abs(int(pixels1[i+1]) - int(pixels2[i+1]));
            int b_diff = std::abs(int(pixels1[i+2]) - int(pixels2[i+2]));
            int a_diff = std::abs(int(pixels1[i+3]) - int(pixels2[i+3]));

            double pixel_diff = (r_diff + g_diff + b_diff + a_diff) / 4.0;

            if (pixel_diff > tolerance) {
                different_pixels++;
            }
        }

        return (double(different_pixels) / double(total_pixels)) * 100.0;
    }

private:
    size_t width_;
    size_t height_;
    id<MTLDevice> device_;
    id<MTLTexture> texture_;
    id<MTLCommandQueue> command_queue_;
};

}  // namespace stdui::rendering::test

#endif  // __APPLE__
