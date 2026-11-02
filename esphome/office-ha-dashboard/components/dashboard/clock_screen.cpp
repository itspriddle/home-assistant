#include "clock_screen.h"

#include <cmath>
#include <string>

#include "dashboard.h"
#include "theme.h"

namespace esphome {
namespace dashboard {

static constexpr int NEXT_EVENT_W = 640;

static void hide(lv_obj_t *obj, bool hidden) {
  if (hidden)
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  else
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

void ClockScreen::build(lv_obj_t *root, Dashboard *dash) {
  this->dash_ = dash;

  // Catch the wake-up touch here so it can't land on a widget of the
  // screen we switch to
  lv_obj_add_flag(root, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *page = box(root);
  lv_obj_set_size(page, SCREEN_W, SCREEN_H);
  lv_obj_clear_flag(page, LV_OBJ_FLAG_CLICKABLE);
  flex_col(page, 0, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  this->clock_ = label(page, fonts.clock_xl, color::TEXT, "--:--");
  lv_obj_set_style_text_letter_space(this->clock_, -10, 0);
  this->date_ = label(page, fonts.title, color::MUTED, "");
  lv_obj_set_style_pad_bottom(this->date_, 36, 0);

  lv_obj_t *summary = box(page);
  lv_obj_set_size(summary, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_clear_flag(summary, LV_OBJ_FLAG_CLICKABLE);
  flex_row(summary, 14);
  this->weather_icon_ = label(summary, fonts.icon_medium, color::TEXT_2, "");
  this->weather_temp_ = label(summary, fonts.value, color::TEXT_2, "--°");
  this->separator_ = label(summary, fonts.value, color::OFF_DOT, "·");
  this->next_event_ = label(summary, fonts.body, color::MUTED, "");
  lv_obj_set_style_max_width(this->next_event_, NEXT_EVENT_W, 0);
  lv_label_set_long_mode(this->next_event_, LV_LABEL_LONG_DOT);
}

void ClockScreen::update(const Model &model, uint32_t dirty) {
  if (dirty & DIRTY_CLOCK)
    this->update_clock_();
  if (dirty & (DIRTY_WEATHER | DIRTY_STATUS))
    this->update_summary_(model);
}

void ClockScreen::update_clock_() {
  set_text(this->clock_, this->dash_->clock_text());

  std::string date;
  auto *rtc = this->dash_->time();
  if (rtc != nullptr) {
    auto now = rtc->now();
    if (now.is_valid())
      date = now.strftime("%A, %B ") + std::to_string(now.day_of_month);
  }
  set_text(this->date_, date);
}

void ClockScreen::update_summary_(const Model &model) {
  const auto &w = model.weather;
  bool has_weather = !std::isnan(w.temperature);
  set_text(this->weather_icon_, icon::weather(w.condition));
  set_text(this->weather_temp_, fmt_temp(w.temperature));
  hide(this->weather_icon_, !has_weather);
  hide(this->weather_temp_, !has_weather);

  const auto &next = model.status.next_event;
  set_text(this->next_event_, next);
  hide(this->next_event_, next.empty());
  hide(this->separator_, !has_weather || next.empty());
}

}  // namespace dashboard
}  // namespace esphome
