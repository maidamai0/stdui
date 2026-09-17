// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "stdui/render/frustum_culler.hpp"
#include <doctest/doctest.h>

using namespace stdui;
using namespace stdui::rendering;

TEST_SUITE("frustum_culler") {
    TEST_CASE("frustum_culler viewport") {
        frustum_culler culler;

        SUBCASE("can set viewport") {
            rect viewport{{0, 0}, {800, 600}};
            REQUIRE_NOTHROW(culler.set_viewport(viewport));
        }
    }

    TEST_CASE("frustum_culler visibility testing") {
        frustum_culler culler;
        rect viewport{{0, 0}, {800, 600}};
        culler.set_viewport(viewport);

        SUBCASE("fully visible rectangle is visible") {
            rect bounds{{100, 100}, {200, 150}};
            CHECK(culler.is_visible(bounds));
        }

        SUBCASE("rectangle at viewport edge is visible") {
            rect bounds{{0, 0}, {100, 100}};
            CHECK(culler.is_visible(bounds));

            rect bounds2{{700, 500}, {100, 100}};
            CHECK(culler.is_visible(bounds2));
        }

        SUBCASE("rectangle completely off-screen left is not visible") {
            rect bounds{{-300, 100}, {200, 100}};
            CHECK_FALSE(culler.is_visible(bounds));
        }

        SUBCASE("rectangle completely off-screen right is not visible") {
            rect bounds{{900, 100}, {200, 100}};
            CHECK_FALSE(culler.is_visible(bounds));
        }

        SUBCASE("rectangle completely off-screen top is not visible") {
            rect bounds{{100, -300}, {200, 100}};
            CHECK_FALSE(culler.is_visible(bounds));
        }

        SUBCASE("rectangle completely off-screen bottom is not visible") {
            rect bounds{{100, 700}, {200, 100}};
            CHECK_FALSE(culler.is_visible(bounds));
        }

        SUBCASE("partially visible rectangle is visible") {
            // Overlaps left edge
            rect bounds1{{-50, 100}, {100, 100}};
            CHECK(culler.is_visible(bounds1));

            // Overlaps right edge
            rect bounds2{{750, 100}, {100, 100}};
            CHECK(culler.is_visible(bounds2));

            // Overlaps top edge
            rect bounds3{{100, -50}, {100, 100}};
            CHECK(culler.is_visible(bounds3));

            // Overlaps bottom edge
            rect bounds4{{100, 550}, {100, 100}};
            CHECK(culler.is_visible(bounds4));
        }

        SUBCASE("rectangle larger than viewport is visible") {
            rect bounds{{-100, -100}, {1000, 800}};
            CHECK(culler.is_visible(bounds));
        }
    }

    TEST_CASE("frustum_culler node testing") {
        frustum_culler culler;
        rect viewport{{0, 0}, {800, 600}};
        culler.set_viewport(viewport);

        SUBCASE("visible rectangle node passes culling") {
            rectangle_properties props;
            props.bounds = rect{{100, 100}, {200, 150}};
            props.fill_color = color{1.0f, 0.0f, 0.0f, 1.0f};

            auto rect_node = rectangle_node(props);
            CHECK(culler.should_render(rect_node));
        }

        SUBCASE("invisible rectangle node fails culling") {
            rectangle_properties props;
            props.bounds = rect{{100, 100}, {200, 150}};
            props.fill_color = color{1.0f, 0.0f, 0.0f, 1.0f};

            auto rect_node = rectangle_node(props);
            rect_node.set_visible(false);
            CHECK_FALSE(culler.should_render(rect_node));
        }

        SUBCASE("off-screen rectangle node fails culling") {
            rectangle_properties props;
            props.bounds = rect{{-500, 100}, {200, 150}};
            props.fill_color = color{1.0f, 0.0f, 0.0f, 1.0f};

            auto rect_node = rectangle_node(props);
            CHECK_FALSE(culler.should_render(rect_node));
        }

        SUBCASE("text node is always rendered (conservative)") {
            text_properties props;
            props.content = "Test";
            props.position = point{100, 100};
            props.font_family = "Helvetica";
            props.font_size = 16.0f;
            props.text_color = color{0.0f, 0.0f, 0.0f, 1.0f};

            auto text_node = rendering::text_node(props);
            CHECK(culler.should_render(text_node));
        }

        SUBCASE("group node is always rendered") {
            auto group = group_node();
            CHECK(culler.should_render(group));
        }

        SUBCASE("image node respects bounds") {
            image_properties props;
            props.texture_id = 1;
            props.bounds = rect{{100, 100}, {200, 150}};
            props.opacity = 1.0f;

            auto img_node = image_node(props);
            CHECK(culler.should_render(img_node));

            // Off-screen image
            props.bounds = rect{{-500, 100}, {200, 150}};
            auto img_node2 = image_node(props);
            CHECK_FALSE(culler.should_render(img_node2));
        }
    }

    TEST_CASE("frustum_culler edge cases") {
        frustum_culler culler;
        rect viewport{{0, 0}, {800, 600}};
        culler.set_viewport(viewport);

        SUBCASE("zero-size rectangle at origin") {
            rect bounds{{0, 0}, {0, 0}};
            // Zero-size rectangles touching the viewport should be visible
            CHECK(culler.is_visible(bounds));
        }

        SUBCASE("single-pixel rectangle is visible if in viewport") {
            rect bounds{{400, 300}, {1, 1}};
            CHECK(culler.is_visible(bounds));
        }

        SUBCASE("rectangle touching viewport corner is visible") {
            rect bounds{{799, 599}, {1, 1}};
            CHECK(culler.is_visible(bounds));
        }
    }
}
