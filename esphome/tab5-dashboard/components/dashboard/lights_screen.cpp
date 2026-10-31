#include "lights_screen.h"

#include <cstring>
#include <string>

#include "dashboard.h"
#include "theme.h"

namespace esphome {
namespace dashboard {

// Layout (from the Lights mock)
static constexpr int PAD_V = 36;
static constexpr int PAD_H = 40;
static constexpr int CONTENT_W = SCREEN_W - RAIL_W - 2 * PAD_H;  // 1088
static constexpr int SECTION_GAP = 22;

static constexpr int TILE_H = 176;
static constexpr int TILE_GAP = 14;
static constexpr int TILE_RADIUS = 26;
static constexpr int TILE_PAD = 20;
static constexpr int POWER_SIZE = 56;
static constexpr int POWER_INSET = 12;

static constexpr int SCENE_COLS = 4;
static constexpr int SCENE_GAP = 12;
static constexpr int SCENE_W = (CONTENT_W - (SCENE_COLS - 1) * SCENE_GAP) / SCENE_COLS;  // 263
static constexpr int SCENE_H = 84;

static constexpr int ROUND_BTN = 56;
static constexpr int BRIGHTNESS_STEP = 10;

static constexpr const char *DEFAULT_ROOM = "office";

// An object that should never steal taps from its clickable parent
static void passthrough(lv_obj_t *obj) { lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE); }

static lv_obj_t *round_button(lv_obj_t *parent, const char *glyph) {
  lv_obj_t *btn = box(parent, color::SURFACE_3, ROUND_BTN / 2);
  lv_obj_set_size(btn, ROUND_BTN, ROUND_BTN);
  lv_obj_t *ic = label(btn, fonts.icon_small, color::TEXT, glyph);
  lv_obj_center(ic);
  return btn;
}

static std::string pct_text(int pct) { return std::to_string(pct) + "%"; }

void LightsScreen::build(lv_obj_t *root, Dashboard *dash) {
  this->dash_ = dash;
  this->rail_ = build_nav_rail(root, dash, ScreenId::LIGHTS);

  const auto &lights = dash->model().lights;
  this->room_count_ = lights.size() < MAX_ROOMS ? lights.size() : MAX_ROOMS;
  this->selected_ = 0;
  for (size_t i = 0; i < this->room_count_; i++) {
    if (std::strcmp(lights[i].key, DEFAULT_ROOM) == 0)
      this->selected_ = i;
  }

  lv_obj_t *content = box(root);
  lv_obj_set_size(content, SCREEN_W - RAIL_W, SCREEN_H);
  lv_obj_align(content, LV_ALIGN_TOP_LEFT, RAIL_W, 0);
  lv_obj_set_style_pad_ver(content, PAD_V, 0);
  lv_obj_set_style_pad_hor(content, PAD_H, 0);
  flex_col(content, SECTION_GAP);

  this->build_header_(content);
  this->build_tiles_(content);
  this->build_scenes_(content);

  // Pushes the brightness row to the bottom
  lv_obj_t *spacer = box(content);
  lv_obj_set_width(spacer, 1);
  lv_obj_set_flex_grow(spacer, 1);

  this->build_brightness_(content);
}

void LightsScreen::build_header_(lv_obj_t *parent) {
  lv_obj_t *header = box(parent);
  lv_obj_set_size(header, lv_pct(100), LV_SIZE_CONTENT);
  flex_row(header, 0, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER);

  lv_obj_t *title = label(header, fonts.title, color::TEXT, "Lights");
  lv_obj_set_style_text_letter_space(title, -1, 0);

  lv_obj_t *all_off = box(header, color::SURFACE_2, ROUND_BTN / 2);
  lv_obj_set_size(all_off, LV_SIZE_CONTENT, ROUND_BTN);
  lv_obj_set_style_pad_hor(all_off, 28, 0);
  lv_obj_t *txt = label(all_off, fonts.small, color::TEXT, "All off");
  lv_obj_center(txt);
  on_click(all_off, [this]() { this->dash_->lights_off(); });
}

void LightsScreen::build_tiles_(lv_obj_t *parent) {
  lv_obj_t *row = box(parent);
  lv_obj_set_size(row, lv_pct(100), TILE_H);
  // The selection ring is drawn outside the tile
  lv_obj_add_flag(row, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  flex_row(row, TILE_GAP, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

  const auto &lights = this->dash_->model().lights;
  for (size_t i = 0; i < this->room_count_; i++) {
    Tile &t = this->tiles_[i];
    t.obj = box(row, color::SURFACE, TILE_RADIUS);
    lv_obj_set_height(t.obj, TILE_H);
    lv_obj_set_flex_grow(t.obj, 1);
    lv_obj_set_style_outline_color(t.obj, hex(color::TEXT), 0);
    lv_obj_set_style_outline_width(t.obj, 3, 0);
    lv_obj_set_style_outline_pad(t.obj, 3, 0);
    lv_obj_set_style_outline_opa(t.obj, LV_OPA_TRANSP, 0);

    lv_obj_t *text = box(t.obj);
    passthrough(text);
    lv_obj_set_size(text, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(text, TILE_PAD, 0);
    lv_obj_align(text, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    flex_col(text, 4);

    t.name = label(text, fonts.label, color::TEXT_2, lights[i].name);
    lv_obj_set_width(t.name, lv_pct(100));
    lv_label_set_long_mode(t.name, LV_LABEL_LONG_DOT);
    t.status = label(text, fonts.caption, color::TEXT_2, "Off");
    lv_obj_set_width(t.status, lv_pct(100));
    lv_label_set_long_mode(t.status, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_opa(t.status, 191, 0);  // 75%

    t.power = box(t.obj, color::SURFACE_3, POWER_SIZE / 2);
    lv_obj_set_size(t.power, POWER_SIZE, POWER_SIZE);
    lv_obj_align(t.power, LV_ALIGN_TOP_RIGHT, -POWER_INSET, POWER_INSET);
    t.power_icon = label(t.power, fonts.icon_small, color::TEXT_2, icon::POWER);
    lv_obj_center(t.power_icon);

    std::string key = lights[i].key;
    on_click(t.power, [this, key]() { this->dash_->toggle_light(key); });
    on_click(t.obj, [this, i]() { this->select_(i); });
  }
}

void LightsScreen::build_scenes_(lv_obj_t *parent) {
  lv_obj_t *section = box(parent);
  lv_obj_set_size(section, lv_pct(100), LV_SIZE_CONTENT);
  flex_col(section, SCENE_GAP);

  this->scenes_title_ = label(section, fonts.small, color::MUTED, "");

  lv_obj_t *grid = box(section);
  lv_obj_set_size(grid, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_layout(grid, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_column(grid, SCENE_GAP, 0);
  lv_obj_set_style_pad_row(grid, SCENE_GAP, 0);

  for (size_t i = 0; i < MAX_SCENES; i++) {
    SceneButton &b = this->scenes_[i];
    b.obj = box(grid, color::SURFACE, 20);
    lv_obj_set_size(b.obj, SCENE_W, SCENE_H);
    lv_obj_set_style_pad_hor(b.obj, 22, 0);
    flex_row(b.obj, 14);

    b.dot = dot(b.obj, 14, color::OFF_DOT);
    passthrough(b.dot);
    b.name = label(b.obj, fonts.body, color::TEXT_2, "");
    lv_obj_set_flex_grow(b.name, 1);
    lv_label_set_long_mode(b.name, LV_LABEL_LONG_DOT);

    int idx = static_cast<int>(i);
    on_click(b.obj, [this, idx]() {
      const auto &lights = this->dash_->model().lights;
      if (this->selected_ < lights.size())
        this->dash_->activate_scene(lights[this->selected_].key, idx);
    });
  }
}

void LightsScreen::build_brightness_(lv_obj_t *parent) {
  lv_obj_t *row = box(parent, color::SURFACE, 22);
  lv_obj_set_size(row, lv_pct(100), 84);
  lv_obj_set_style_pad_left(row, 24, 0);
  lv_obj_set_style_pad_right(row, 16, 0);
  flex_row(row, 20);

  lv_obj_t *title = label(row, fonts.small, color::MUTED, "Brightness");
  lv_obj_set_width(title, 120);

  lv_obj_t *minus = round_button(row, icon::MINUS);
  on_click(minus, [this]() { this->step_brightness_(-BRIGHTNESS_STEP); });

  this->bri_bar_ = bar(row, 100, 16, color::LINE, color::AMBER);
  lv_obj_set_flex_grow(this->bri_bar_, 1);

  lv_obj_t *plus = round_button(row, icon::PLUS);
  on_click(plus, [this]() { this->step_brightness_(BRIGHTNESS_STEP); });

  this->bri_label_ = label(row, fonts.label, color::TEXT, "Off");
  lv_obj_set_width(this->bri_label_, 80);
  lv_obj_set_style_text_align(this->bri_label_, LV_TEXT_ALIGN_RIGHT, 0);
}

void LightsScreen::select_(size_t index) {
  if (index >= this->room_count_ || index == this->selected_)
    return;
  this->selected_ = index;
  this->redraw_(this->dash_->model());
}

void LightsScreen::step_brightness_(int delta) {
  const auto &lights = this->dash_->model().lights;
  if (this->selected_ >= lights.size())
    return;
  const LightRoom &l = lights[this->selected_];
  int current = l.on ? l.brightness_pct : 0;
  int next = current + delta;
  next = next < 0 ? 0 : (next > 100 ? 100 : next);
  if (next == current)
    return;
  this->dash_->set_light_level(l.key, next);
}

void LightsScreen::update(const Model &model, uint32_t dirty) {
  if (dirty & DIRTY_CLOCK)
    update_nav_rail(this->rail_, this->dash_);
  if (dirty & DIRTY_LIGHTS)
    this->redraw_(model);
}

void LightsScreen::redraw_(const Model &model) {
  // Room tiles
  for (size_t i = 0; i < this->room_count_ && i < model.lights.size(); i++) {
    const LightRoom &l = model.lights[i];
    Tile &t = this->tiles_[i];
    bool has_scene = l.active_scene >= 0 && l.active_scene < static_cast<int>(l.scenes.size());

    std::string status = "Off";
    if (l.on)
      status = std::string(has_scene ? l.scenes[l.active_scene].name : "On") + " · " + pct_text(l.brightness_pct);

    set_bg(t.obj, l.on ? color::AMBER : color::SURFACE);
    uint32_t ink = l.on ? color::AMBER_INK : color::TEXT_2;
    set_text_color(t.name, ink);
    set_text_color(t.status, ink);
    set_text(t.status, status);

    // Power button: translucent ink on amber, solid on dark
    lv_obj_set_style_bg_color(t.power, hex(l.on ? color::AMBER_INK : color::SURFACE_3), 0);
    lv_obj_set_style_bg_opa(t.power, l.on ? 31 : LV_OPA_COVER, 0);  // 12% on amber
    set_text_color(t.power_icon, ink);

    lv_obj_set_style_outline_opa(t.obj, i == this->selected_ ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
  }

  if (this->selected_ >= model.lights.size())
    return;
  const LightRoom &sel = model.lights[this->selected_];

  // Scenes for the selected room
  set_text(this->scenes_title_, std::string(sel.name) + " scenes");
  for (size_t i = 0; i < MAX_SCENES; i++) {
    SceneButton &b = this->scenes_[i];
    if (i >= sel.scenes.size()) {
      lv_obj_add_flag(b.obj, LV_OBJ_FLAG_HIDDEN);
      continue;
    }
    const SceneDef &s = sel.scenes[i];
    bool active = sel.on && sel.active_scene == static_cast<int>(i);
    lv_obj_clear_flag(b.obj, LV_OBJ_FLAG_HIDDEN);
    set_text(b.name, s.name);
    set_bg(b.dot, s.dot_color);
    set_bg(b.obj, active ? color::AMBER_TINT : color::SURFACE);
    set_text_color(b.name, active ? color::AMBER : color::TEXT_2);
  }

  // Brightness
  set_bar(this->bri_bar_, sel.on ? sel.brightness_pct : 0);
  set_text(this->bri_label_, sel.on ? pct_text(sel.brightness_pct) : "Off");
}

}  // namespace dashboard
}  // namespace esphome
