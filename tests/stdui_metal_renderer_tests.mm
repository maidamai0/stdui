// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#ifdef __APPLE__

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "stdui/rendering/metal_renderer.hpp"
#include "stdui/rendering/render_tree.hpp"
#include <doctest/doctest.h>

using namespace stdui;
using namespace stdui::rendering;

TEST_SUITE("metal_renderer") {
    TEST_CASE("metal_renderer initialization") {
        // Create a small viewport
        size viewport_size{800, 600};

        SUBCASE("can create metal renderer") {
            REQUIRE_NOTHROW(metal_renderer(viewport_size, nullptr));
        }

        SUBCASE("viewport size is correct") {
            metal_renderer renderer(viewport_size, nullptr);
            CHECK(renderer.viewport_size().width == 800);
            CHECK(renderer.viewport_size().height == 600);
        }

        SUBCASE("can resize viewport") {
            metal_renderer renderer(viewport_size, nullptr);
            size new_size{1024, 768};
            renderer.resize(new_size);
            CHECK(renderer.viewport_size().width == 1024);
            CHECK(renderer.viewport_size().height == 768);
        }
    }

    TEST_CASE("metal_renderer frame lifecycle") {
        size viewport_size{800, 600};
        metal_renderer renderer(viewport_size, nullptr);

        SUBCASE("can begin and end frame without rendering") {
            REQUIRE_NOTHROW(renderer.begin_frame());
            REQUIRE_NOTHROW(renderer.end_frame());
        }

        SUBCASE("can render empty tree") {
            render_tree tree;
            REQUIRE_NOTHROW(renderer.begin_frame());
            REQUIRE_NOTHROW(renderer.render(tree));
            REQUIRE_NOTHROW(renderer.end_frame());
        }
    }

    TEST_CASE("metal_renderer rectangle rendering") {
        size viewport_size{800, 600};
        metal_renderer renderer(viewport_size, nullptr);

        SUBCASE("can render single rectangle") {
            render_tree tree;

            rectangle_properties props;
            props.bounds = rect{{100, 100}, {200, 150}};
            props.fill_color = color{1.0f, 0.0f, 0.0f, 1.0f};  // Red

            auto rect_node = std::make_shared<rectangle_node>(props);
            tree.set_root(rect_node);

            REQUIRE_NOTHROW(renderer.begin_frame());
            REQUIRE_NOTHROW(renderer.render(tree));
            REQUIRE_NOTHROW(renderer.end_frame());
        }

        SUBCASE("can render multiple rectangles") {
            render_tree tree;

            auto group = std::make_shared<group_node>();

            // Add 5 rectangles
            for (int i = 0; i < 5; ++i) {
                rectangle_properties props;
                props.bounds = rect{{float(i * 50), 100}, {40, 40}};
                props.fill_color = color{float(i) / 5.0f, 0.5f, 1.0f, 1.0f};

                auto rect_node = std::make_shared<rectangle_node>(props);
                group->add_child(rect_node);
            }

            tree.set_root(group);

            REQUIRE_NOTHROW(renderer.begin_frame());
            REQUIRE_NOTHROW(renderer.render(tree));
            REQUIRE_NOTHROW(renderer.end_frame());
        }

        SUBCASE("can render rectangle with transparency") {
            render_tree tree;

            rectangle_properties props;
            props.bounds = rect{{100, 100}, {200, 150}};
            props.fill_color = color{1.0f, 0.0f, 0.0f, 0.5f};  // Semi-transparent red

            auto rect_node = std::make_shared<rectangle_node>(props);
            tree.set_root(rect_node);

            REQUIRE_NOTHROW(renderer.begin_frame());
            REQUIRE_NOTHROW(renderer.render(tree));
            REQUIRE_NOTHROW(renderer.end_frame());
        }
    }

    TEST_CASE("metal_renderer text rendering") {
        size viewport_size{800, 600};
        metal_renderer renderer(viewport_size, nullptr);

        SUBCASE("can render text node") {
            render_tree tree;

            text_properties props;
            props.content = "Hello, World!";
            props.position = point{100, 100};
            props.font_family = "Helvetica";
            props.font_size = 24.0f;
            props.text_color = color{0.0f, 0.0f, 0.0f, 1.0f};  // Black

            auto text_node = std::make_shared<rendering::text_node>(props);
            tree.set_root(text_node);

            REQUIRE_NOTHROW(renderer.begin_frame());
            REQUIRE_NOTHROW(renderer.render(tree));
            REQUIRE_NOTHROW(renderer.end_frame());
        }

        SUBCASE("can render multiple text nodes") {
            render_tree tree;

            auto group = std::make_shared<group_node>();

            for (int i = 0; i < 3; ++i) {
                text_properties props;
                props.content = "Line " + std::to_string(i);
                props.position = point{50, float(50 + i * 30)};
                props.font_family = "Helvetica";
                props.font_size = 16.0f;
                props.text_color = color{0.0f, 0.0f, 0.0f, 1.0f};

                auto text_node = std::make_shared<rendering::text_node>(props);
                group->add_child(text_node);
            }

            tree.set_root(group);

            REQUIRE_NOTHROW(renderer.begin_frame());
            REQUIRE_NOTHROW(renderer.render(tree));
            REQUIRE_NOTHROW(renderer.end_frame());
        }
    }

    TEST_CASE("metal_renderer path rendering") {
        size viewport_size{800, 600};
        metal_renderer renderer(viewport_size, nullptr);

        SUBCASE("can render triangle path") {
            render_tree tree;

            path_properties props;
            props.points = {
                point{100, 100},
                point{200, 100},
                point{150, 200}
            };
            props.fill_color = color{0.0f, 1.0f, 0.0f, 1.0f};  // Green

            auto path_node = std::make_shared<rendering::path_node>(props);
            tree.set_root(path_node);

            REQUIRE_NOTHROW(renderer.begin_frame());
            REQUIRE_NOTHROW(renderer.render(tree));
            REQUIRE_NOTHROW(renderer.end_frame());
        }

        SUBCASE("can render path with stroke") {
            render_tree tree;

            path_properties props;
            props.points = {
                point{100, 100},
                point{200, 100},
                point{200, 200},
                point{100, 200}
            };
            props.fill_color = color{1.0f, 1.0f, 0.0f, 1.0f};  // Yellow
            props.stroke_color = color{0.0f, 0.0f, 1.0f, 1.0f};  // Blue
            props.stroke_width = 3.0f;

            auto path_node = std::make_shared<rendering::path_node>(props);
            tree.set_root(path_node);

            REQUIRE_NOTHROW(renderer.begin_frame());
            REQUIRE_NOTHROW(renderer.render(tree));
            REQUIRE_NOTHROW(renderer.end_frame());
        }
    }

    TEST_CASE("metal_renderer visibility culling") {
        size viewport_size{800, 600};
        metal_renderer renderer(viewport_size, nullptr);

        SUBCASE("invisible node is not rendered") {
            render_tree tree;

            rectangle_properties props;
            props.bounds = rect{{100, 100}, {200, 150}};
            props.fill_color = color{1.0f, 0.0f, 0.0f, 1.0f};

            auto rect_node = std::make_shared<rectangle_node>(props);
            rect_node->set_visible(false);
            tree.set_root(rect_node);

            REQUIRE_NOTHROW(renderer.begin_frame());
            REQUIRE_NOTHROW(renderer.render(tree));
            REQUIRE_NOTHROW(renderer.end_frame());
        }

        SUBCASE("off-screen rectangle is culled") {
            render_tree tree;

            rectangle_properties props;
            props.bounds = rect{{-1000, -1000}, {100, 100}};  // Far off-screen
            props.fill_color = color{1.0f, 0.0f, 0.0f, 1.0f};

            auto rect_node = std::make_shared<rectangle_node>(props);
            tree.set_root(rect_node);

            REQUIRE_NOTHROW(renderer.begin_frame());
            REQUIRE_NOTHROW(renderer.render(tree));
            REQUIRE_NOTHROW(renderer.end_frame());
        }
    }

    TEST_CASE("metal_renderer complex scene") {
        size viewport_size{800, 600};
        metal_renderer renderer(viewport_size, nullptr);

        SUBCASE("can render mixed content") {
            render_tree tree;

            auto root = std::make_shared<group_node>();

            // Background rectangle
            rectangle_properties bg_props;
            bg_props.bounds = rect{{0, 0}, {800, 600}};
            bg_props.fill_color = color{0.9f, 0.9f, 0.9f, 1.0f};
            root->add_child(std::make_shared<rectangle_node>(bg_props));

            // Title text
            text_properties title_props;
            title_props.content = "stdui Rendering Test";
            title_props.position = point{50, 50};
            title_props.font_family = "Helvetica";
            title_props.font_size = 32.0f;
            title_props.text_color = color{0.0f, 0.0f, 0.0f, 1.0f};
            root->add_child(std::make_shared<rendering::text_node>(title_props));

            // Some colored rectangles
            for (int i = 0; i < 10; ++i) {
                rectangle_properties rect_props;
                rect_props.bounds = rect{{float(50 + i * 70), 150}, {60, 60}};
                rect_props.fill_color = color{
                    float(i) / 10.0f,
                    1.0f - float(i) / 10.0f,
                    0.5f,
                    1.0f
                };
                root->add_child(std::make_shared<rectangle_node>(rect_props));
            }

            tree.set_root(root);

            REQUIRE_NOTHROW(renderer.begin_frame());
            REQUIRE_NOTHROW(renderer.render(tree));
            REQUIRE_NOTHROW(renderer.end_frame());
        }
    }
}

#endif  // __APPLE__
