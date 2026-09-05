/**
 * Example 4: Application Layout
 *
 * Demonstrates a typical application layout with:
 * - Side panel navigation
 * - Main content area
 * - Status bar
 *
 * This represents the target use case: heavy rendering in the main area
 * with UI controls in side panels.
 */

#include <stdui/expressions.hpp>
#include <stdui/layout.hpp>
#include <stdui/geometry.hpp>

#include <iostream>
#include <string>
#include <vector>

// Navigation item
struct nav_item {
    std::string icon;
    std::string label;
    bool active;
};

auto make_nav_item(nav_item const& item) {
    auto prefix = item.active ? "▶ " : "  ";
    return stdui::hstack(
        stdui::text(prefix + item.icon),
        stdui::text(" "),
        stdui::text(item.label)
    );
}

auto make_sidebar(std::vector<nav_item> const& items) {
    return stdui::vstack(
        stdui::text("Navigation"),
        stdui::text("=========="),
        stdui::text(""),
        stdui::dynamic_list(
            items,
            [](auto const& item) { return &item; },
            [](auto const& item) { return make_nav_item(item); }
        )
    );
}

auto make_viewport() {
    return stdui::vstack(
        stdui::text("┌─────────────────────────────────┐"),
        stdui::text("│                                 │"),
        stdui::text("│     Main Rendering Area         │"),
        stdui::text("│                                 │"),
        stdui::text("│  (Heavy graphics rendering)     │"),
        stdui::text("│                                 │"),
        stdui::text("│  - 3D viewport                  │"),
        stdui::text("│  - Canvas                       │"),
        stdui::text("│  - Video player                 │"),
        stdui::text("│  - Game scene                   │"),
        stdui::text("│                                 │"),
        stdui::text("└─────────────────────────────────┘")
    );
}

auto make_properties_panel() {
    return stdui::vstack(
        stdui::text("Properties"),
        stdui::text("=========="),
        stdui::text(""),
        stdui::text("Transform"),
        stdui::text("  X: 0.0"),
        stdui::text("  Y: 0.0"),
        stdui::text("  Z: 0.0"),
        stdui::text(""),
        stdui::text("Material"),
        stdui::text("  Color: #FFFFFF"),
        stdui::text("  Opacity: 1.0"),
        stdui::text(""),
        stdui::text("Render"),
        stdui::text("  FPS: 60"),
        stdui::text("  Draw calls: 142")
    );
}

auto make_status_bar() {
    return stdui::hstack(
        stdui::text("Ready"),
        stdui::text(" | "),
        stdui::text("Vertices: 12,450"),
        stdui::text(" | "),
        stdui::text("Memory: 256 MB")
    );
}

auto make_application_layout() {
    // Grid-based application layout
    auto grid_opts = stdui::grid_options{};
    grid_opts.columns = 3;

    return stdui::vstack(
        stdui::text("═══════════════════════════════════════════════════════════"),
        stdui::text("                    Application Title                      "),
        stdui::text("═══════════════════════════════════════════════════════════"),
        stdui::text(""),

        // Main application area (3-column layout)
        stdui::hstack(
            // Left sidebar (navigation)
            stdui::vstack(
                make_sidebar({
                    {"📁", "Files", true},
                    {"🔧", "Tools", false},
                    {"⚙️", "Settings", false},
                    {"📊", "Stats", false}
                }),
                stdui::text(""),
                stdui::text("────────────")
            ),

            stdui::text("  │  "),  // Separator

            // Main viewport (largest area)
            make_viewport(),

            stdui::text("  │  "),  // Separator

            // Right properties panel
            make_properties_panel()
        ),

        stdui::text(""),
        stdui::text("───────────────────────────────────────────────────────────"),
        make_status_bar()
    );
}

int main() {
    std::cout << "stdui Example 4: Application Layout\n";
    std::cout << "=====================================\n\n";

    auto app = make_application_layout();

    std::cout << "This demonstrates the target use case:\n\n";
    std::cout << "┌─────────────┬───────────────────────┬─────────────┐\n";
    std::cout << "│             │                       │             │\n";
    std::cout << "│  Sidebar    │   Main Viewport       │ Properties  │\n";
    std::cout << "│  (Nav)      │   (Heavy rendering)   │   Panel     │\n";
    std::cout << "│             │                       │             │\n";
    std::cout << "└─────────────┴───────────────────────┴─────────────┘\n";
    std::cout << "                   Status Bar                        \n\n";

    std::cout << "Design principles:\n";
    std::cout << "- Majority of window for rendering (viewport)\n";
    std::cout << "- Side panels for navigation/settings\n";
    std::cout << "- Lightweight UI (Neovim/Blender style)\n";
    std::cout << "- Native graphics APIs for the viewport\n";
    std::cout << "- Simple primitives for UI chrome\n";

    return 0;
}
