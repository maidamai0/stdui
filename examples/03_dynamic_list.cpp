/**
 * Example 3: Dynamic List
 *
 * Demonstrates dynamic collections with explicit identity.
 * Shows how to build lists from runtime data.
 */

#include <stdui/expressions.hpp>
#include <stdui/component.hpp>

#include <iostream>
#include <string>
#include <vector>

struct task {
    int id;
    std::string title;
    bool completed;
};

auto make_task_row(task const& t) {
    auto status = t.completed ? "[✓]" : "[ ]";
    return stdui::hstack(
        stdui::text(status),
        stdui::text(" "),
        stdui::text(t.title)
    );
}

struct task_list_tag {};

auto make_task_list(std::vector<task> tasks) {
    return stdui::component<task_list_tag>([tasks = std::move(tasks)](auto& ctx) {
        // Use dynamic_list for runtime-sized collections with explicit identity
        return stdui::vstack(
            stdui::text("Task List"),
            stdui::text("========="),
            stdui::text(""),
            stdui::dynamic_list(
                tasks,
                [](task const& t) { return t.id; },  // Explicit identity
                [](task const& t) {
                    return make_task_row(t);
                }
            )
        );
    });
}

auto make_dashboard(std::vector<task> tasks) {
    int total = tasks.size();
    int completed = 0;
    for (auto const& t : tasks) {
        if (t.completed) ++completed;
    }

    return stdui::vstack(
        stdui::text("Dashboard"),
        stdui::text("========="),
        stdui::text(""),
        stdui::hstack(
            stdui::text("Total: "),
            stdui::text(std::to_string(total))
        ),
        stdui::hstack(
            stdui::text("Completed: "),
            stdui::text(std::to_string(completed))
        ),
        stdui::hstack(
            stdui::text("Remaining: "),
            stdui::text(std::to_string(total - completed))
        ),
        stdui::text(""),
        make_task_list(std::move(tasks))
    );
}

int main() {
    std::cout << "stdui Example 3: Dynamic List\n";
    std::cout << "===============================\n\n";

    // Sample data
    std::vector<task> tasks = {
        {1, "Review pull request", true},
        {2, "Write documentation", false},
        {3, "Fix rendering bug", false},
        {4, "Update tests", true},
        {5, "Deploy to staging", false}
    };

    // Create dashboard with dynamic list
    auto dashboard = make_dashboard(tasks);

    std::cout << "Created dynamic task list with " << tasks.size() << " items\n";
    std::cout << "\nKey concepts:\n";
    std::cout << "- dynamic_list() for runtime-sized collections\n";
    std::cout << "- Explicit identity via ID function\n";
    std::cout << "- Identity ensures state persistence across reorders\n";
    std::cout << "- Each item can be independently stateful\n";

    return 0;
}
