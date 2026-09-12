#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <stdui/runtime_contracts.hpp>

#include <chrono>

namespace {

class manual_frame_scheduler : public stdui::frame_scheduler {
public:
  void request_frame() override { ++frame_requests; }

  int frame_requests = 0;
};

} // namespace

TEST_CASE("manual clock advances deterministically") {
  stdui::manual_clock clock;

  CHECK(clock.now() == stdui::runtime_time::zero());

  clock.advance(std::chrono::milliseconds(16));
  CHECK(clock.now() == std::chrono::milliseconds(16));
}

TEST_CASE("queued event source preserves event order") {
  stdui::queued_event_source source;
  source.push(stdui::mouse_event{{10.0, 20.0}});
  source.push(stdui::keyboard_event{.key = "Enter"});

  REQUIRE(source.size() == 2);

  auto first = source.poll();
  REQUIRE(first.has_value());
  CHECK(std::holds_alternative<stdui::mouse_event>(*first));

  auto second = source.poll();
  REQUIRE(second.has_value());
  CHECK(std::holds_alternative<stdui::keyboard_event>(*second));

  CHECK_FALSE(source.poll().has_value());
  CHECK(source.empty());
}

TEST_CASE("frame scheduler requests frames without platform dependencies") {
  manual_frame_scheduler scheduler;

  scheduler.request_frame();
  scheduler.request_frame();

  CHECK(scheduler.frame_requests == 2);
}
