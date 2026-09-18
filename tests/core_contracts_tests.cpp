#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdui/core/animation.hpp>
#include <stdui/core/effects.hpp>
#include <stdui/core/layout_options.hpp>
#include <stdui/render/target.hpp>
#include <stdui/core/semantics.hpp>
#include <stdui/core/text_measurement.hpp>

#include <memory>
#include <variant>

namespace {

class recording_text_measurer : public stdui::text_measurer {
public:
  auto measure(std::string_view text, stdui::font_descriptor const &font) const
      -> stdui::size override {
    ++measure_calls;
    last_text = std::string(text);
    last_font = font;
    return {100.0, 20.0};
  }

  auto measure_wrapped(std::string_view text, stdui::font_descriptor const &font,
                       double max_width) const -> stdui::size override {
    ++wrapped_calls;
    last_wrap_width = max_width;
    return {max_width, 40.0};
  }

  mutable int measure_calls = 0;
  mutable int wrapped_calls = 0;
  mutable std::string last_text;
  mutable stdui::font_descriptor last_font;
  mutable double last_wrap_width = 0.0;
};

} // namespace

TEST_CASE("core color exposes logical channels") {
  stdui::color value{0.1f, 0.2f, 0.3f, 0.4f};

  CHECK(value.red == doctest::Approx(0.1f));
  CHECK(value.green == doctest::Approx(0.2f));
  CHECK(value.blue == doctest::Approx(0.3f));
  CHECK(value.alpha == doctest::Approx(0.4f));
}

TEST_CASE("core color supplies standard values") {
  CHECK(stdui::color::black() == stdui::color{0.0f, 0.0f, 0.0f, 1.0f});
  CHECK(stdui::color::white() == stdui::color{1.0f, 1.0f, 1.0f, 1.0f});
  CHECK(stdui::color::red_color() == stdui::color{1.0f, 0.0f, 0.0f, 1.0f});
  CHECK(stdui::color::green_color() == stdui::color{0.0f, 1.0f, 0.0f, 1.0f});
  CHECK(stdui::color::blue_color() == stdui::color{0.0f, 0.0f, 1.0f, 1.0f});
  CHECK(stdui::color::transparent() == stdui::color{0.0f, 0.0f, 0.0f, 0.0f});
}

TEST_CASE("core color converts between byte and packed representations") {
  auto color = stdui::color::from_rgb(255, 128, 0, 64);

  CHECK(color.red == doctest::Approx(1.0f));
  CHECK(color.green == doctest::Approx(128.0f / 255.0f));
  CHECK(color.blue == doctest::Approx(0.0f));
  CHECK(color.alpha == doctest::Approx(64.0f / 255.0f));
  CHECK(color.to_rgba() == 0xFF800040U);
}

TEST_CASE("core effects describe requested behavior") {
  stdui::visual_effect effect = stdui::shadow_effect{
      .offset = {2.0, 3.0},
      .blur_radius = 4.0f,
      .shadow_color = stdui::color::black(),
  };

  REQUIRE(std::holds_alternative<stdui::shadow_effect>(effect));
  auto const &shadow = std::get<stdui::shadow_effect>(effect);
  CHECK(shadow.offset == stdui::point{2.0, 3.0});
  CHECK(shadow.blur_radius == doctest::Approx(4.0f));
}

TEST_CASE("core animation describes timing without executing it") {
  stdui::animation_spec animation{
      .curve = {.kind = stdui::animation_curve_kind::spring, .duration_seconds = 0.4},
      .repeat = false,
      .autoreverse = true,
  };

  CHECK(animation.curve.kind == stdui::animation_curve_kind::spring);
  CHECK(animation.curve.duration_seconds == doctest::Approx(0.4));
  CHECK(animation.autoreverse);
}

TEST_CASE("core semantics preserve hierarchy and state") {
  stdui::semantic_node button{
      .role = stdui::semantic_role::button,
      .label = "Save",
      .state = {.disabled = true},
      .actions = {stdui::semantic_action::activate},
  };
  stdui::semantic_node root{
      .role = stdui::semantic_role::generic,
      .label = "Toolbar",
      .children = {button},
  };

  REQUIRE(root.children.size() == 1);
  CHECK(root.children[0].role == stdui::semantic_role::button);
  CHECK(root.children[0].label == "Save");
  CHECK(root.children[0].state.disabled);
}

TEST_CASE("render target separates logical and physical size") {
  stdui::render_target_descriptor target{
      .kind = stdui::render_target_kind::image,
      .logical_size = {400.0, 300.0},
      .scale = 2.0,
  };

  CHECK(target.physical_size() == stdui::size{800.0, 600.0});
}

TEST_CASE("layout option factories expose reusable track and inset values") {
  auto insets = stdui::edge_insets::all(4.0);
  CHECK(insets.left == 4.0);
  CHECK(insets.top == 4.0);
  CHECK(insets.right == 4.0);
  CHECK(insets.bottom == 4.0);

  auto fixed = stdui::grid_track::fixed(40.0);
  CHECK(fixed.type == stdui::grid_track::kind::fixed);
  CHECK(fixed.size == 40.0);

  auto flexible = stdui::grid_track::flexible(10.0);
  CHECK(flexible.type == stdui::grid_track::kind::flexible);
  CHECK(flexible.size == 10.0);

  auto tracks = stdui::repeat_track(flexible, 3);
  REQUIRE(tracks.size() == 3);
  CHECK(tracks[2].size == 10.0);
}

TEST_CASE("cached text measurer delegates to its backend") {
  auto backend = std::make_unique<recording_text_measurer>();
  auto *backend_ptr = backend.get();
  stdui::cached_text_measurer measurer(std::move(backend));

  stdui::font_descriptor font{.family = "Test", .size = 18.0};
  CHECK(measurer.measure("hello", font) == stdui::size{100.0, 20.0});
  CHECK(measurer.measure_wrapped("hello", font, 50.0) == stdui::size{50.0, 40.0});

  CHECK(backend_ptr->measure_calls == 1);
  CHECK(backend_ptr->wrapped_calls == 1);
  CHECK(backend_ptr->last_text == "hello");
  CHECK(backend_ptr->last_font == font);
  CHECK(backend_ptr->last_wrap_width == doctest::Approx(50.0));
}
