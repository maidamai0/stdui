/**
 * Example 1: Simple Layout
 *
 * Demonstrates basic layout primitives: text, vstack, hstack, and overlay.
 * This is a headless example showing the DSL API.
 */

#include <stdui/expressions.hpp>
#include <stdui/layout.hpp>
#include <stdui/geometry.hpp>

#include <iostream>
#include <string>

auto make_header(std::string title) {
    return stdui::vstack(
        stdui::text(std::move(title)),
        stdui::text("================")
    );
}

auto make_info_row(std::string label, std::string value) {
    return stdui::hstack(
        stdui::text(std::move(label) + ": "),
        stdui::text(std::move(value))
    );
}

auto make_contact_card() {
    return stdui::vstack(
        make_header("Contact Card"),
        stdui::text(""),  // Spacer
        make_info_row("Name", "John Doe"),
        make_info_row("Role", "Software Engineer"),
        make_info_row("Email", "john.doe@example.com"),
        make_info_row("Phone", "+1-555-0123"),
        stdui::text(""),
        stdui::text("Last updated: 2026-09-04")
    );
}

auto make_status_badge(std::string status) {
    // Overlay demonstration: status text over background indicator
    return stdui::overlay(
        stdui::text("[ " + status + " ]"),
        stdui::text("●")  // Status indicator
    );
}

int main() {
    std::cout << "stdui Example 1: Simple Layout\n";
    std::cout << "================================\n\n";

    // Create the view expression
    auto card = make_contact_card();

    // In a complete application, this would be:
    // 1. Reconciled against previous frame
    // 2. Measured with layout constraints
    // 3. Arranged into final geometry
    // 4. Rendered through a backend

    std::cout << "View expression created successfully.\n";
    std::cout << "This demonstrates stdui's compositional DSL.\n\n";

    // Example: Measure the layout (simplified)
    auto proposal = stdui::proposal::bounded(400.0, 300.0);
    std::cout << "Layout proposal: 400x300 logical units\n";

    // Example: Create a status overlay
    auto status = make_status_badge("Online");
    std::cout << "\nStatus badge created with overlay primitive.\n";

    return 0;
}
