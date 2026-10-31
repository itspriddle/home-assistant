#include "dashboard.h"

#include <cmath>
#include <cstdio>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#include "climate_screen.h"
#include "clock_screen.h"
#include "home_screen.h"
#include "lights_screen.h"

namespace esphome {
namespace dashboard {

static const char *const TAG = "dashboard";

// After a local change, ignore incoming HA state for this long so the old
// value echoing back doesn't make the UI flicker.
static constexpr uint32_t LOCAL_HOLD_MS = 3000;
// Coalesce rapid +/- taps into one HA call
static constexpr uint32_t DEBOUNCE_MS = 700;
// Backlight fades: slow into dim, quick back out
static constexpr uint32_t DIM_TRANSITION_MS = 3000;
static constexpr uint32_t WAKE_TRANSITION_MS = 250;

// Scheduler timer IDs (one per light room, starting at TIMER_LIGHT_LEVEL)
static constexpr uint32_t TIMER_SETPOINT = 1;
static constexpr uint32_t TIMER_LIGHT_LEVEL = 100;

Dashboard::Dashboard() {
  this->screens_[static_cast<size_t>(ScreenId::HOME)] = std::make_unique<HomeScreen>();
  this->screens_[static_cast<size_t>(ScreenId::LIGHTS)] = std::make_unique<LightsScreen>();
  this->screens_[static_cast<size_t>(ScreenId::CLIMATE)] = std::make_unique<ClimateScreen>();
  this->screens_[static_cast<size_t>(ScreenId::CLOCK)] = std::make_unique<ClockScreen>();
}

void Dashboard::set_font(const std::string &role, font::Font *font) {
  const lv_font_t *f = font->get_lv_font();
  if (role == "caption")
    fonts.caption = f;
  else if (role == "small")
    fonts.small = f;
  else if (role == "body")
    fonts.body = f;
  else if (role == "label")
    fonts.label = f;
  else if (role == "value")
    fonts.value = f;
  else if (role == "title")
    fonts.title = f;
  else if (role == "large")
    fonts.large = f;
  else if (role == "display")
    fonts.display = f;
  else if (role == "clock")
    fonts.clock = f;
  else if (role == "clock_xl")
    fonts.clock_xl = f;
  else if (role == "icon_small")
    fonts.icon_small = f;
  else if (role == "icon_medium")
    fonts.icon_medium = f;
  else if (role == "icon_large")
    fonts.icon_large = f;
  else
    ESP_LOGW(TAG, "Unknown font role '%s'", role.c_str());
}

void Dashboard::setup() {
  for (auto &screen : this->screens_) {
    lv_obj_t *root = lv_obj_create(nullptr);
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, SCREEN_W, SCREEN_H);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    set_bg(root, color::BG);
    lv_obj_set_style_text_font(root, fonts.body, 0);
    lv_obj_set_style_text_color(root, hex(color::TEXT), 0);
    screen->set_root(root);
    screen->build(root, this);
    screen->update(this->model_, DIRTY_ALL);
  }
  this->built_ = true;
  this->dirty_ = 0;
  lv_scr_load(this->screens_[static_cast<size_t>(ScreenId::HOME)]->root());
}

void Dashboard::dump_config() {
  ESP_LOGCONFIG(TAG, "Dashboard:");
  ESP_LOGCONFIG(TAG, "  Idle timeout: %u ms", this->idle_timeout_ms_);
  ESP_LOGCONFIG(TAG, "  Clock timeout: %u ms", this->clock_timeout_ms_);
  ESP_LOGCONFIG(TAG, "  Dim timeout: %u ms", this->dim_timeout_ms_);
  ESP_LOGCONFIG(TAG, "  Dim brightness: %.0f%%", this->dim_brightness_ * 100.0f);
}

void Dashboard::loop() {
  if (!this->built_)
    return;

  // Clock: refresh on minute change
  if (this->time_ != nullptr) {
    auto now = this->time_->now();
    if (now.is_valid() && now.minute != this->last_minute_) {
      this->last_minute_ = now.minute;
      this->mark_(DIRTY_CLOCK);
    }
  }

  this->update_idle_();

  if (this->dirty_ == 0)
    return;
  uint32_t dirty = this->dirty_;
  this->dirty_ = 0;
  // Only the visible screen needs to redraw now; others catch up on show()
  this->screens_[static_cast<size_t>(this->current_)]->update(this->model_, dirty);
}

void Dashboard::show(ScreenId id) {
  if (id == this->current_)
    return;
  this->current_ = id;
  auto &screen = this->screens_[static_cast<size_t>(id)];
  screen->update(this->model_, DIRTY_ALL);
  lv_scr_load_anim(screen->root(), LV_SCR_LOAD_ANIM_FADE_ON, 150, 0, false);
}

// Inactivity (LVGL's timer, reset by touch and presence changes) drives
// Home after idle_timeout, the clock after clock_timeout, and a dimmed
// backlight after dim_timeout. While the room is occupied the clock and
// dimming are held off. Any activity wakes straight to Home.
void Dashboard::update_idle_() {
  uint32_t idle = lv_display_get_inactive_time(nullptr);
  bool clock = !this->present_ && this->clock_timeout_ms_ > 0 && idle >= this->clock_timeout_ms_;
  bool dim = !this->present_ && this->dim_timeout_ms_ > 0 && idle >= this->dim_timeout_ms_;

  if (clock) {
    this->show(ScreenId::CLOCK);
  } else if (this->current_ == ScreenId::CLOCK) {
    this->show(ScreenId::HOME);
  } else if (this->current_ != ScreenId::HOME && this->idle_timeout_ms_ > 0 && idle >= this->idle_timeout_ms_) {
    this->show(ScreenId::HOME);
  }

  if (dim != this->dimmed_)
    this->dim_(dim);
}

void Dashboard::dim_(bool dim) {
  this->dimmed_ = dim;
  if (this->backlight_ == nullptr)
    return;
  // Leave the backlight alone if it was switched off (e.g. from HA)
  if (!this->backlight_->remote_values.is_on())
    return;
  float brightness;
  uint32_t transition;
  if (dim) {
    this->wake_brightness_ = this->backlight_->remote_values.get_brightness();
    brightness = this->dim_brightness_;
    transition = DIM_TRANSITION_MS;
  } else {
    brightness = this->wake_brightness_;
    transition = WAKE_TRANSITION_MS;
  }
  ESP_LOGD(TAG, "%s backlight to %.0f%%", dim ? "Dimming" : "Restoring", brightness * 100.0f);
  auto call = this->backlight_->make_call();
  call.set_state(true);
  call.set_brightness(brightness);
  call.set_transition_length(transition);
  // Don't persist the dimmed level as the light's restore value
  call.set_save(false);
  call.perform();
}

std::string Dashboard::clock_text() const {
  if (this->time_ == nullptr)
    return "--:--";
  auto now = this->time_->now();
  if (!now.is_valid())
    return "--:--";
  int h = now.hour % 12;
  if (h == 0)
    h = 12;
  char buf[8];
  snprintf(buf, sizeof(buf), "%d:%02d", h, now.minute);
  return buf;
}

// ------------------------------------------------------------------
// Inputs
// ------------------------------------------------------------------

void Dashboard::set_weather_temperature(float t) {
  this->model_.weather.temperature = t;
  this->mark_(DIRTY_WEATHER);
}
void Dashboard::set_weather_condition(const std::string &c) {
  this->model_.weather.condition = c;
  this->mark_(DIRTY_WEATHER);
}
void Dashboard::set_today_high(float t) {
  this->model_.weather.today_high = t;
  this->mark_(DIRTY_WEATHER);
}
void Dashboard::set_today_low(float t) {
  this->model_.weather.today_low = t;
  this->mark_(DIRTY_WEATHER);
}
void Dashboard::set_forecast_day(int i, const std::string &day) {
  if (i < 0 || i >= 4)
    return;
  // HA gives full names ("Wednesday"); keep the short form
  this->model_.weather.days[i].day = day.substr(0, 3);
  this->mark_(DIRTY_WEATHER);
}
void Dashboard::set_forecast_condition(int i, const std::string &c) {
  if (i < 0 || i >= 4)
    return;
  this->model_.weather.days[i].condition = c;
  this->mark_(DIRTY_WEATHER);
}
void Dashboard::set_forecast_high(int i, float t) {
  if (i < 0 || i >= 4)
    return;
  this->model_.weather.days[i].high = t;
  this->mark_(DIRTY_WEATHER);
}
void Dashboard::set_forecast_low(int i, float t) {
  if (i < 0 || i >= 4)
    return;
  this->model_.weather.days[i].low = t;
  this->mark_(DIRTY_WEATHER);
}

void Dashboard::set_room_temperature(const std::string &room, float t) {
  if (auto *r = this->model_.room(room)) {
    r->temperature = t;
    this->mark_(DIRTY_ROOMS);
  }
}
void Dashboard::set_room_humidity(const std::string &room, float h) {
  if (auto *r = this->model_.room(room)) {
    r->humidity = h;
    this->mark_(DIRTY_ROOMS);
  }
}
void Dashboard::set_room_occupied(const std::string &room, bool occupied) {
  if (auto *r = this->model_.room(room)) {
    r->occupied = occupied;
    this->mark_(DIRTY_ROOMS);
  }
}

void Dashboard::set_light_on(const std::string &room, bool on) {
  auto *l = this->model_.light(room);
  if (l == nullptr || millis() < l->hold_until)
    return;
  l->on = on;
  if (!on)
    l->active_scene = -1;
  this->mark_(DIRTY_LIGHTS);
}
void Dashboard::set_light_brightness(const std::string &room, float brightness_255) {
  auto *l = this->model_.light(room);
  if (l == nullptr || millis() < l->hold_until)
    return;
  l->brightness_pct = std::isnan(brightness_255) ? 0 : static_cast<uint8_t>(std::lround(brightness_255 / 2.55f));
  this->mark_(DIRTY_LIGHTS);
}

void Dashboard::set_climate_current(float t) {
  this->model_.climate.current = t;
  this->mark_(DIRTY_CLIMATE);
}
void Dashboard::set_climate_humidity(float h) {
  this->model_.climate.humidity = h;
  this->mark_(DIRTY_CLIMATE);
}
void Dashboard::set_climate_setpoint(float t) {
  if (millis() < this->model_.climate.hold_until)
    return;
  this->model_.climate.setpoint = t;
  this->mark_(DIRTY_CLIMATE);
}
void Dashboard::set_climate_mode(const std::string &mode) {
  if (millis() < this->model_.climate.hold_until)
    return;
  this->model_.climate.mode = mode;
  this->mark_(DIRTY_CLIMATE);
}
void Dashboard::set_climate_action(const std::string &action) {
  this->model_.climate.action = action;
  this->mark_(DIRTY_CLIMATE);
}

void Dashboard::set_meeting(bool meeting) {
  this->model_.status.meeting = meeting;
  this->mark_(DIRTY_STATUS);
}
void Dashboard::set_next_event(const std::string &text) {
  this->model_.status.next_event = text;
  this->mark_(DIRTY_STATUS);
}
void Dashboard::set_printer_status(const std::string &status) {
  this->model_.status.printer_status = status;
  this->mark_(DIRTY_STATUS);
}
void Dashboard::set_printer_progress(float pct) {
  this->model_.status.printer_progress = pct;
  this->mark_(DIRTY_STATUS);
}

void Dashboard::set_presence(bool present) {
  if (present == this->present_)
    return;
  this->present_ = present;
  ESP_LOGD(TAG, "Presence %s", present ? "detected" : "cleared");
  // Arriving wakes the screen; leaving starts the idle countdown from now
  if (this->built_)
    lv_display_trigger_activity(nullptr);
}

// ------------------------------------------------------------------
// Outputs
// ------------------------------------------------------------------

void Dashboard::toggle_light(const std::string &room) {
  auto *l = this->model_.light(room);
  if (l == nullptr)
    return;
  l->on = !l->on;
  if (l->on && l->brightness_pct == 0)
    l->brightness_pct = 100;
  if (!l->on)
    l->active_scene = -1;
  l->hold_until = millis() + LOCAL_HOLD_MS;
  this->mark_(DIRTY_LIGHTS);
  if (this->light_toggle_ != nullptr)
    this->light_toggle_->trigger(l->entity_id);
}

void Dashboard::set_light_level(const std::string &room, int pct) {
  auto *l = this->model_.light(room);
  if (l == nullptr)
    return;
  pct = pct < 0 ? 0 : (pct > 100 ? 100 : pct);
  l->brightness_pct = pct;
  l->on = pct > 0;
  l->active_scene = -1;
  l->hold_until = millis() + DEBOUNCE_MS + LOCAL_HOLD_MS;
  this->mark_(DIRTY_LIGHTS);
  std::string entity = l->entity_id;
  auto timer_id = TIMER_LIGHT_LEVEL + static_cast<uint32_t>(l - this->model_.lights.data());
  this->set_timeout(timer_id, DEBOUNCE_MS, [this, entity, pct]() {
    if (this->light_brightness_ != nullptr)
      this->light_brightness_->trigger(entity, pct);
  });
}

void Dashboard::lights_off() {
  uint32_t hold = millis() + LOCAL_HOLD_MS;
  for (auto &l : this->model_.lights) {
    l.on = false;
    l.active_scene = -1;
    l.hold_until = hold;
  }
  this->mark_(DIRTY_LIGHTS);
  if (this->lights_off_ != nullptr)
    this->lights_off_->trigger();
}

void Dashboard::activate_scene(const std::string &room, int scene_index) {
  auto *l = this->model_.light(room);
  if (l == nullptr || scene_index < 0 || scene_index >= static_cast<int>(l->scenes.size()))
    return;
  l->active_scene = scene_index;
  l->on = true;
  l->hold_until = millis() + LOCAL_HOLD_MS;
  this->mark_(DIRTY_LIGHTS);
  if (this->scene_ != nullptr)
    this->scene_->trigger(l->scenes[scene_index].entity_id);
}

void Dashboard::set_setpoint(float t) {
  auto &c = this->model_.climate;
  c.setpoint = t;
  c.hold_until = millis() + DEBOUNCE_MS + LOCAL_HOLD_MS;
  this->mark_(DIRTY_CLIMATE);
  this->set_timeout(TIMER_SETPOINT, DEBOUNCE_MS, [this, t]() {
    if (this->climate_temperature_ != nullptr)
      this->climate_temperature_->trigger(t);
  });
}

void Dashboard::set_mode(const std::string &mode) {
  auto &c = this->model_.climate;
  c.mode = mode;
  c.hold_until = millis() + LOCAL_HOLD_MS;
  this->mark_(DIRTY_CLIMATE);
  if (this->climate_mode_ != nullptr)
    this->climate_mode_->trigger(mode);
}

}  // namespace dashboard
}  // namespace esphome
