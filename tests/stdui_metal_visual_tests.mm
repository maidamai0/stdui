// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#ifdef __APPLE__

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "metal_test_helpers.hpp"
#include "stdui/rendering/metal_renderer.hpp"
#include "stdui/rendering/render_node.hpp"
#include <doctest/doctest.h>
#include <filesystem>

using namespace stdui;
using namespace stdui::rendering;
using namespace stdui::rendering::test;

namespace fs = std::filesystem;

// Test configuration
static const size_t TEST_WIDTH = 800;
static const size_t TEST_HEIGHT = 600;
static const double PIXEL_TOLERANCE = 2.0;  // Allow up to 2/255 difference per channel
static const double MAX_DIFF_PERCENT = 0.1;  // Allow 0.1% different pixels

static const std::string GOLDEN_DIR = "tests/golden_images/";
static const std::string OUTPUT_DIR = "tests/output_images/";

TEST_SUITE("metal_visual_tests") {
    TEST_CASE("setup output directory") {
        fs::create_directories(OUTPUT_DIR);
        REQUIRE(fs::exists(OUTPUT_DIR));
    }

    TEST_CASE("visual: solid red rectangle") {
        metal_test_renderer test_renderer(TEST_WIDTH, TEST_HEIGHT);
        metal_renderer renderer({TEST_WIDTH, TEST_HEIGHT}, nullptr);

        // Create render tree with red rectangle
        render_tree tree;
        rectangle_properties props;
        props.bounds = rect{{100, 100}, {200, 150}};
        props.fill_color = color{1.0f, 0.0f, 0.0f, 1.0f};  // Red
        auto rect_node = std::make_shared<rectangle_node>(props);
        tree.set_root(rect_node);

        // Render to off-screen texture
        renderer.begin_frame();
        renderer.render(tree);
        renderer.end_frame();

        // Capture output
        auto pixels = test_renderer.capture_pixels();
        std::string output_path = OUTPUT_DIR + "red_rectangle.png";
        test_renderer.save_png(output_path);

        // Compare with golden image if it exists
        std::string golden_path = GOLDEN_DIR + "red_rectangle.png";
        if (fs::exists(golden_path)) {
            size_t golden_w, golden_h;
            auto golden_pixels = metal_test_renderer::load_png(golden_path, golden_w, golden_h);

            REQUIRE(golden_w == TEST_WIDTH);
            REQUIRE(golden_h == TEST_HEIGHT);

            double diff = metal_test_renderer::compare_pixels(pixels, golden_pixels, PIXEL_TOLERANCE);
            CHECK(diff < MAX_DIFF_PERCENT);

            if (diff >= MAX_DIFF_PERCENT) {
                MESSAGE("Visual difference: ", diff, "% (threshold: ", MAX_DIFF_PERCENT, "%)");
                MESSAGE("Output saved to: ", output_path);
            }
        } else {
            MESSAGE("Golden image not found, output saved to: ", output_path);
        }
    }

    TEST_CASE("visual: text rendering") {
        metal_test_renderer test_renderer(TEST_WIDTH, TEST_HEIGHT);
        metal_renderer renderer({TEST_WIDTH, TEST_HEIGHT}, nullptr);

        render_tree tree;
        text_properties props;
        props.content = "Hello, stdui!";
        props.position = point{100, 100};
        props.font_family = "Helvetica";
        props.font_size = 32.0f;
        props.text_color = color{0.0f, 0.0f, 0.0f, 1.0f};

        auto text_node = std::make_shared<rendering::text_node>(props);
        tree.set_root(text_node);

        renderer.begin_frame();
        renderer.render(tree);
        renderer.end_frame();

        auto pixels = test_renderer.capture_pixels();
        std::string output_path = OUTPUT_DIR + "hello_text.png";
        test_renderer.save_png(output_path);

        std::string golden_path = GOLDEN_DIR + "hello_text.png";
        if (fs::exists(golden_path)) {
            size_t golden_w, golden_h;
            auto golden_pixels = metal_test_renderer::load_png(golden_path, golden_w, golden_h);

            REQUIRE(golden_w == TEST_WIDTH);
            REQUIRE(golden_h == TEST_HEIGHT);

            double diff = metal_test_renderer::compare_pixels(pixels, golden_pixels, PIXEL_TOLERANCE);
            CHECK(diff < MAX_DIFF_PERCENT);

            if (diff >= MAX_DIFF_PERCENT) {
                MESSAGE("Visual difference: ", diff, "%");
                MESSAGE("Output saved to: ", output_path);
            }
        } else {
            MESSAGE("Golden image not found, output saved to: ", output_path);
        }
    }

    TEST_CASE("visual: colored rectangles grid") {
        metal_test_renderer test_renderer(TEST_WIDTH, TEST_HEIGHT);
        metal_renderer renderer({TEST_WIDTH, TEST_HEIGHT}, nullptr);

        render_tree tree;
        auto group = std::make_shared<group_node>();

        // Create a 5x5 grid of colored rectangles
        for (int row = 0; row < 5; ++row) {
            for (int col = 0; col < 5; ++col) {
                rectangle_properties props;
                props.bounds = rect{
                    {float(col * 150 + 25), float(row * 110 + 25)},
                    {140, 100}
                };
                props.fill_color = color{
                    float(col) / 4.0f,
                    float(row) / 4.0f,
                    0.5f,
                    1.0f
                };
                group->add_child(std::make_shared<rectangle_node>(props));
            }
        }

        tree.set_root(group);

        renderer.begin_frame();
        renderer.render(tree);
        renderer.end_frame();

        auto pixels = test_renderer.capture_pixels();
        std::string output_path = OUTPUT_DIR + "colored_grid.png";
        test_renderer.save_png(output_path);

        std::string golden_path = GOLDEN_DIR + "colored_grid.png";
        if (fs::exists(golden_path)) {
            size_t golden_w, golden_h;
            auto golden_pixels = metal_test_renderer::load_png(golden_path, golden_w, golden_h);

            REQUIRE(golden_w == TEST_WIDTH);
            REQUIRE(golden_h == TEST_HEIGHT);

            double diff = metal_test_renderer::compare_pixels(pixels, golden_pixels, PIXEL_TOLERANCE);
            CHECK(diff < MAX_DIFF_PERCENT);

            if (diff >= MAX_DIFF_PERCENT) {
                MESSAGE("Visual difference: ", diff, "%");
                MESSAGE("Output saved to: ", output_path);
            }
        } else {
            MESSAGE("Golden image not found, output saved to: ", output_path);
        }
    }

    TEST_CASE("visual: triangle path") {
        metal_test_renderer test_renderer(TEST_WIDTH, TEST_HEIGHT);
        metal_renderer renderer({TEST_WIDTH, TEST_HEIGHT}, nullptr);

        render_tree tree;
        path_properties props;
        props.points = {
            point{400, 100},
            point{600, 400},
            point{200, 400}
        };
        props.fill_color = color{0.0f, 0.8f, 0.2f, 1.0f};  // Green

        auto path_node = std::make_shared<rendering::path_node>(props);
        tree.set_root(path_node);

        renderer.begin_frame();
        renderer.render(tree);
        renderer.end_frame();

        auto pixels = test_renderer.capture_pixels();
        std::string output_path = OUTPUT_DIR + "triangle.png";
        test_renderer.save_png(output_path);

        std::string golden_path = GOLDEN_DIR + "triangle.png";
        if (fs::exists(golden_path)) {
            size_t golden_w, golden_h;
            auto golden_pixels = metal_test_renderer::load_png(golden_path, golden_w, golden_h);

            REQUIRE(golden_w == TEST_WIDTH);
            REQUIRE(golden_h == TEST_HEIGHT);

            double diff = metal_test_renderer::compare_pixels(pixels, golden_pixels, PIXEL_TOLERANCE);
            CHECK(diff < MAX_DIFF_PERCENT);

            if (diff >= MAX_DIFF_PERCENT) {
                MESSAGE("Visual difference: ", diff, "%");
                MESSAGE("Output saved to: ", output_path);
            }
        } else {
            MESSAGE("Golden image not found, output saved to: ", output_path);
        }
    }

    TEST_CASE("visual: alpha blending") {
        metal_test_renderer test_renderer(TEST_WIDTH, TEST_HEIGHT);
        metal_renderer renderer({TEST_WIDTH, TEST_HEIGHT}, nullptr);

        render_tree tree;
        auto group = std::make_shared<group_node>();

        // Background white rectangle
        rectangle_properties bg_props;
        bg_props.bounds = rect{{0, 0}, {TEST_WIDTH, TEST_HEIGHT}};
        bg_props.fill_color = color{1.0f, 1.0f, 1.0f, 1.0f};
        group->add_child(std::make_shared<rectangle_node>(bg_props));

        // Overlapping semi-transparent rectangles
        rectangle_properties red_props;
        red_props.bounds = rect{{200, 200}, {300, 200}};
        red_props.fill_color = color{1.0f, 0.0f, 0.0f, 0.5f};  // 50% transparent
        group->add_child(std::make_shared<rectangle_node>(red_props));

        rectangle_properties blue_props;
        blue_props.bounds = rect{{300, 250}, {300, 200}};
        blue_props.fill_color = color{0.0f, 0.0f, 1.0f, 0.5f};  // 50% transparent
        group->add_child(std::make_shared<rectangle_node>(blue_props));

        tree.set_root(group);

        renderer.begin_frame();
        renderer.render(tree);
        renderer.end_frame();

        auto pixels = test_renderer.capture_pixels();
        std::string output_path = OUTPUT_DIR + "alpha_blend.png";
        test_renderer.save_png(output_path);

        std::string golden_path = GOLDEN_DIR + "alpha_blend.png";
        if (fs::exists(golden_path)) {
            size_t golden_w, golden_h;
            auto golden_pixels = metal_test_renderer::load_png(golden_path, golden_w, golden_h);

            REQUIRE(golden_w == TEST_WIDTH);
            REQUIRE(golden_h == TEST_HEIGHT);

            double diff = metal_test_renderer::compare_pixels(pixels, golden_pixels, PIXEL_TOLERANCE);
            CHECK(diff < MAX_DIFF_PERCENT);

            if (diff >= MAX_DIFF_PERCENT) {
                MESSAGE("Visual difference: ", diff, "%");
                MESSAGE("Output saved to: ", output_path);
            }
        } else {
            MESSAGE("Golden image not found, output saved to: ", output_path);
        }
    }

    TEST_CASE("visual: complex scene") {
        metal_test_renderer test_renderer(TEST_WIDTH, TEST_HEIGHT);
        metal_renderer renderer({TEST_WIDTH, TEST_HEIGHT}, nullptr);

        render_tree tree;
        auto root = std::make_shared<group_node>();

        // Background
        rectangle_properties bg_props;
        bg_props.bounds = rect{{0, 0}, {TEST_WIDTH, TEST_HEIGHT}};
        bg_props.fill_color = color{0.95f, 0.95f, 0.95f, 1.0f};
        root->add_child(std::make_shared<rectangle_node>(bg_props));

        // Title
        text_properties title_props;
        title_props.content = "stdui Visual Test";
        title_props.position = point{50, 50};
        title_props.font_family = "Helvetica";
        title_props.font_size = 48.0f;
        title_props.text_color = color{0.1f, 0.1f, 0.1f, 1.0f};
        root->add_child(std::make_shared<rendering::text_node>(title_props));

        // Colored rectangles
        for (int i = 0; i < 8; ++i) {
            rectangle_properties rect_props;
            rect_props.bounds = rect{{float(50 + i * 90), 150}, {80, 80}};
            rect_props.fill_color = color{
                float(i) / 8.0f,
                1.0f - float(i) / 8.0f,
                0.5f,
                0.8f
            };
            root->add_child(std::make_shared<rectangle_node>(rect_props));
        }

        // Path
        path_properties path_props;
        path_props.points = {
            point{400, 300},
            point{500, 300},
            point{450, 400}
        };
        path_props.fill_color = color{0.2f, 0.6f, 0.8f, 1.0f};
        root->add_child(std::make_shared<rendering::path_node>(path_props));

        tree.set_root(root);

        renderer.begin_frame();
        renderer.render(tree);
        renderer.end_frame();

        auto pixels = test_renderer.capture_pixels();
        std::string output_path = OUTPUT_DIR + "complex_scene.png";
        test_renderer.save_png(output_path);

        std::string golden_path = GOLDEN_DIR + "complex_scene.png";
        if (fs::exists(golden_path)) {
            size_t golden_w, golden_h;
            auto golden_pixels = metal_test_renderer::load_png(golden_path, golden_w, golden_h);

            REQUIRE(golden_w == TEST_WIDTH);
            REQUIRE(golden_h == TEST_HEIGHT);

            double diff = metal_test_renderer::compare_pixels(pixels, golden_pixels, PIXEL_TOLERANCE);
            CHECK(diff < MAX_DIFF_PERCENT);

            if (diff >= MAX_DIFF_PERCENT) {
                MESSAGE("Visual difference: ", diff, "%");
                MESSAGE("Output saved to: ", output_path);
            }
        } else {
            MESSAGE("Golden image not found, output saved to: ", output_path);
        }
    }
}

#endif  // __APPLE__
