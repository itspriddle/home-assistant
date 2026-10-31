#include "climate_screen.h"

#include <cmath>
#include <algorithm>
#include <cstdio>
#include <string>

#include "dashboard.h"
#include "theme.h"

namespace esphome {
namespace dashboard {

// Layout (see Climate.dc.html): rail | 40px | dial column 560 | 32 | side column | 40px
static constexpr int PAD_X = 40;
static constexpr int PAD_Y = 36;
static constexpr int DIAL_COL_W = 560;
static constexpr int COL_GAP = 32;
static constexpr int CONTENT_H = SCREEN_H - 2 * PAD_Y;

// Dial: 270° sweep starting bottom-left (LVGL 0° = 3 o'clock, clockwise)
static constexpr int DIAL_RADIUS = 220;  // centerline of the stroke
static constexpr int DIAL_STROKE = 30;
static constexpr int KNOB_D = 44;
static constexpr int DIAL_MIN = 60;
static constexpr int DIAL_MAX = 80;
// The +/- buttons may go a little beyond the dial range (the knob pins at the end)
static constexpr float SETPOINT_MIN = 50.0f;
static constexpr float SETPOINT_MAX = 90.0f;
static constexpr float SETPOINT_STEP = 1.0f;  // HA thermostat is °F

static constexpr uint32_t DIAL_TRACK = 0x262626;
static constexpr uint32_t ACCENT_OFF = 0x4a4a4a;
static constexpr uint32_t ROW_LINE = 0x2a2a2a;
static constexpr int ROW_H = 76;

enum class Mode { OFF, HEAT, COOL, AUTO, UNKNOWN };

static Mode parse_mode(const std::string &m) {
  if (m == "off")
    return Mode::OFF;
  if (m == "heat")
    return Mode::HEAT;
  if (m == "cool")
    return Mode::COOL;
  if (m == "heat_cool" || m == "auto")
    return Mode::AUTO;
  return Mode::UNKNOWN;
}

static uint32_t accent_for(Mode m) {
  switch (m) {
    case Mode::HEAT:
      return color::HEAT;
    case Mode::COOL:
      return color::COOL;
    case Mode::AUTO:
      return color::AUTO;
    default:
      return ACCENT_OFF;
  }
}

static std::string action_text(const Climate &c, Mode mode) {
  const std::string &a = c.action;
  if (a == "heating")
    return "HEATING";
  if (a == "cooling")
    return "COOLING";
  if (a == "idle")
    return "IDLE";
  if (a == "off" || (a.empty() && mode == Mode::OFF))
    return "OFF";
  std::string out;
  for (char ch : a)
    out += static_cast<char>(ch >= 'a' && ch <= 'z' ? ch - 'a' + 'A' : ch == '_' ? ' ' : ch);
  return out;
}

// "70°", "70.5°", "--°"
static std::string setpoint_text(float t) {
  if (std::isnan(t))
    return "--°";
  bool whole = std::fabs(t - std::round(t)) < 0.05f;
  return fmt_temp(t, whole ? 0 : 1);
}

static void strip_part(lv_obj_t *obj, lv_part_t part) {
  lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, part);
  lv_obj_set_style_border_width(obj, 0, part);
  lv_obj_set_style_pad_all(obj, 0, part);
}

static void arc_value_cb(lv_event_t *e) {
  auto *dash = static_cast<Dashboard *>(lv_event_get_user_data(e));
  lv_obj_t *arc = lv_event_get_target_obj(e);
  dash->set_setpoint(static_cast<float>(lv_arc_get_value(arc)));
}

void ClimateScreen::build(lv_obj_t *root, Dashboard *dash) {
  this->dash_ = dash;
  this->rail_ = build_nav_rail(root, dash, ScreenId::CLIMATE);

  lv_obj_t *dial_col = box(root);
  lv_obj_set_size(dial_col, DIAL_COL_W, CONTENT_H);
  lv_obj_set_pos(dial_col, RAIL_W + PAD_X, PAD_Y);
  this->build_dial_(dial_col);

  lv_obj_t *side = box(root);
  int side_x = RAIL_W + PAD_X + DIAL_COL_W + COL_GAP;
  lv_obj_set_size(side, SCREEN_W - PAD_X - side_x, CONTENT_H);
  lv_obj_set_pos(side, side_x, PAD_Y);
  this->build_side_(side);
}

void ClimateScreen::build_dial_(lv_obj_t *parent) {
  // ---- arc ----
  lv_obj_t *arc = lv_arc_create(parent);
  this->arc_ = arc;
  lv_obj_remove_style_all(arc);
  int arc_size = 2 * DIAL_RADIUS + DIAL_STROKE;  // outer edge of the stroke
  lv_obj_set_size(arc, arc_size, arc_size);
  lv_obj_align(arc, LV_ALIGN_CENTER, 0, 0);
  lv_arc_set_rotation(arc, 135);
  lv_arc_set_bg_angles(arc, 0, 270);
  lv_arc_set_range(arc, DIAL_MIN, DIAL_MAX);
  lv_arc_set_value(arc, DIAL_MIN);

  lv_obj_set_style_arc_width(arc, DIAL_STROKE, LV_PART_MAIN);
  lv_obj_set_style_arc_color(arc, hex(DIAL_TRACK), LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(arc, true, LV_PART_MAIN);
  lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_MAIN);
  strip_part(arc, LV_PART_MAIN);

  lv_obj_set_style_arc_width(arc, DIAL_STROKE, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(arc, hex(color::HEAT), LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
  lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_INDICATOR);
  strip_part(arc, LV_PART_INDICATOR);

  // Knob is centered on the stroke; pad grows it past the stroke width
  lv_obj_set_style_bg_color(arc, hex(color::TEXT), LV_PART_KNOB);
  lv_obj_set_style_bg_opa(arc, LV_OPA_COVER, LV_PART_KNOB);
  lv_obj_set_style_radius(arc, LV_RADIUS_CIRCLE, LV_PART_KNOB);
  lv_obj_set_style_pad_all(arc, (KNOB_D - DIAL_STROKE) / 2, LV_PART_KNOB);

  // Draggable along the ring only (ADV_HITTEST ignores presses inside it)
  lv_obj_add_flag(arc, LV_OBJ_FLAG_ADV_HITTEST);
  lv_obj_clear_flag(arc, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  lv_obj_set_ext_click_area(arc, 12);
  lv_obj_add_event_cb(arc, arc_value_cb, LV_EVENT_VALUE_CHANGED, this->dash_);

  // ---- center readout ----
  lv_obj_t *center = box(parent);
  lv_obj_set_size(center, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_align(center, LV_ALIGN_TOP_MID, 0, 150);
  flex_col(center, 4, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(center, LV_OBJ_FLAG_CLICKABLE);

  this->action_ = label(center, fonts.small, ACCENT_OFF, "");
  lv_obj_set_style_text_letter_space(this->action_, 3, 0);

  this->setpoint_ = label(center, fonts.display, color::TEXT, "--°");
  lv_obj_set_style_text_letter_space(this->setpoint_, -6, 0);

  this->inside_ = label(center, fonts.body, color::MUTED, "");

  // ---- -/+ buttons ----
  lv_obj_t *btns = box(parent);
  lv_obj_set_size(btns, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_align(btns, LV_ALIGN_BOTTOM_MID, 0, -36);
  flex_row(btns, 200);
  lv_obj_clear_flag(btns, LV_OBJ_FLAG_CLICKABLE);

  auto make_btn = [this, btns](const char *glyph, float delta) {
    lv_obj_t *b = box(btns, color::SURFACE_2, LV_RADIUS_CIRCLE);
    lv_obj_set_size(b, 88, 88);
    lv_obj_t *ic = label(b, fonts.icon_small, color::TEXT, glyph);
    lv_obj_center(ic);
    on_click(b, [this, delta]() { this->step_setpoint_(delta); });
    return b;
  };
  this->minus_ = make_btn(icon::MINUS, -SETPOINT_STEP);
  this->plus_ = make_btn(icon::PLUS, SETPOINT_STEP);
}

void ClimateScreen::build_side_(lv_obj_t *parent) {
  flex_col(parent, 22, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

  lv_obj_t *title = label(parent, fonts.title, color::TEXT, "Climate");
  lv_obj_set_style_text_letter_space(title, -1, 0);

  // ---- mode segmented control ----
  lv_obj_t *seg = box(parent, color::SURFACE, 24);
  lv_obj_set_size(seg, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(seg, 8, 0);
  flex_row(seg, 8);

  static const struct {
    const char *name;
    const char *mode;
  } MODES[] = {{"Off", "off"}, {"Heat", "heat"}, {"Cool", "cool"}, {"Auto", "heat_cool"}};

  for (size_t i = 0; i < this->modes_.size(); i++) {
    lv_obj_t *b = box(seg, -1, 18);
    lv_obj_set_height(b, 64);
    lv_obj_set_flex_grow(b, 1);
    lv_obj_t *t = label(b, fonts.small, color::MUTED, MODES[i].name);
    lv_obj_center(t);
    const char *mode = MODES[i].mode;
    Dashboard *dash = this->dash_;
    on_click(b, [dash, mode]() { dash->set_mode(mode); });
    this->modes_[i] = {mode, b, t};
  }

  // ---- room list ----
  lv_obj_t *card = box(parent, color::SURFACE, 24);
  lv_obj_set_size(card, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_hor(card, 24, 0);
  lv_obj_set_style_pad_ver(card, 4, 0);
  flex_col(card, 0, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

  const auto &rooms = this->dash_->model().rooms;
  this->rows_.clear();
  for (size_t i = 0; i < rooms.size(); i++) {
    lv_obj_t *row = box(card);
    lv_obj_set_size(row, lv_pct(100), ROW_H);
    flex_row(row, 14);
    if (i > 0) {
      lv_obj_set_style_border_side(row, LV_BORDER_SIDE_TOP, 0);
      lv_obj_set_style_border_width(row, 1, 0);
      lv_obj_set_style_border_color(row, hex(ROW_LINE), 0);
      lv_obj_set_style_border_opa(row, LV_OPA_COVER, 0);
    }
    RoomRow r{};
    r.dot = dot(row, 10, color::OFF_DOT);
    r.name = label(row, fonts.body, color::TEXT, rooms[i].name);
    lv_obj_set_flex_grow(r.name, 1);
    r.note = label(row, fonts.caption, color::MUTED, "");
    r.temp = label(row, fonts.label, color::TEXT, "--°");
    lv_obj_set_width(r.temp, 92);
    lv_obj_set_style_text_align(r.temp, LV_TEXT_ALIGN_RIGHT, 0);
    this->rows_.push_back(r);
  }

  label(parent, fonts.caption, color::DIM, "Green dot = someone's there (thermostat room sensors)");
}

void ClimateScreen::update(const Model &model, uint32_t dirty) {
  if (dirty & DIRTY_CLOCK)
    update_nav_rail(this->rail_, this->dash_);
  if (dirty & DIRTY_CLIMATE)
    this->update_climate_(model.climate);
  if (dirty & DIRTY_ROOMS)
    this->update_rooms_(model);
}

void ClimateScreen::update_climate_(const Climate &c) {
  Mode mode = parse_mode(c.mode);
  bool off = mode == Mode::OFF;
  uint32_t accent = accent_for(mode);
  bool has_sp = !std::isnan(c.setpoint);

  // Dial. Leave it alone mid-drag so the model echo can't fight the finger.
  if (!lv_obj_has_state(this->arc_, LV_STATE_PRESSED)) {
    int v = has_sp ? static_cast<int>(std::lround(c.setpoint)) : DIAL_MIN;
    lv_arc_set_value(this->arc_, v < DIAL_MIN ? DIAL_MIN : (v > DIAL_MAX ? DIAL_MAX : v));
  }
  lv_obj_set_style_arc_color(this->arc_, hex(accent), LV_PART_INDICATOR);
  lv_obj_set_style_arc_opa(this->arc_, off || !has_sp ? LV_OPA_TRANSP : LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(this->arc_, has_sp && !off ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_KNOB);
  if (off || !has_sp)
    lv_obj_clear_flag(this->arc_, LV_OBJ_FLAG_CLICKABLE);
  else
    lv_obj_add_flag(this->arc_, LV_OBJ_FLAG_CLICKABLE);

  // Center readout
  set_text(this->action_, action_text(c, mode));
  set_text_color(this->action_, accent);
  set_text(this->setpoint_, off ? "Off" : setpoint_text(c.setpoint));

  std::string inside = "Inside " + fmt_temp(c.current);
  std::string hum = fmt_pct(c.humidity);
  if (!hum.empty())
    inside += " · " + hum + " humidity";
  set_text(this->inside_, inside);

  // -/+ are inert while off or before HA reports a setpoint
  lv_opa_t btn_opa = off || !has_sp ? LV_OPA_40 : LV_OPA_COVER;
  lv_obj_set_style_opa(this->minus_, btn_opa, 0);
  lv_obj_set_style_opa(this->plus_, btn_opa, 0);

  // Mode segments
  for (auto &m : this->modes_) {
    Mode seg_mode = parse_mode(m.mode);
    bool selected = seg_mode == mode;
    if (selected) {
      set_bg(m.btn, color::SURFACE_3);
      set_text_color(m.text, off ? color::TEXT : accent);
    } else {
      lv_obj_set_style_bg_opa(m.btn, LV_OPA_TRANSP, 0);
      set_text_color(m.text, color::MUTED);
    }
  }
}

void ClimateScreen::update_rooms_(const Model &model) {
  size_t n = std::min(this->rows_.size(), model.rooms.size());
  for (size_t i = 0; i < n; i++) {
    const RoomClimate &room = model.rooms[i];
    RoomRow &row = this->rows_[i];
    set_bg(row.dot, room.has_occupancy && room.occupied ? color::OCCUPIED : color::OFF_DOT);
    set_text(row.note, fmt_pct(room.humidity));
    set_text(row.temp, fmt_temp(room.temperature, 1));
  }
}

void ClimateScreen::step_setpoint_(float delta) {
  const Climate &c = this->dash_->model().climate;
  if (std::isnan(c.setpoint) || parse_mode(c.mode) == Mode::OFF)
    return;
  float t = std::round(c.setpoint / SETPOINT_STEP) * SETPOINT_STEP + delta;
  t = t < SETPOINT_MIN ? SETPOINT_MIN : (t > SETPOINT_MAX ? SETPOINT_MAX : t);
  this->dash_->set_setpoint(t);
}

}  // namespace dashboard
}  // namespace esphome
