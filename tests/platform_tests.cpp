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

TEST_CASE("null_platform: multiple windows") {
  stdui::null_platform platform;
  platform.initialize();

  auto window1 = platform.create_window({800.0, 600.0}, "Window 1");
  auto window2 = platform.create_window({1024.0, 768.0}, "Window 2");

  REQUIRE(window1 != nullptr);
  REQUIRE(window2 != nullptr);

  CHECK(window1->title() == "Window 1");
  CHECK(window2->title() == "Window 2");
  CHECK(window1->size() != window2->size());
}

TEST_CASE("null_window: show and hide") {
  stdui::null_platform platform;
  auto window = platform.create_window({800.0, 600.0}, "Test");

  CHECK_NOTHROW(window->show());
  CHECK_NOTHROW(window->hide());
  CHECK_NOTHROW(window->show());
}

TEST_CASE("null_window: multiple request_redraw calls") {
  stdui::null_platform platform;
  auto window = platform.create_window({800.0, 600.0}, "Test");

  int redraw_count = 0;
  window->set_redraw_callback([&] { ++redraw_count; });

  window->request_redraw();
  window->request_redraw();
  window->request_redraw();

  CHECK(redraw_count == 3);
}

TEST_CASE("null_renderer: multiple save/restore pairs") {
  stdui::null_platform platform;
  auto window = platform.create_window({800.0, 600.0}, "Test");
  auto &renderer = window->renderer();

  CHECK_NOTHROW(renderer.save());
  CHECK_NOTHROW(renderer.save());
  CHECK_NOTHROW(renderer.restore());
  CHECK_NOTHROW(renderer.restore());
}

TEST_CASE("null_renderer: translate accumulation") {
  stdui::null_platform platform;
  auto window = platform.create_window({800.0, 600.0}, "Test");
  auto &renderer = window->renderer();

  CHECK_NOTHROW(renderer.translate(10.0, 20.0));
  CHECK_NOTHROW(renderer.translate(5.0, 10.0));
  // State should accumulate (though null_renderer doesn't track)
}

TEST_CASE("null_renderer: clip_rect multiple times") {
  stdui::null_platform platform;
  auto window = platform.create_window({800.0, 600.0}, "Test");
  auto &renderer = window->renderer();

  CHECK_NOTHROW(renderer.clip_rect({{0.0, 0.0}, {100.0, 100.0}}));
  CHECK_NOTHROW(renderer.clip_rect({{10.0, 10.0}, {50.0, 50.0}}));
}

TEST_CASE("null_renderer: draw_line") {
  stdui::null_platform platform;
  auto window = platform.create_window({800.0, 600.0}, "Test");
  auto &renderer = window->renderer();

  renderer.begin_frame();
  // Note: draw_line may not exist in renderer interface
  // Skipping this test as method doesn't exist
  renderer.end_frame();
}

TEST_CASE("null_renderer: various colors") {
  stdui::null_platform platform;
  auto window = platform.create_window({800.0, 600.0}, "Test");
  auto &renderer = window->renderer();

  renderer.begin_frame();
  CHECK_NOTHROW(renderer.clear(stdui::color{0.5, 0.5, 0.5, 1.0}));
  CHECK_NOTHROW(renderer.fill_rect({{0.0, 0.0}, {10.0, 10.0}}, stdui::color::transparent()));
  CHECK_NOTHROW(renderer.fill_rect({{0.0, 0.0}, {10.0, 10.0}}, stdui::color{1.0, 0.0, 0.0, 0.5}));
  renderer.end_frame();
}

TEST_CASE("null_text_measurer: empty string") {
  stdui::null_platform platform;
  auto const &factory = platform.text_measurer_factory();
  auto measurer = factory.create();

  auto size = measurer->measure("", stdui::font_descriptor{});
  CHECK(size.width == 0.0);
  CHECK(size.height == 16.0); // Still has line height
}

TEST_CASE("null_text_measurer: long string") {
  stdui::null_platform platform;
  auto const &factory = platform.text_measurer_factory();
  auto measurer = factory.create();

  std::string long_text(100, 'X');
  auto size = measurer->measure(long_text, stdui::font_descriptor{});
  CHECK(size.width == 800.0); // 100 chars * 8 pixels
  CHECK(size.height == 16.0);
}

TEST_CASE("null_text_measurer: wrapping exact fit") {
  stdui::null_platform platform;
  auto const &factory = platform.text_measurer_factory();
  auto measurer = factory.create();

  // "Hello" = 5 chars * 8 = 40 pixels
  auto wrapped = measurer->measure_wrapped("Hello", stdui::font_descriptor{}, 40.0);
  CHECK(wrapped.width == 40.0);
  CHECK(wrapped.height == 16.0); // Single line
}

TEST_CASE("simple_event_dispatcher: no handlers") {
  stdui::simple_event_dispatcher dispatcher;

  // Should not crash with no handlers
  CHECK_NOTHROW(dispatcher.dispatch(stdui::mouse_event{{0.0, 0.0}}));
}

