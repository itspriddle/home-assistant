#pragma once

#include <array>
#include <memory>
#include <string>

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/components/font/font.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/time/real_time_clock.h"
#include "lvgl.h"

#include "model.h"
#include "screen.h"
#include "theme.h"

namespace esphome {
namespace dashboard {

class Dashboard : public Component {
 public:
  Dashboard();

  // ---- config (from __init__.py) ----
  void set_time(time::RealTimeClock *time) { this->time_ = time; }
  void set_idle_timeout(uint32_t ms) { this->idle_timeout_ms_ = ms; }
  void set_clock_timeout(uint32_t ms) { this->clock_timeout_ms_ = ms; }
  void set_dim_timeout(uint32_t ms) { this->dim_timeout_ms_ = ms; }
  void set_dim_brightness(float b) { this->dim_brightness_ = b; }
  void set_backlight(light::LightState *backlight) { this->backlight_ = backlight; }
  void set_font(const std::string &role, font::Font *font);

  void add_on_light_toggle(Trigger<std::string> *t) { this->light_toggle_ = t; }
  void add_on_light_brightness(Trigger<std::string, int> *t) { this->light_brightness_ = t; }
  void add_on_lights_off(Trigger<> *t) { this->lights_off_ = t; }
  void add_on_scene(Trigger<std::string> *t) { this->scene_ = t; }
  void add_on_climate_temperature(Trigger<float> *t) { this->climate_temperature_ = t; }
  void add_on_climate_mode(Trigger<std::string> *t) { this->climate_mode_ = t; }

  // ---- component ----
  void setup() override;
  void loop() override;
  void dump_config() override;
  // After LVGL (PROCESSOR) and fonts
  float get_setup_priority() const override { return setup_priority::LATE; }

  // ---- inputs: called from YAML homeassistant sensors ----
  void set_weather_temperature(float t);
  void set_weather_condition(const std::string &c);
  void set_today_high(float t);
  void set_today_low(float t);
  void set_forecast_day(int i, const std::string &day);  // i = 0..3, 0 = tomorrow
  void set_forecast_condition(int i, const std::string &c);
  void set_forecast_high(int i, float t);
  void set_forecast_low(int i, float t);

  void set_room_temperature(const std::string &room, float t);
  void set_room_humidity(const std::string &room, float h);
  void set_room_occupied(const std::string &room, bool occupied);

  void set_light_on(const std::string &room, bool on);
  void set_light_brightness(const std::string &room, float brightness_255);

  void set_climate_current(float t);
  void set_climate_humidity(float h);
  void set_climate_setpoint(float t);
  void set_climate_mode(const std::string &mode);
  void set_climate_action(const std::string &action);

  void set_meeting(bool meeting);
  void set_next_event(const std::string &text);
  void set_printer_status(const std::string &status);
  void set_printer_progress(float pct);

  // Room occupancy: holds off the clock / dimming while present, and
  // arriving wakes the screen
  void set_presence(bool present);

  // ---- outputs: called by screens on touch ----
  // These update the model optimistically, then fire the YAML trigger.
  void toggle_light(const std::string &room);
  void set_light_level(const std::string &room, int pct);
  void lights_off();
  void activate_scene(const std::string &room, int scene_index);
  void set_setpoint(float t);
  void set_mode(const std::string &mode);

  // ---- navigation ----
  void show(ScreenId id);
  ScreenId current() const { return this->current_; }

  const Model &model() const { return this->model_; }
  time::RealTimeClock *time() const { return this->time_; }
  std::string clock_text() const;  // "4:23" (12h, no leading zero)

 protected:
  void mark_(uint32_t dirty) { this->dirty_ |= dirty; }
  void update_idle_();
  void dim_(bool dim);

  Model model_;
  time::RealTimeClock *time_{nullptr};
  uint32_t idle_timeout_ms_{60000};
  uint32_t clock_timeout_ms_{600000};
  uint32_t dim_timeout_ms_{1200000};
  float dim_brightness_{0.01f};
  light::LightState *backlight_{nullptr};

  bool present_{false};
  bool dimmed_{false};
  float wake_brightness_{1.0f};  // backlight level to restore on wake

  std::array<std::unique_ptr<Screen>, static_cast<size_t>(ScreenId::COUNT)> screens_;
  ScreenId current_{ScreenId::HOME};
  uint32_t dirty_{DIRTY_ALL};
  bool built_{false};
  int last_minute_{-1};

  Trigger<std::string> *light_toggle_{nullptr};
  Trigger<std::string, int> *light_brightness_{nullptr};
  Trigger<> *lights_off_{nullptr};
  Trigger<std::string> *scene_{nullptr};
  Trigger<float> *climate_temperature_{nullptr};
  Trigger<std::string> *climate_mode_{nullptr};
};

}  // namespace dashboard
}  // namespace esphome
