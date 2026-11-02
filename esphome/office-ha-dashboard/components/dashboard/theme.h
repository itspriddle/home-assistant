#pragma once

#include <functional>
#include <string>

#include "lvgl.h"

namespace esphome {
namespace dashboard {

// Screen is 1280x720 landscape (display rotated 270°, USB port on the left)
static constexpr int SCREEN_W = 1280;
static constexpr int SCREEN_H = 720;
static constexpr int RAIL_W = 112;  // left nav rail on Lights / Climate

// Palette (from the design canvas, "Round 2")
namespace color {
static constexpr uint32_t BG = 0x121212;
static constexpr uint32_t RAIL = 0x171717;
static constexpr uint32_t SURFACE = 0x1c1c1c;  // cards / tiles
static constexpr uint32_t SURFACE_2 = 0x1f1f1f;  // buttons on BG
static constexpr uint32_t SURFACE_3 = 0x2a2a2a;  // buttons on SURFACE
static constexpr uint32_t LINE = 0x2c2c2c;  // dividers, empty bar tracks
static constexpr uint32_t TEXT = 0xededed;
static constexpr uint32_t TEXT_2 = 0xcfcfcf;
static constexpr uint32_t MUTED = 0x9a9a9a;
static constexpr uint32_t DIM = 0x7a7a7a;
static constexpr uint32_t OFF_DOT = 0x3a3a3a;

static constexpr uint32_t AMBER = 0xf5c26b;  // lights on
static constexpr uint32_t AMBER_INK = 0x1a1405;  // text on AMBER
static constexpr uint32_t AMBER_TINT = 0x2a2418;  // selected / active on dark

static constexpr uint32_t HEAT = 0xe8896b;
static constexpr uint32_t HEAT_TINT = 0x33201a;
static constexpr uint32_t COOL = 0x7fb2f0;
static constexpr uint32_t AUTO = 0xb9a6f0;
static constexpr uint32_t OCCUPIED = 0x6fcf8f;
static constexpr uint32_t PRINTER = 0x9fb7d9;

static constexpr uint32_t MEETING_BG = 0x3a1f1a;
static constexpr uint32_t MEETING_FG = 0xf0a089;
static constexpr uint32_t MEETING_DOT = 0xf0806a;
}  // namespace color

// Material Design Icons codepoints (UTF-8). Every glyph used must also be
// listed in the icon fonts' `glyphs:` in tab5-dashboard.yaml.
namespace icon {
static constexpr const char *HOME = "\U000F02DC";
static constexpr const char *LIGHTBULB = "\U000F0335";
static constexpr const char *LIGHTBULB_OUTLINE = "\U000F0336";
static constexpr const char *THERMOMETER = "\U000F050F";
static constexpr const char *POWER = "\U000F0425";
static constexpr const char *PRINTER_3D = "\U000F042B";
static constexpr const char *CALENDAR = "\U000F00ED";
static constexpr const char *WATER_PERCENT = "\U000F058E";
static constexpr const char *MINUS = "\U000F0374";
static constexpr const char *PLUS = "\U000F0415";
static constexpr const char *FIRE = "\U000F0238";
static constexpr const char *SNOWFLAKE = "\U000F0717";
static constexpr const char *WIFI_OFF = "\U000F05AA";
static constexpr const char *WEATHER_SUNNY = "\U000F0599";
static constexpr const char *WEATHER_NIGHT = "\U000F0594";
static constexpr const char *WEATHER_PARTLY_CLOUDY = "\U000F0595";
static constexpr const char *WEATHER_CLOUDY = "\U000F0590";
static constexpr const char *WEATHER_RAINY = "\U000F0597";
static constexpr const char *WEATHER_POURING = "\U000F0596";
static constexpr const char *WEATHER_SNOWY = "\U000F0598";
static constexpr const char *WEATHER_FOG = "\U000F0591";
static constexpr const char *WEATHER_LIGHTNING = "\U000F0593";
static constexpr const char *WEATHER_LIGHTNING_RAINY = "\U000F067E";
static constexpr const char *WEATHER_WINDY = "\U000F059D";
static constexpr const char *WEATHER_HAIL = "\U000F0592";

// HA weather condition -> icon
const char *weather(const std::string &condition);
}  // namespace icon

// Fonts, resolved from the YAML `fonts:` block at setup
struct Fonts {
  const lv_font_t *caption{nullptr};  // 18 regular
  const lv_font_t *small{nullptr};  // 20 regular
  const lv_font_t *body{nullptr};  // 22 regular
  const lv_font_t *label{nullptr};  // 26 semibold
  const lv_font_t *value{nullptr};  // 34 medium
  const lv_font_t *title{nullptr};  // 40 semibold
  const lv_font_t *large{nullptr};  // 96 regular   (digits, ° - . %)
  const lv_font_t *display{nullptr};  // 140 regular  (digits, ° - O f)
  const lv_font_t *clock{nullptr};  // 168 medium   (digits, :)
  const lv_font_t *clock_xl{nullptr};  // 260 medium   (digits, :), clock screen
  const lv_font_t *icon_small{nullptr};  // 34 MDI
  const lv_font_t *icon_medium{nullptr};  // 44 MDI
  const lv_font_t *icon_large{nullptr};  // 88 MDI
};

extern Fonts fonts;

// ------------------------------------------------------------------
// Widget helpers. Every helper strips LVGL's default theme styles so
// what you set is what you get: no borders, shadows, padding, or
// scrolling unless asked for.
// ------------------------------------------------------------------

inline lv_color_t hex(uint32_t c) { return lv_color_hex(c); }

// Plain container. bg < 0 means transparent.
lv_obj_t *box(lv_obj_t *parent, int64_t bg = -1, int radius = 0);

// Flex helpers: set layout on an existing object
void flex_row(lv_obj_t *obj, int gap, lv_flex_align_t main = LV_FLEX_ALIGN_START,
              lv_flex_align_t cross = LV_FLEX_ALIGN_CENTER);
void flex_col(lv_obj_t *obj, int gap, lv_flex_align_t main = LV_FLEX_ALIGN_START,
              lv_flex_align_t cross = LV_FLEX_ALIGN_START);

lv_obj_t *label(lv_obj_t *parent, const lv_font_t *font, uint32_t color, const char *text = "");
void set_text(lv_obj_t *label, const std::string &text);  // no-op if unchanged

// Small filled circle (status / occupancy / scene dots)
lv_obj_t *dot(lv_obj_t *parent, int size, uint32_t color);

// Horizontal progress bar: track + fill. Returns the track; use set_bar().
lv_obj_t *bar(lv_obj_t *parent, int width, int height, uint32_t track, uint32_t fill);
void set_bar(lv_obj_t *bar, float pct);  // 0-100

// Make any object tappable. Adds a darkened pressed state and an
// expanded hit area. `fn` lives for the life of the object.
void on_click(lv_obj_t *obj, std::function<void()> fn);

void set_bg(lv_obj_t *obj, uint32_t color);
void set_text_color(lv_obj_t *obj, uint32_t color);

std::string fmt_temp(float t, int decimals = 0);  // "72°" / "72.5°" / "--°"
std::string fmt_pct(float p);  // "43%" / ""
std::string fmt_duration_h(float hours);  // "51m" / "1h 23m" / ""

}  // namespace dashboard
}  // namespace esphome
