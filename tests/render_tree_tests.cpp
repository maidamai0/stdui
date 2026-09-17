#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// Copyright (c) 2026 stdui
// SPDX-License-Identifier: MIT

#include <doctest/doctest.h>

#include "stdui/rendering/render_tree_builder.hpp"
#include "stdui/rendering/render_node.hpp"
#include "stdui/layout_tree.hpp"

#include <variant>

using namespace stdui;
using namespace stdui::rendering;

TEST_CASE("render_tree_builder - basic construction") {
    render_tree_builder builder;

    SUBCASE("Empty layout node") {
        layout_node root;
        rect viewport{{0, 0}, {800, 600}};
        auto tree = builder.build(root, viewport);

        REQUIRE(tree != nullptr);
        REQUIRE(tree->root() != nullptr);
        REQUIRE(tree->frame_number() == 0);
    }

    SUBCASE("Frame number increments") {
        layout_node root;
        rect viewport{{0, 0}, {800, 600}};

        auto tree1 = builder.build(root, viewport);
        REQUIRE(tree1->frame_number() == 0);

        auto tree2 = builder.build(root, viewport);
        REQUIRE(tree2->frame_number() == 1);

        auto tree3 = builder.build(root, viewport);
        REQUIRE(tree3->frame_number() == 2);
    }
}

TEST_CASE("render_tree_builder - culling") {
    render_tree_builder builder;
    rect viewport{{0, 0}, {800, 600}};

    SUBCASE("Node renders within viewport") {
        layout_node root;
        auto tree = builder.build(root, viewport);
        REQUIRE(tree->root() != nullptr);
    }
}

TEST_CASE("render_tree_builder - children") {
    render_tree_builder builder;
    rect viewport{{0, 0}, {800, 600}};

    SUBCASE("Node with children creates group") {
        layout_node root;
        layout_node child1;
        layout_node child2;

        root.children.push_back(child1);
        root.children.push_back(child2);

        auto tree = builder.build(root, viewport);
        REQUIRE(tree->root() != nullptr);
        REQUIRE(tree->root()->type() == render_node_type::group);

        auto group = std::static_pointer_cast<group_node>(tree->root());
        REQUIRE(group->children().size() == 3);  // Background + 2 children
    }
}

TEST_CASE("render_node - basic properties") {
    SUBCASE("Default node constructors") {
        REQUIRE(rectangle_node{}.type() == render_node_type::rectangle);
        REQUIRE(text_node{}.type() == render_node_type::text);
        REQUIRE(path_node{}.type() == render_node_type::path);
        REQUIRE(image_node{}.type() == render_node_type::image);
        REQUIRE(group_node{}.type() == render_node_type::group);
        REQUIRE(effect_node{}.type() == render_node_type::effect);
        REQUIRE(scene_view_node{}.type() == render_node_type::scene_view);

        mat3 identity = mat3::identity();
        REQUIRE(identity.m[0] == 1.0f);
        REQUIRE(identity.m[4] == 1.0f);
        REQUIRE(identity.m[8] == 1.0f);
    }

    SUBCASE("Rectangle node") {
        auto node = std::make_shared<rectangle_node>();

        REQUIRE(node->type() == render_node_type::rectangle);
        REQUIRE(node->opacity() == 1.0f);
        REQUIRE(node->is_visible() == true);
        REQUIRE(!node->transform().has_value());
        REQUIRE(!node->clip_rect().has_value());

        // Set properties
        node->set_opacity(0.5f);
        REQUIRE(node->opacity() == 0.5f);

        node->set_visible(false);
        REQUIRE(node->is_visible() == false);

        mat3 transform = mat3::identity();
        node->set_transform(transform);
        REQUIRE(node->transform().has_value());

        rect clip{{0, 0}, {100, 100}};
        node->set_clip_rect(clip);
        REQUIRE(node->clip_rect().has_value());
    }

    SUBCASE("Text node") {
        auto node = std::make_shared<text_node>();

        REQUIRE(node->type() == render_node_type::text);

        auto& props = node->properties();
        props.content = "Hello, World!";
        props.font_size = 16.0f;

        REQUIRE(node->properties().content == "Hello, World!");
        REQUIRE(node->properties().font_size == 16.0f);
    }

    SUBCASE("Path node") {
        path_properties props;
        props.commands = {1, 2, 3};
        props.points = {point{0, 0}, point{10, 0}, point{0, 10}};
        props.fill_color = color{0.0f, 1.0f, 0.0f, 1.0f};
        props.stroke_color = color::black();
        props.stroke_width = 2.0f;

        path_node node(props);
        REQUIRE(node.type() == render_node_type::path);
        REQUIRE(node.properties().commands.size() == 3);
        REQUIRE(node.properties().points.size() == 3);
        REQUIRE(node.properties().stroke_width == 2.0f);

        node.properties().stroke_width = 4.0f;
        REQUIRE(node.properties().stroke_width == 4.0f);

        const path_node const_node(props);
        REQUIRE(const_node.properties().commands.size() == 3);
    }

    SUBCASE("Image node") {
        image_properties props;
        props.bounds = rect{{10, 20}, {30, 40}};
        props.texture_id = 42;
        props.opacity = 0.5f;

        image_node node(props);
        REQUIRE(node.type() == render_node_type::image);
        REQUIRE(node.properties().texture_id == 42);
        REQUIRE(node.properties().opacity == 0.5f);

        node.properties().opacity = 0.75f;
        REQUIRE(node.properties().opacity == 0.75f);

        const image_node const_node(props);
        REQUIRE(const_node.properties().texture_id == 42);
    }

    SUBCASE("Group node") {
        auto group = std::make_shared<group_node>();

        REQUIRE(group->type() == render_node_type::group);
        REQUIRE(group->children().empty());

        auto child1 = std::make_shared<rectangle_node>();
        auto child2 = std::make_shared<text_node>();

        group->add_child(child1);
        group->add_child(child2);

        REQUIRE(group->children().size() == 2);
    }

    SUBCASE("Effect node") {
        auto effect = std::make_shared<effect_node>();

        REQUIRE(effect->type() == render_node_type::effect);

        blur_effect blur{5.0f};
        effect->set_effect(effect_type::blur);
        effect->set_parameters(blur);

        auto child = std::make_shared<rectangle_node>();
        effect->set_child(child);

        REQUIRE(effect->effect() == effect_type::blur);
        REQUIRE(std::holds_alternative<blur_effect>(effect->parameters()));
        REQUIRE(effect->child() != nullptr);
    }

    SUBCASE("Scene view node") {
        auto scene = std::make_shared<scene_view_node>();

        REQUIRE(scene->type() == render_node_type::scene_view);

        rect viewport{{0, 0}, {800, 600}};
        scene->set_viewport(viewport);
        scene->set_scene_id(42);

        REQUIRE(scene->viewport().extent.width == 800);
        REQUIRE(scene->scene_id() == 42);
    }
}

TEST_CASE("render_tree - basic properties") {
    render_tree tree;

    SUBCASE("Empty tree") {
        REQUIRE(tree.root() == nullptr);
        REQUIRE(tree.frame_number() == 0);
    }

    SUBCASE("Set root") {
        auto root = std::make_shared<rectangle_node>();
        tree.set_root(root);

        REQUIRE(tree.root() != nullptr);
        REQUIRE(tree.root()->type() == render_node_type::rectangle);
    }

    SUBCASE("Frame number") {
        tree.set_frame_number(42);
        REQUIRE(tree.frame_number() == 42);
    }
}