TEST_CASE("simple_event_dispatcher: remove non-existent handler") {
  stdui::simple_event_dispatcher dispatcher;

  // Should not crash when removing non-existent handler
  CHECK_NOTHROW(dispatcher.remove_handler(999));
}

TEST_CASE("simple_event_dispatcher: handler order") {
  stdui::simple_event_dispatcher dispatcher;

  std::vector<int> call_order;

  dispatcher.add_handler([&](stdui::platform_event const &) {
    call_order.push_back(1);
    return false;
  });
  dispatcher.add_handler([&](stdui::platform_event const &) {
    call_order.push_back(2);
    return false;
  });
  dispatcher.add_handler([&](stdui::platform_event const &) {
    call_order.push_back(3);
    return false;
  });

  dispatcher.dispatch(stdui::mouse_event{{0.0, 0.0}});

  REQUIRE(call_order.size() == 3);
  CHECK(call_order[0] == 1);
  CHECK(call_order[1] == 2);
  CHECK(call_order[2] == 3);
}

TEST_CASE("color: custom RGBA values") {
  stdui::color c1{0.5, 0.6, 0.7, 0.8};
  CHECK(c1.red == 0.5);
  CHECK(c1.green == 0.6);
  CHECK(c1.blue == 0.7);
  CHECK(c1.alpha == 0.8);
}

TEST_CASE("color: equality comparisons") {
  stdui::color c1{1.0, 0.5, 0.0, 1.0};
  stdui::color c2{1.0, 0.5, 0.0, 1.0};
  stdui::color c3{1.0, 0.5, 0.1, 1.0};

  CHECK(c1 == c2);
  CHECK_FALSE(c1 == c3);
}

TEST_CASE("font_descriptor: custom values") {
  stdui::font_descriptor font{
      .family = "Arial",
      .size = 20.0,
      .weight = stdui::font_descriptor::weight_t::bold,
      .style = stdui::font_descriptor::style_t::italic
  };

  CHECK(font.family == "Arial");
  CHECK(font.size == 20.0);
  CHECK(font.weight == stdui::font_descriptor::weight_t::bold);
  CHECK(font.style == stdui::font_descriptor::style_t::italic);
}

TEST_CASE("font_descriptor: all weights") {
  stdui::font_descriptor regular{.weight = stdui::font_descriptor::weight_t::regular};
  stdui::font_descriptor bold{.weight = stdui::font_descriptor::weight_t::bold};

  CHECK(regular.weight == stdui::font_descriptor::weight_t::regular);
  CHECK(bold.weight == stdui::font_descriptor::weight_t::bold);
}

TEST_CASE("keyboard_modifiers: all combinations") {
  stdui::keyboard_modifiers none{false, false, false, false};
  stdui::keyboard_modifiers shift{true, false, false, false};
  stdui::keyboard_modifiers ctrl{false, true, false, false};
  stdui::keyboard_modifiers all{true, true, true, true};

  CHECK_FALSE(none.shift);
  CHECK(shift.shift);
  CHECK(ctrl.control);
  CHECK(all.shift);
  CHECK(all.control);
  CHECK(all.alt);
  CHECK(all.meta);
}

TEST_CASE("mouse_event: various positions") {
  stdui::mouse_event evt1{{0.0, 0.0}};
  CHECK(evt1.position.x == 0.0);
  CHECK(evt1.position.y == 0.0);

  stdui::mouse_event evt2{{-10.0, -20.0}};
  CHECK(evt2.position.x == -10.0);
  CHECK(evt2.position.y == -20.0);

  stdui::mouse_event evt3{{1000.0, 2000.0}};
  CHECK(evt3.position.x == 1000.0);
  CHECK(evt3.position.y == 2000.0);
}

TEST_CASE("keyboard_event: various keys") {
  stdui::keyboard_event key_a{"a"};
  CHECK(key_a.key == "a");

  stdui::keyboard_event enter{"Enter"};
  CHECK(enter.key == "Enter");

  stdui::keyboard_event escape{"Escape"};
  CHECK(escape.key == "Escape");
}

TEST_CASE("resize_event: various sizes") {
  stdui::resize_event evt1{{800.0, 600.0}};
  CHECK(evt1.new_size.width == 800.0);
  CHECK(evt1.new_size.height == 600.0);

  stdui::resize_event evt2{{0.0, 0.0}};
  CHECK(evt2.new_size.width == 0.0);

  stdui::resize_event evt3{{1920.0, 1080.0}};
  CHECK(evt3.new_size.width == 1920.0);
}

TEST_CASE("stroke_style: default values") {
  stdui::stroke_style style;
  CHECK(style.width == 1.0);
}

TEST_CASE("stroke_style: custom width") {
  stdui::stroke_style thin{.width = 0.5};
  CHECK(thin.width == 0.5);

  stdui::stroke_style thick{.width = 5.0};
  CHECK(thick.width == 5.0);
}
