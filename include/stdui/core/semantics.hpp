#pragma once

#include <string>
#include <vector>

namespace stdui {

/// Platform-independent accessibility role.
enum class semantic_role {
  generic,
  text,
  heading,
  button,
  link,
  image,
  list,
  list_item,
  text_field,
  check_box,
  switch_control,
  slider,
  dialog,
  menu,
  menu_item,
  tab,
  table,
  row,
  cell,
};

/// Actions exposed by a semantic node.
enum class semantic_action {
  activate,
  increment,
  decrement,
  set_value,
  focus,
  dismiss,
};

/// Reusable state exposed to accessibility and automation clients.
struct semantic_state {
  bool disabled = false;
  bool selected = false;
  bool checked = false;
  bool focused = false;
  bool expanded = false;

  bool operator==(semantic_state const &) const = default;
};

/// Platform-independent accessibility and automation description.
struct semantic_node {
  semantic_role role = semantic_role::generic;
  std::string label;
  std::string value;
  std::string description;
  semantic_state state;
  std::vector<semantic_action> actions;
  std::vector<semantic_node> children;

  bool operator==(semantic_node const &) const = default;
};

} // namespace stdui
