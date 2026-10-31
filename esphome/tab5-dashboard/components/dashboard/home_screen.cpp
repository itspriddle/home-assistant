#include "home_screen.h"

#include <cmath>
#include <string>

#include "dashboard.h"
#include "theme.h"

namespace esphome {
namespace dashboard {

// Layout (from the Home mock, 1280x720)
static constexpr int PAD_V = 36;
static constexpr int PAD_H = 40;
static constexpr int GAP = 20;
static constexpr int ROOM_H = 100;
static constexpr int TILE_H = 112;
// Clock column vs weather card is 1.15 : 1 of the 1180px top row
static constexpr int HERO_W = 631;

static void hide(lv_obj_t *obj, bool hidden) {
  if (hidden)
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  else
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

// HA weather condition -> display text
static std::string condition_text(const std::string &c) {
  if (c.empty())
    return "";
  if (c == "partlycloudy")
    return "Partly cloudy";
  if (c == "clear-night")
    return "Clear";
  if (c == "snowy-rainy")
    return "Sleet";
  if (c == "lightning-rainy")
    return "Thunderstorms";
  if (c == "lightning")
    return "Lightning";
  if (c == "windy-variant")
    return "Windy";
  if (c == "exceptional")
    return "Severe weather";
  std::string out = c;
  out[0] = static_cast<char>(toupper(static_cast<unsigned char>(out[0])));
  return out;
}

static uint32_t weather_icon_color(const std::string &c) {
  if (c == "sunny" || c == "clear" || c == "partlycloudy")
    return color::AMBER;
  return color::TEXT_2;
}

// ------------------------------------------------------------------
// Build
// ------------------------------------------------------------------

void HomeScreen::build(lv_obj_t *root, Dashboard *dash) {
  this->dash_ = dash;

  lv_obj_t *page = box(root);
  lv_obj_set_size(page, SCREEN_W, SCREEN_H);
  lv_obj_set_style_pad_ver(page, PAD_V, 0);
  lv_obj_set_style_pad_hor(page, PAD_H, 0);
  flex_col(page, GAP);

  // Top: clock/status (flex 1.15) + weather card (flex 1)
  lv_obj_t *top = box(page);
  lv_obj_set_width(top, lv_pct(100));
  lv_obj_set_flex_grow(top, 1);
  flex_row(top, GAP, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  this->build_hero_(top);
  this->build_weather_(top);

  this->build_rooms_(page);
  this->build_tiles_(page);
}

void HomeScreen::build_hero_(lv_obj_t *parent) {
  lv_obj_t *hero = box(parent);
  lv_obj_set_size(hero, HERO_W, lv_pct(100));
  lv_obj_set_style_pad_top(hero, 8, 0);
  lv_obj_set_style_pad_hor(hero, 8, 0);
  lv_obj_set_style_pad_bottom(hero, 4, 0);
  flex_col(hero, 0, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START);

  lv_obj_t *head = box(hero);
  lv_obj_set_size(head, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  flex_col(head, 0);
  this->clock_ = label(head, fonts.clock, color::TEXT, "--:--");
  lv_obj_set_style_text_letter_space(this->clock_, -6, 0);
  // Inter's line box has generous leading; pull the date up under the digits
  this->date_ = label(head, fonts.label, color::MUTED, "");

  // Always laid out (even when empty) so the clock stays pinned to the top
  this->status_row_ = box(hero);
  lv_obj_set_size(this->status_row_, lv_pct(100), 44);
  flex_row(this->status_row_, 14);

  this->meeting_chip_ = box(this->status_row_, color::MEETING_BG, 22);
  lv_obj_set_size(this->meeting_chip_, LV_SIZE_CONTENT, 44);
  lv_obj_set_style_pad_hor(this->meeting_chip_, 18, 0);
  flex_row(this->meeting_chip_, 10);
  dot(this->meeting_chip_, 10, color::MEETING_DOT);
  label(this->meeting_chip_, fonts.small, color::MEETING_FG, "In a meeting");

  this->next_event_ = label(this->status_row_, fonts.body, color::MUTED, "");
  lv_obj_set_flex_grow(this->next_event_, 1);
  lv_label_set_long_mode(this->next_event_, LV_LABEL_LONG_DOT);
}

void HomeScreen::build_weather_(lv_obj_t *parent) {
  lv_obj_t *card = box(parent, color::SURFACE, 28);
  lv_obj_set_height(card, lv_pct(100));
  lv_obj_set_flex_grow(card, 1);
  lv_obj_set_style_pad_ver(card, 28, 0);
  lv_obj_set_style_pad_hor(card, 32, 0);
  flex_col(card, 0, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START);

  lv_obj_t *head = box(card);
  lv_obj_set_size(head, lv_pct(100), LV_SIZE_CONTENT);
  flex_row(head, 12, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START);

  lv_obj_t *info = box(head);
  lv_obj_set_size(info, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_grow(info, 1);
  flex_col(info, 6);
  this->weather_temp_ = label(info, fonts.large, color::TEXT, "--°");
  lv_obj_set_style_text_letter_space(this->weather_temp_, -3, 0);
  this->weather_condition_ = label(info, fonts.body, color::TEXT_2, "");
  this->weather_hilo_ = label(info, fonts.small, color::MUTED, "");

  this->weather_icon_ = label(head, fonts.icon_large, color::TEXT_2, icon::WEATHER_PARTLY_CLOUDY);

  lv_obj_t *forecast = box(card);
  lv_obj_set_size(forecast, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_border_side(forecast, LV_BORDER_SIDE_TOP, 0);
  lv_obj_set_style_border_width(forecast, 1, 0);
  lv_obj_set_style_border_color(forecast, hex(color::LINE), 0);
  lv_obj_set_style_border_opa(forecast, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_top(forecast, 18, 0);
  flex_row(forecast, 8, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

  for (auto &f : this->forecast_) {
    f.col = box(forecast);
    lv_obj_set_height(f.col, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(f.col, 1);
    flex_col(f.col, 4);
    f.day = label(f.col, fonts.caption, color::MUTED, "");
    lv_obj_t *temps = box(f.col);
    lv_obj_set_size(temps, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    flex_row(temps, 6, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END);
    f.high = label(temps, fonts.label, color::TEXT, "--°");
    f.low = label(temps, fonts.body, color::DIM, "--°");
    lv_obj_set_style_pad_bottom(f.low, 2, 0);
  }
}

void HomeScreen::build_rooms_(lv_obj_t *parent) {
  lv_obj_t *strip = box(parent);
  lv_obj_set_size(strip, lv_pct(100), ROOM_H);
  flex_row(strip, 16);

  for (auto &r : this->rooms_) {
    r.tile = box(strip, color::SURFACE, 22);
    lv_obj_set_height(r.tile, ROOM_H);
    lv_obj_set_flex_grow(r.tile, 1);
    lv_obj_set_style_pad_ver(r.tile, 16, 0);
    lv_obj_set_style_pad_hor(r.tile, 20, 0);
    flex_col(r.tile, 0, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START);

    lv_obj_t *name_row = box(r.tile);
    lv_obj_set_size(name_row, lv_pct(100), LV_SIZE_CONTENT);
    flex_row(name_row, 8);
    r.dot = dot(name_row, 8, color::OFF_DOT);
    r.name = label(name_row, fonts.caption, color::MUTED, "");
    lv_obj_set_flex_grow(r.name, 1);
    lv_label_set_long_mode(r.name, LV_LABEL_LONG_DOT);

    lv_obj_t *val_row = box(r.tile);
    lv_obj_set_size(val_row, lv_pct(100), LV_SIZE_CONTENT);
    flex_row(val_row, 8, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END);
    r.temp = label(val_row, fonts.value, color::TEXT, "--°");
    r.humidity = label(val_row, fonts.small, color::DIM, "");
    lv_obj_set_style_pad_bottom(r.humidity, 4, 0);
  }
}

void HomeScreen::build_tiles_(lv_obj_t *parent) {
  lv_obj_t *row = box(parent);
  lv_obj_set_size(row, lv_pct(100), TILE_H);
  flex_row(row, 16);

  auto make_tile = [&](lv_obj_t **icon_out, const char *glyph, uint32_t icon_color) {
    lv_obj_t *tile = box(row, color::SURFACE, 26);
    lv_obj_set_height(tile, TILE_H);
    lv_obj_set_flex_grow(tile, 1);
    lv_obj_set_style_pad_hor(tile, 28, 0);
    flex_row(tile, 20);
    *icon_out = label(tile, fonts.icon_medium, icon_color, glyph);
    lv_obj_t *text = box(tile);
    lv_obj_set_height(text, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(text, 1);
    return text;
  };

  // Lights
  lv_obj_t *text = make_tile(&this->lights_icon_, icon::LIGHTBULB_OUTLINE, color::MUTED);
  this->lights_tile_ = lv_obj_get_parent(text);
  flex_col(text, 2);
  this->lights_title_ = label(text, fonts.label, color::TEXT, "Lights");
  this->lights_sub_ = label(text, fonts.small, color::MUTED, "All off");
  lv_obj_set_width(this->lights_sub_, lv_pct(100));
  lv_label_set_long_mode(this->lights_sub_, LV_LABEL_LONG_DOT);
  Dashboard *dash = this->dash_;
  on_click(this->lights_tile_, [dash]() { dash->show(ScreenId::LIGHTS); });

  // Climate
  text = make_tile(&this->climate_icon_, icon::THERMOMETER, color::HEAT);
  flex_col(text, 2);
  label(text, fonts.label, color::TEXT, "Climate");
  this->climate_sub_ = label(text, fonts.small, color::MUTED, "--");
  lv_obj_set_width(this->climate_sub_, lv_pct(100));
  lv_label_set_long_mode(this->climate_sub_, LV_LABEL_LONG_DOT);
  on_click(lv_obj_get_parent(text), [dash]() { dash->show(ScreenId::CLIMATE); });

  // Printer
  lv_obj_t *printer_icon = nullptr;
  text = make_tile(&printer_icon, icon::PRINTER_3D, color::PRINTER);
  flex_col(text, 8);
  lv_obj_t *head = box(text);
  lv_obj_set_size(head, lv_pct(100), LV_SIZE_CONTENT);
  flex_row(head, 8);
  this->printer_title_ = label(head, fonts.label, color::TEXT, "P2S");
  lv_obj_set_flex_grow(this->printer_title_, 1);
  this->printer_pct_ = label(head, fonts.small, color::MUTED, "");
  this->printer_bar_ = bar(text, 100, 8, color::LINE, color::PRINTER);
  lv_obj_set_width(this->printer_bar_, lv_pct(100));
}

// ------------------------------------------------------------------
// Update
// ------------------------------------------------------------------

void HomeScreen::update(const Model &model, uint32_t dirty) {
  if (dirty & DIRTY_CLOCK)
    this->update_clock_();
  if (dirty & DIRTY_WEATHER)
    this->update_weather_(model);
  if (dirty & DIRTY_ROOMS)
    this->update_rooms_(model);
  if (dirty & DIRTY_LIGHTS)
    this->update_lights_(model);
  if (dirty & DIRTY_CLIMATE)
    this->update_climate_(model);
  if (dirty & DIRTY_STATUS)
    this->update_status_(model);
}

void HomeScreen::update_clock_() {
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

void HomeScreen::update_weather_(const Model &model) {
  const auto &w = model.weather;
  set_text(this->weather_temp_, fmt_temp(w.temperature));
  set_text(this->weather_condition_, condition_text(w.condition));
  hide(this->weather_condition_, w.condition.empty());

  if (std::isnan(w.today_high) && std::isnan(w.today_low)) {
    set_text(this->weather_hilo_, "");
  } else {
    set_text(this->weather_hilo_, "H " + fmt_temp(w.today_high) + " · L " + fmt_temp(w.today_low));
  }

  set_text(this->weather_icon_, icon::weather(w.condition));
  set_text_color(this->weather_icon_, weather_icon_color(w.condition));

  for (size_t i = 0; i < FORECAST_DAYS; i++) {
    const auto &d = w.days[i];
    auto &f = this->forecast_[i];
    bool empty = d.day.empty() && std::isnan(d.high) && std::isnan(d.low);
    // Keep the column (so the grid stays 4-wide) but blank it out
    lv_obj_set_style_opa(f.col, empty ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    set_text(f.day, d.day.empty() ? "--" : d.day);
    set_text(f.high, fmt_temp(d.high));
    set_text(f.low, fmt_temp(d.low));
  }
}

void HomeScreen::update_rooms_(const Model &model) {
  for (size_t i = 0; i < ROOM_TILES; i++) {
    auto &t = this->rooms_[i];
    if (i >= model.rooms.size()) {
      hide(t.tile, true);
      continue;
    }
    const auto &r = model.rooms[i];
    hide(t.tile, false);
    set_text(t.name, r.name);
    hide(t.dot, !r.has_occupancy);
    set_bg(t.dot, r.occupied ? color::OCCUPIED : color::OFF_DOT);
    bool is_thermostat = std::string(r.key) == "thermostat";
    set_text(t.temp, fmt_temp(r.temperature, is_thermostat ? 0 : 1));
    set_text(t.humidity, fmt_pct(r.humidity));
    hide(t.humidity, std::isnan(r.humidity));
  }
}

void HomeScreen::update_lights_(const Model &model) {
  int on = model.lights_on();
  bool any = on > 0;

  set_bg(this->lights_tile_, any ? color::AMBER : color::SURFACE);
  set_text(this->lights_icon_, any ? icon::LIGHTBULB : icon::LIGHTBULB_OUTLINE);
  set_text_color(this->lights_icon_, any ? color::AMBER_INK : color::MUTED);
  set_text_color(this->lights_title_, any ? color::AMBER_INK : color::TEXT);
  set_text_color(this->lights_sub_, any ? color::AMBER_INK : color::MUTED);
  lv_obj_set_style_text_opa(this->lights_sub_, any ? LV_OPA_80 : LV_OPA_COVER, 0);

  std::string sub;
  if (!any) {
    sub = "All off";
  } else if (on > 2) {
    sub = std::to_string(on) + " rooms on";
  } else {
    for (const auto &l : model.lights) {
      if (!l.on)
        continue;
      if (!sub.empty())
        sub += ", ";
      sub += l.name;
    }
    sub += " on";
  }
  set_text(this->lights_sub_, sub);
}

void HomeScreen::update_climate_(const Model &model) {
  const auto &c = model.climate;

  std::string sub;
  uint32_t ic = color::MUTED;
  if (c.mode == "heat") {
    sub = "Heat to " + fmt_temp(c.setpoint);
    ic = color::HEAT;
  } else if (c.mode == "cool") {
    sub = "Cool to " + fmt_temp(c.setpoint);
    ic = color::COOL;
  } else if (c.mode == "heat_cool" || c.mode == "auto") {
    sub = "Auto · " + fmt_temp(c.setpoint);
    ic = color::AUTO;
  } else if (c.mode == "off") {
    sub = "Off";
  }

  if (!std::isnan(c.humidity)) {
    if (!sub.empty())
      sub += " · ";
    sub += fmt_pct(c.humidity) + " humidity";
  }
  if (sub.empty())
    sub = std::isnan(c.current) ? "--" : fmt_temp(c.current) + " inside";

  set_text(this->climate_sub_, sub);
  set_text_color(this->climate_icon_, ic);
}

void HomeScreen::update_status_(const Model &model) {
  const auto &s = model.status;

  // Meeting chip + next event
  hide(this->meeting_chip_, !s.meeting);
  bool has_next = !s.next_event.empty();
  if (has_next)
    set_text(this->next_event_, "Next: " + s.next_event);
  hide(this->next_event_, !has_next);

  // Printer (Bambu print_status)
  const std::string &ps = s.printer_status;
  bool active = ps == "running" || ps == "pause" || ps == "prepare";
  std::string state;
  if (ps == "running")
    state = "printing";
  else if (ps == "pause")
    state = "paused";
  else if (ps == "prepare")
    state = "preparing";
  else if (ps == "finish")
    state = "done";
  else if (ps == "failed")
    state = "failed";
  else if (ps == "offline" || ps == "unavailable")
    state = "offline";
  else
    state = "idle";
  set_text(this->printer_title_, "P2S " + state);

  bool show_pct = active && !std::isnan(s.printer_progress);
  set_text(this->printer_pct_, show_pct ? fmt_pct(s.printer_progress) : "");
  hide(this->printer_pct_, !show_pct);
  hide(this->printer_bar_, !active);
  set_bar(this->printer_bar_, active ? s.printer_progress : 0);
}

}  // namespace dashboard
}  // namespace esphome
