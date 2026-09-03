#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdui/platform.hpp>

TEST_CASE("null_platform: can be initialized and shut down") {
  stdui::null_platform platform;

  CHECK_NOTHROW(platform.initialize());
  CHECK_NOTHROW(platform.shutdown());
}

TEST_CASE("null_platform: creates windows") {
  stdui::null_platform platform;
  platform.initialize();

  auto window = platform.create_window({800.0, 600.0}, "Test Window");
  REQUIRE(window != nullptr);

  CHECK(window->size() == stdui::size{800.0, 600.0});
  CHECK(window->title() == "Test Window");
}

TEST_CASE("null_window: supports size changes") {
  stdui::null_platform platform;
  auto window = platform.create_window({800.0, 600.0}, "Test");

  window->set_size({1024.0, 768.0});
  CHECK(window->size() == stdui::size{1024.0, 768.0});
}

TEST_CASE("null_window: supports title changes") {
  stdui::null_platform platform;
  auto window = platform.create_window({800.0, 600.0}, "Original");

  window->set_title("Updated");
  CHECK(window->title() == "Updated");
}

TEST_CASE("null_window: provides renderer") {
  stdui::null_platform platform;
  auto window = platform.create_window({800.0, 600.0}, "Test");

  auto &renderer = window->renderer();
  CHECK_NOTHROW(renderer.begin_frame());
  CHECK_NOTHROW(renderer.clear(stdui::color::white()));
  CHECK_NOTHROW(renderer.end_frame());
}

TEST_CASE("null_window: provides event dispatcher") {
  stdui::null_platform platform;
  auto window = platform.create_window({800.0, 600.0}, "Test");

  auto &dispatcher = window->event_dispatcher();

  bool handler_called = false;
  auto handler_id = dispatcher.add_handler([&](stdui::platform_event const &) {
    handler_called = true;
    return true;
  });

  dispatcher.dispatch(stdui::mouse_event{{10.0, 20.0}});
  CHECK(handler_called);

  dispatcher.remove_handler(handler_id);
}

TEST_CASE("null_window: redraw callback") {
  stdui::null_platform platform;
  auto window = platform.create_window({800.0, 600.0}, "Test");

  bool redraw_called = false;
  window->set_redraw_callback([&] { redraw_called = true; });

  window->request_redraw();
  CHECK(redraw_called);
}

TEST_CASE("null_renderer: supports graphics state operations") {
  stdui::null_platform platform;
  auto window = platform.create_window({800.0, 600.0}, "Test");
  auto &renderer = window->renderer();

  CHECK_NOTHROW(renderer.save());
  CHECK_NOTHROW(renderer.translate(10.0, 20.0));
  CHECK_NOTHROW(renderer.clip_rect({{0.0, 0.0}, {100.0, 100.0}}));
  CHECK_NOTHROW(renderer.restore());
}

TEST_CASE("null_renderer: supports drawing operations") {
  stdui::null_platform platform;
  auto window = platform.create_window({800.0, 600.0}, "Test");
  auto &renderer = window->renderer();

  renderer.begin_frame();
  CHECK_NOTHROW(renderer.fill_rect({{10.0, 10.0}, {50.0, 50.0}}, stdui::color::black()));
  CHECK_NOTHROW(renderer.stroke_rect({{20.0, 20.0}, {30.0, 30.0}}, stdui::color::white(),
                                     stdui::stroke_style{}));
  CHECK_NOTHROW(renderer.draw_text("Hello", {100.0, 100.0}, stdui::font_descriptor{},
                                   stdui::color::black()));
  renderer.end_frame();
}

TEST_CASE("null_text_measurer: provides heuristic measurements") {
  stdui::null_platform platform;
  auto const &factory = platform.text_measurer_factory();
  auto measurer = factory.create();

  auto size = measurer->measure("Hello", stdui::font_descriptor{});
  CHECK(size.width == 40.0);  // 5 chars * 8 pixels
  CHECK(size.height == 16.0); // Fixed height

  auto wrapped = measurer->measure_wrapped("Hello World", stdui::font_descriptor{}, 50.0);
  CHECK(wrapped.width == 50.0); // Clamped to max_width
  CHECK(wrapped.height == 32.0); // 2 lines * 16 pixels
}

TEST_CASE("simple_event_dispatcher: dispatches events to handlers") {
  stdui::simple_event_dispatcher dispatcher;

  int call_count = 0;
  dispatcher.add_handler([&](stdui::platform_event const &) {
    ++call_count;
    return false; // Don't stop propagation
  });
  dispatcher.add_handler([&](stdui::platform_event const &) {
    ++call_count;
    return false;
  });

  dispatcher.dispatch(stdui::mouse_event{{0.0, 0.0}});
  CHECK(call_count == 2);
}

TEST_CASE("simple_event_dispatcher: stops propagation when handler returns true") {
  stdui::simple_event_dispatcher dispatcher;

  int call_count = 0;
  dispatcher.add_handler([&](stdui::platform_event const &) {
    ++call_count;
    return true; // Stop propagation
  });
  dispatcher.add_handler([&](stdui::platform_event const &) {
    ++call_count;
    return false;
  });

  dispatcher.dispatch(stdui::mouse_event{{0.0, 0.0}});
  CHECK(call_count == 1); // Second handler not called
}

TEST_CASE("simple_event_dispatcher: removes handlers") {
  stdui::simple_event_dispatcher dispatcher;

  int call_count = 0;
  auto id = dispatcher.add_handler([&](stdui::platform_event const &) {
    ++call_count;
    return false;
  });

  dispatcher.dispatch(stdui::mouse_event{{0.0, 0.0}});
  CHECK(call_count == 1);

  dispatcher.remove_handler(id);
  dispatcher.dispatch(stdui::mouse_event{{0.0, 0.0}});
  CHECK(call_count == 1); // Handler not called after removal
}

TEST_CASE("color: predefined colors") {
  CHECK(stdui::color::black() == stdui::color{0.0, 0.0, 0.0, 1.0});
  CHECK(stdui::color::white() == stdui::color{1.0, 1.0, 1.0, 1.0});
  CHECK(stdui::color::transparent() == stdui::color{0.0, 0.0, 0.0, 0.0});
}

TEST_CASE("font_descriptor: default values") {
  stdui::font_descriptor font;
  CHECK(font.family == "system");
  CHECK(font.size == 14.0);
  CHECK(font.weight == stdui::font_descriptor::weight_t::regular);
  CHECK(font.style == stdui::font_descriptor::style_t::normal);
}

TEST_CASE("keyboard_modifiers: equality") {
  stdui::keyboard_modifiers mod1{.shift = true, .control = false};
  stdui::keyboard_modifiers mod2{.shift = true, .control = false};
  stdui::keyboard_modifiers mod3{.shift = false, .control = true};

  CHECK(mod1 == mod2);
  CHECK_FALSE(mod1 == mod3);
}

TEST_CASE("platform_event: variant holds different event types") {
  stdui::platform_event mouse_evt = stdui::mouse_event{{10.0, 20.0}};
  CHECK(std::holds_alternative<stdui::mouse_event>(mouse_evt));

  stdui::platform_event keyboard_evt = stdui::keyboard_event{"Enter"};
  CHECK(std::holds_alternative<stdui::keyboard_event>(keyboard_evt));

  stdui::platform_event resize_evt = stdui::resize_event{{800.0, 600.0}};
  CHECK(std::holds_alternative<stdui::resize_event>(resize_evt));
}
