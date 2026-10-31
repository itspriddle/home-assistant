#include "theme.h"

#include <cmath>
#include <cstdio>

namespace esphome {
namespace dashboard {

Fonts fonts;

const char *icon::weather(const std::string &c) {
  if (c == "sunny" || c == "clear")
    return WEATHER_SUNNY;
  if (c == "clear-night")
    return WEATHER_NIGHT;
  if (c == "partlycloudy")
    return WEATHER_PARTLY_CLOUDY;
  if (c == "cloudy")
    return WEATHER_CLOUDY;
  if (c == "rainy")
    return WEATHER_RAINY;
  if (c == "pouring")
    return WEATHER_POURING;
  if (c == "snowy" || c == "snowy-rainy")
    return WEATHER_SNOWY;
  if (c == "fog" || c == "hazy")
    return WEATHER_FOG;
  if (c == "lightning")
    return WEATHER_LIGHTNING;
  if (c == "lightning-rainy")
    return WEATHER_LIGHTNING_RAINY;
  if (c == "windy" || c == "windy-variant")
    return WEATHER_WINDY;
  if (c == "hail")
    return WEATHER_HAIL;
  return WEATHER_PARTLY_CLOUDY;
}

lv_obj_t *box(lv_obj_t *parent, int64_t bg, int radius) {
  lv_obj_t *obj = lv_obj_create(parent);
  lv_obj_remove_style_all(obj);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  if (bg >= 0) {
    lv_obj_set_style_bg_color(obj, hex(static_cast<uint32_t>(bg)), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  }
  if (radius > 0)
    lv_obj_set_style_radius(obj, radius, 0);
  return obj;
}

void flex_row(lv_obj_t *obj, int gap, lv_flex_align_t main, lv_flex_align_t cross) {
  lv_obj_set_layout(obj, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(obj, main, cross, cross);
  lv_obj_set_style_pad_column(obj, gap, 0);
}

void flex_col(lv_obj_t *obj, int gap, lv_flex_align_t main, lv_flex_align_t cross) {
  lv_obj_set_layout(obj, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(obj, main, cross, cross);
  lv_obj_set_style_pad_row(obj, gap, 0);
}

lv_obj_t *label(lv_obj_t *parent, const lv_font_t *font, uint32_t color, const char *text) {
  lv_obj_t *obj = lv_label_create(parent);
  lv_obj_remove_style_all(obj);
  if (font != nullptr)
    lv_obj_set_style_text_font(obj, font, 0);
  lv_obj_set_style_text_color(obj, hex(color), 0);
  lv_label_set_text(obj, text);
  return obj;
}

void set_text(lv_obj_t *obj, const std::string &text) {
  const char *cur = lv_label_get_text(obj);
  if (cur != nullptr && text == cur)
    return;
  lv_label_set_text(obj, text.c_str());
}

lv_obj_t *dot(lv_obj_t *parent, int size, uint32_t color) {
  lv_obj_t *obj = box(parent, color, LV_RADIUS_CIRCLE);
  lv_obj_set_size(obj, size, size);
  return obj;
}

lv_obj_t *bar(lv_obj_t *parent, int width, int height, uint32_t track, uint32_t fill) {
  lv_obj_t *t = box(parent, track, height / 2);
  lv_obj_set_size(t, width, height);
  lv_obj_t *f = box(t, fill, height / 2);
  lv_obj_set_size(f, 0, height);
  return t;
}

void set_bar(lv_obj_t *bar, float pct) {
  lv_obj_t *fill = lv_obj_get_child(bar, 0);
  if (fill == nullptr)
    return;
  if (std::isnan(pct))
    pct = 0;
  pct = pct < 0 ? 0 : (pct > 100 ? 100 : pct);
  lv_obj_set_width(fill, lv_pct(static_cast<int>(pct)));
  lv_obj_set_style_opa(fill, pct > 0 ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
}

static void click_cb(lv_event_t *e) {
  auto *fn = static_cast<std::function<void()> *>(lv_event_get_user_data(e));
  if (fn != nullptr && *fn)
    (*fn)();
}

void on_click(lv_obj_t *obj, std::function<void()> fn) {
  lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  lv_obj_set_ext_click_area(obj, 8);
  // Pressed feedback: dim the whole object slightly
  lv_obj_set_style_opa(obj, LV_OPA_70, LV_STATE_PRESSED);
  // UI objects are never deleted, so this is intentionally not freed
  auto *heap_fn = new std::function<void()>(std::move(fn));
  lv_obj_add_event_cb(obj, click_cb, LV_EVENT_CLICKED, heap_fn);
}

void set_bg(lv_obj_t *obj, uint32_t color) {
  lv_obj_set_style_bg_color(obj, hex(color), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
}

void set_text_color(lv_obj_t *obj, uint32_t color) { lv_obj_set_style_text_color(obj, hex(color), 0); }

std::string fmt_temp(float t, int decimals) {
  if (std::isnan(t))
    return "--°";
  char buf[16];
  snprintf(buf, sizeof(buf), "%.*f°", decimals, t);
  return buf;
}

std::string fmt_pct(float p) {
  if (std::isnan(p))
    return "";
  char buf[8];
  snprintf(buf, sizeof(buf), "%.0f%%", p);
  return buf;
}

}  // namespace dashboard
}  // namespace esphome
