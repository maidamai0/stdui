// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#ifdef __APPLE__

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "stdui/rendering/glyph_atlas.hpp"
#include <doctest/doctest.h>
#include <Metal/Metal.h>

using namespace stdui::rendering;

TEST_SUITE("glyph_atlas") {
    TEST_CASE("glyph_atlas initialization") {
        SUBCASE("can create atlas with valid dimensions") {
            REQUIRE_NOTHROW(glyph_atlas(1024, 1024));
            REQUIRE_NOTHROW(glyph_atlas(2048, 2048));
        }

        SUBCASE("atlas reports correct dimensions") {
            glyph_atlas atlas(1024, 1024);
            CHECK(atlas.width() == 1024);
            CHECK(atlas.height() == 1024);
        }
    }

    TEST_CASE("glyph_atlas caching") {
        glyph_atlas atlas(2048, 2048);

        // Create a Metal device for testing
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        REQUIRE(device != nullptr);
        atlas.set_metal_device(device);

        SUBCASE("can cache glyphs") {
            auto entry = atlas.get_or_create_glyph("Helvetica", 24.0f, 'A');
            REQUIRE(entry.has_value());
            CHECK(entry->advance > 0);
        }

        SUBCASE("cached glyph returns same texture coordinates") {
            auto entry1 = atlas.get_or_create_glyph("Helvetica", 24.0f, 'B');
            auto entry2 = atlas.get_or_create_glyph("Helvetica", 24.0f, 'B');

            REQUIRE(entry1.has_value());
            REQUIRE(entry2.has_value());

            CHECK(entry1->tex_coords.u0 == entry2->tex_coords.u0);
            CHECK(entry1->tex_coords.v0 == entry2->tex_coords.v0);
            CHECK(entry1->tex_coords.u1 == entry2->tex_coords.u1);
            CHECK(entry1->tex_coords.v1 == entry2->tex_coords.v1);
        }

        SUBCASE("different fonts create different glyphs") {
            auto entry1 = atlas.get_or_create_glyph("Helvetica", 24.0f, 'C');
            auto entry2 = atlas.get_or_create_glyph("Times", 24.0f, 'C');

            REQUIRE(entry1.has_value());
            REQUIRE(entry2.has_value());

            // Should have different texture coordinates (different glyphs)
            bool coords_different =
                entry1->tex_coords.u0 != entry2->tex_coords.u0 ||
                entry1->tex_coords.v0 != entry2->tex_coords.v0;
            CHECK(coords_different);
        }

        SUBCASE("different sizes create different glyphs") {
            auto entry1 = atlas.get_or_create_glyph("Helvetica", 16.0f, 'D');
            auto entry2 = atlas.get_or_create_glyph("Helvetica", 32.0f, 'D');

            REQUIRE(entry1.has_value());
            REQUIRE(entry2.has_value());

            // Different sizes should have different advances
            CHECK(entry1->advance != entry2->advance);
        }

        [device release];
    }

    TEST_CASE("glyph_atlas metrics") {
        glyph_atlas atlas(2048, 2048);

        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        REQUIRE(device != nullptr);
        atlas.set_metal_device(device);

        SUBCASE("glyph has valid bounds") {
            auto entry = atlas.get_or_create_glyph("Helvetica", 24.0f, 'M');
            REQUIRE(entry.has_value());

            CHECK(entry->bounds.extent.width > 0);
            CHECK(entry->bounds.extent.height > 0);
        }

        SUBCASE("glyph has valid advance width") {
            auto entry = atlas.get_or_create_glyph("Helvetica", 24.0f, 'W');
            REQUIRE(entry.has_value());
            CHECK(entry->advance > 0);
        }

        SUBCASE("texture coordinates are normalized") {
            auto entry = atlas.get_or_create_glyph("Helvetica", 24.0f, 'X');
            REQUIRE(entry.has_value());

            CHECK(entry->tex_coords.u0 >= 0.0f);
            CHECK(entry->tex_coords.u0 <= 1.0f);
            CHECK(entry->tex_coords.v0 >= 0.0f);
            CHECK(entry->tex_coords.v0 <= 1.0f);
            CHECK(entry->tex_coords.u1 >= 0.0f);
            CHECK(entry->tex_coords.u1 <= 1.0f);
            CHECK(entry->tex_coords.v1 >= 0.0f);
            CHECK(entry->tex_coords.v1 <= 1.0f);
        }

        [device release];
    }

    TEST_CASE("glyph_atlas can clear") {
        glyph_atlas atlas(2048, 2048);

        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        REQUIRE(device != nullptr);
        atlas.set_metal_device(device);

        SUBCASE("clearing atlas removes cached glyphs") {
            // Add some glyphs
            atlas.get_or_create_glyph("Helvetica", 24.0f, 'A');
            atlas.get_or_create_glyph("Helvetica", 24.0f, 'B');

            // Clear
            atlas.clear();

            // Should be able to add glyphs again
            auto entry = atlas.get_or_create_glyph("Helvetica", 24.0f, 'C');
            REQUIRE(entry.has_value());
        }

        [device release];
    }
}

#endif  // __APPLE__
