/**
 * Example 2: Stateful Counter
 *
 * Demonstrates component state management and identity.
 * Shows how state persists across view expression evaluations.
 */

#include <stdui/component.hpp>
#include <stdui/expressions.hpp>
#include <stdui/state.hpp>

#include <iostream>
#include <string>

// Component tag for unique identity
struct counter_component_tag {};

auto make_counter(std::string label) {
    return stdui::component<counter_component_tag>([label = std::move(label)](auto& ctx) {
        // Framework-managed state - persists across evaluations
        auto count = ctx.state(0);

        // Build the view
        return stdui::vstack(
            stdui::text(label),
            stdui::text("Count: " + std::to_string(count.get())),
            stdui::text("[+] Increment"),  // Placeholder for button
            stdui::text("[-] Decrement")
        );
    });
}

struct multi_counter_tag {};

auto make_multi_counter() {
    return stdui::component<multi_counter_tag>([](auto& ctx) {
        auto counter_a = ctx.state(0);
        auto counter_b = ctx.state(0);
        auto total = ctx.state(0);

        // Calculate total
        total.set(counter_a.get() + counter_b.get());

        return stdui::vstack(
            stdui::text("Multiple Counters"),
            stdui::text("=================="),
            stdui::text(""),
            stdui::text("Counter A: " + std::to_string(counter_a.get())),
            stdui::text("Counter B: " + std::to_string(counter_b.get())),
            stdui::text(""),
            stdui::text("Total: " + std::to_string(total.get()))
        );
    });
}

int main() {
    std::cout << "stdui Example 2: Stateful Counter\n";
    std::cout << "===================================\n\n";

    // Create stateful components
    auto counter = make_counter("My Counter");
    auto multi = make_multi_counter();

    std::cout << "Created stateful components:\n";
    std::cout << "- Single counter with persistent state\n";
    std::cout << "- Multi-counter with computed total\n\n";

    std::cout << "State is managed by the framework and persists\n";
    std::cout << "across view expression evaluations.\n\n";

    std::cout << "Component identity is determined by:\n";
    std::cout << "1. Component type (tag)\n";
    std::cout << "2. Structural position or explicit ID\n";

    return 0;
}
