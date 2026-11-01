#include "emby_ui.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace emby_ui {

static const char *const TAG = "emby_ui";

// Material Design Icons (UTF-8)
static const char *const ICON_NOW_PLAYING = "\xF3\xB0\xB4\x99";  // play-box-multiple
static const char *const ICON_MOVIE = "\xF3\xB0\xBF\x8E";        // movie-open
static const char *const ICON_EPISODE = "\xF3\xB0\x9F\xB4";      // television-classic
static const char *const ICON_AUDIO = "\xF3\xB0\x9D\x9A";        // music
static const char *const ICON_OTHER = "\xF3\xB0\x88\xB0";        // filmstrip
static const char *const ICON_PLAY = "\xF3\xB0\x90\x8A";         // play
static const char *const ICON_PAUSE = "\xF3\xB0\x8F\xA4";        // pause
static const char *const ICON_SLEEP = "\xF3\xB0\x92\xB2";        // sleep

static const char *const SEP = " \xC2\xB7 ";  // middle dot

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string str_or_empty(JsonVariantConst v) { return v.is<const char *>() ? v.as<const char *>() : ""; }

static void append_part(std::string &out, const std::string &part) {
  if (part.empty())
    return;
  if (!out.empty())
    out += SEP;
  out += part;
}

static std::string format_duration(int32_t seconds) {
  if (seconds < 0)
    seconds = 0;
  char buf[16];
  int h = seconds / 3600, m = (seconds % 3600) / 60, s = seconds % 60;
  if (h > 0) {
    snprintf(buf, sizeof(buf), "%d:%02d:%02d", h, m, s);
  } else {
    snprintf(buf, sizeof(buf), "%02d:%02d", m, s);
  }
  return buf;
}

static const char *type_icon(const std::string &type) {
  if (type == "movie")
    return ICON_MOVIE;
  if (type == "episode")
    return ICON_EPISODE;
  if (type == "audio")
    return ICON_AUDIO;
  return ICON_OTHER;
}

static lv_obj_t *create_box(lv_obj_t *parent, lv_flex_flow_t flow) {
  lv_obj_t *box = lv_obj_create(parent);
  lv_obj_remove_style_all(box);
  lv_obj_set_size(box, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_layout(box, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(box, flow);
  lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);
  return box;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void EmbyUi::setup() {
  this->lv_font_title_ = this->font_title_->get_lv_font();
  this->lv_font_body_ = this->font_body_->get_lv_font();
  this->lv_font_small_ = this->font_small_->get_lv_font();
  this->lv_font_icon_ = this->font_icon_->get_lv_font();

  this->init_styles_();

  // Own the screen outright, regardless of what ESPHome's LVGL component created.
  lv_obj_t *scr = lv_obj_create(nullptr);
  lv_screen_load(scr);
  lv_obj_set_style_bg_color(scr, lv_color_hex(colors::SLATE_950), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(scr, 8, 0);
  lv_obj_set_style_pad_row(scr, 6, 0);
  lv_obj_set_layout(scr, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
  lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  this->create_header_(scr);

  this->list_ = create_box(scr, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_grow(this->list_, 1);
  lv_obj_set_style_pad_row(this->list_, 6, 0);

  this->render_();
}

void EmbyUi::loop() {
  uint32_t now = millis();
  if (now - this->last_tick_ < 1000)
    return;
  this->last_tick_ = now;

  this->tick_();

  auto t = this->time_->now();
  if (!t.is_valid() || t.minute == this->last_minute_)
    return;
  this->last_minute_ = t.minute;
  int hour = t.hour % 12 == 0 ? 12 : t.hour % 12;
  lv_label_set_text_fmt(this->clock_label_, "%d:%02d %s", hour, t.minute, t.hour < 12 ? "AM" : "PM");
}

void EmbyUi::dump_config() {
  ESP_LOGCONFIG(TAG, "Emby UI:");
  ESP_LOGCONFIG(TAG, "  Sessions: %u", (unsigned) this->sessions_.size());
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void EmbyUi::set_sessions(JsonArrayConst sessions) {
  this->sessions_.clear();
  for (JsonObjectConst s : sessions) {
    Session session;
    session.type = str_or_empty(s["type"]);
    session.playing = str_or_empty(s["state"]) != "paused";
    session.duration = s["duration"] | 0;
    session.position = s["position"] | 0;
    session.updated = s["updated"] | (int64_t) 0;

    std::string title = str_or_empty(s["title"]);
    if (session.type == "episode") {
      session.title = str_or_empty(s["series"]);
      std::string code;
      if (!s["season"].isNull() && !s["episode"].isNull()) {
        char buf[16];
        snprintf(buf, sizeof(buf), "S%02dE%02d", s["season"].as<int>(), s["episode"].as<int>());
        code = buf;
      }
      append_part(session.subtitle, code);
      append_part(session.subtitle, title);
    } else if (session.type == "audio") {
      session.title = title;
      append_part(session.subtitle, str_or_empty(s["artist"]));
      append_part(session.subtitle, str_or_empty(s["album"]));
    } else {
      session.title = title;
      if (!s["year"].isNull())
        session.subtitle = to_string(s["year"].as<int>());
    }
    if (session.title.empty())
      session.title = title.empty() ? "Unknown" : title;

    append_part(session.device, str_or_empty(s["device"]));
    append_part(session.device, str_or_empty(s["client"]));
    append_part(session.device, str_or_empty(s["user"]));

    this->sessions_.push_back(std::move(session));
  }

  this->loaded_ = true;
  if (this->page_ >= this->page_count_())
    this->page_ = 0;
  ESP_LOGD(TAG, "Received %u sessions", (unsigned) this->sessions_.size());
  this->render_();
}

void EmbyUi::show_message(const std::string &message) {
  this->rows_.clear();
  this->render_empty_(ICON_SLEEP, message);
}

void EmbyUi::next_page() {
  uint8_t pages = this->page_count_();
  if (pages < 2)
    return;
  this->page_ = (this->page_ + 1) % pages;
  this->render_();
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

uint8_t EmbyUi::page_count_() const { return (this->sessions_.size() + PER_PAGE - 1) / PER_PAGE; }

void EmbyUi::init_styles_() {
  lv_style_init(&this->style_card_);
  lv_style_set_bg_color(&this->style_card_, lv_color_hex(colors::SLATE_800));
  lv_style_set_bg_opa(&this->style_card_, LV_OPA_COVER);
  lv_style_set_radius(&this->style_card_, 10);
  lv_style_set_pad_left(&this->style_card_, 10);
  lv_style_set_pad_right(&this->style_card_, 10);
  lv_style_set_pad_ver(&this->style_card_, 8);
  lv_style_set_pad_row(&this->style_card_, 4);
  lv_style_set_border_width(&this->style_card_, 4);
  lv_style_set_border_side(&this->style_card_, LV_BORDER_SIDE_LEFT);

  lv_style_init(&this->style_muted_);
  lv_style_set_text_color(&this->style_muted_, lv_color_hex(colors::SLATE_400));
  lv_style_set_text_font(&this->style_muted_, this->lv_font_small_);
}

void EmbyUi::create_header_(lv_obj_t *parent) {
  lv_obj_t *bar = create_box(parent, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(bar, 6, 0);

  lv_obj_t *icon = lv_label_create(bar);
  lv_label_set_text(icon, ICON_NOW_PLAYING);
  lv_obj_set_style_text_font(icon, this->lv_font_icon_, 0);
  lv_obj_set_style_text_color(icon, lv_color_hex(colors::EMERALD_400), 0);

  lv_obj_t *title = lv_label_create(bar);
  lv_label_set_text(title, "Now Playing");
  lv_obj_set_style_text_font(title, this->lv_font_body_, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(colors::SLATE_200), 0);

  this->count_label_ = lv_label_create(bar);
  lv_label_set_text(this->count_label_, "");
  lv_obj_add_style(this->count_label_, &this->style_muted_, 0);
  lv_obj_set_flex_grow(this->count_label_, 1);

  // "More" pager, only shown when sessions don't fit on one page.
  this->more_button_ = lv_button_create(bar);
  lv_obj_set_height(this->more_button_, 22);
  lv_obj_set_style_bg_color(this->more_button_, lv_color_hex(colors::SLATE_700), 0);
  lv_obj_set_style_radius(this->more_button_, 11, 0);
  lv_obj_set_style_shadow_width(this->more_button_, 0, 0);
  lv_obj_set_style_pad_hor(this->more_button_, 10, 0);
  lv_obj_set_style_pad_ver(this->more_button_, 0, 0);
  lv_obj_set_ext_click_area(this->more_button_, 8);
  lv_obj_add_flag(this->more_button_, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(
      this->more_button_, [](lv_event_t *e) { static_cast<EmbyUi *>(lv_event_get_user_data(e))->next_page(); },
      LV_EVENT_CLICKED, this);

  this->more_label_ = lv_label_create(this->more_button_);
  lv_obj_set_style_text_font(this->more_label_, this->lv_font_small_, 0);
  lv_obj_set_style_text_color(this->more_label_, lv_color_hex(colors::SLATE_200), 0);
  lv_obj_center(this->more_label_);

  this->clock_label_ = lv_label_create(bar);
  lv_label_set_text(this->clock_label_, "--:--");
  lv_obj_set_style_text_font(this->clock_label_, this->lv_font_body_, 0);
  lv_obj_set_style_text_color(this->clock_label_, lv_color_hex(colors::SLATE_400), 0);
}

void EmbyUi::render_() {
  this->rows_.clear();

  size_t count = this->sessions_.size();
  uint8_t pages = this->page_count_();
  lv_label_set_text_fmt(this->count_label_, "%u", (unsigned) count);
  if (pages > 1) {
    lv_label_set_text_fmt(this->more_label_, "More  %u/%u", (unsigned) this->page_ + 1, (unsigned) pages);
    lv_obj_remove_flag(this->more_button_, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(this->more_button_, LV_OBJ_FLAG_HIDDEN);
  }

  if (!this->loaded_) {
    this->render_empty_(ICON_NOW_PLAYING, "Waiting for Home Assistant...");
    return;
  }
  if (count == 0) {
    lv_label_set_text(this->count_label_, "");
    this->render_empty_(ICON_SLEEP, "Nothing playing");
    return;
  }

  lv_obj_clean(this->list_);
  lv_obj_set_flex_align(this->list_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  size_t start = this->page_ * PER_PAGE;
  for (size_t i = start; i < count && i < start + PER_PAGE; i++)
    this->create_card_(this->list_, this->sessions_[i]);
  this->tick_();
}

void EmbyUi::render_empty_(const char *icon, const std::string &message) {
  lv_obj_clean(this->list_);
  lv_obj_set_flex_align(this->list_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t *icon_label = lv_label_create(this->list_);
  lv_label_set_text(icon_label, icon);
  lv_obj_set_style_text_font(icon_label, this->lv_font_icon_, 0);
  lv_obj_set_style_text_color(icon_label, lv_color_hex(colors::SLATE_500), 0);
  lv_obj_set_style_transform_scale(icon_label, 512, 0);  // 2x
  lv_obj_set_style_pad_bottom(icon_label, 12, 0);

  lv_obj_t *label = lv_label_create(this->list_);
  lv_label_set_text(label, message.c_str());
  lv_obj_set_style_text_font(label, this->lv_font_body_, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(colors::SLATE_500), 0);
}

void EmbyUi::create_card_(lv_obj_t *parent, const Session &s) {
  lv_color_t accent = lv_color_hex(s.playing ? colors::EMERALD_400 : colors::AMBER_400);

  lv_obj_t *card = create_box(parent, LV_FLEX_FLOW_COLUMN);
  lv_obj_add_style(card, &this->style_card_, 0);
  lv_obj_set_style_border_color(card, accent, 0);

  // Title row: type icon, title, play/pause state
  lv_obj_t *top = create_box(card, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(top, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(top, 6, 0);

  lv_obj_t *type = lv_label_create(top);
  lv_label_set_text(type, type_icon(s.type));
  lv_obj_set_style_text_font(type, this->lv_font_icon_, 0);
  lv_obj_set_style_text_color(type, lv_color_hex(colors::SLATE_400), 0);

  lv_obj_t *title = lv_label_create(top);
  lv_label_set_text(title, s.title.c_str());
  lv_label_set_long_mode(title, LV_LABEL_LONG_MODE_DOTS);
  lv_obj_set_width(title, 1);  // flex_grow sizes it; the long mode needs a fixed width
  lv_obj_set_flex_grow(title, 1);
  lv_obj_set_style_text_font(title, this->lv_font_title_, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(colors::SLATE_200), 0);

  lv_obj_t *state = lv_label_create(top);
  lv_label_set_text(state, s.playing ? ICON_PLAY : ICON_PAUSE);
  lv_obj_set_style_text_font(state, this->lv_font_icon_, 0);
  lv_obj_set_style_text_color(state, accent, 0);

  if (!s.subtitle.empty()) {
    lv_obj_t *subtitle = lv_label_create(card);
    lv_label_set_text(subtitle, s.subtitle.c_str());
    lv_label_set_long_mode(subtitle, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_width(subtitle, lv_pct(100));
    lv_obj_set_style_text_font(subtitle, this->lv_font_body_, 0);
    lv_obj_set_style_text_color(subtitle, lv_color_hex(colors::SLATE_400), 0);
  }

  lv_obj_t *bar = lv_bar_create(card);
  lv_obj_set_size(bar, lv_pct(100), 4);
  lv_bar_set_range(bar, 0, 1000);
  lv_obj_set_style_bg_color(bar, lv_color_hex(colors::SLATE_700), LV_PART_MAIN);
  lv_obj_set_style_bg_color(bar, accent, LV_PART_INDICATOR);
  lv_obj_set_style_anim_duration(bar, 0, 0);

  // Bottom row: elapsed / duration, device · client · user
  lv_obj_t *bottom = create_box(card, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(bottom, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(bottom, 8, 0);

  lv_obj_t *time_label = lv_label_create(bottom);
  lv_obj_add_style(time_label, &this->style_muted_, 0);

  lv_obj_t *device = lv_label_create(bottom);
  lv_label_set_text(device, s.device.c_str());
  lv_label_set_long_mode(device, LV_LABEL_LONG_MODE_DOTS);
  lv_obj_set_width(device, 1);
  lv_obj_set_flex_grow(device, 1);
  lv_obj_set_style_text_align(device, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_add_style(device, &this->style_muted_, 0);
  lv_obj_set_style_text_color(device, lv_color_hex(colors::SLATE_500), 0);

  this->rows_.push_back({&s, bar, time_label});
}

// ---------------------------------------------------------------------------
// Progress — extrapolated locally from position + updated, like the Lovelace card
// ---------------------------------------------------------------------------

int32_t EmbyUi::elapsed_(const Session &s) const {
  int64_t seconds = s.position;
  if (s.playing && s.updated > 0) {
    auto now = this->time_->now();
    if (now.is_valid() && now.timestamp > s.updated)
      seconds += now.timestamp - s.updated;
  }
  if (s.duration > 0 && seconds > s.duration)
    seconds = s.duration;
  return (int32_t) seconds;
}

void EmbyUi::tick_() {
  for (auto &row : this->rows_) {
    const Session &s = *row.session;
    int32_t elapsed = this->elapsed_(s);
    std::string text = format_duration(elapsed) + " / " + (s.duration > 0 ? format_duration(s.duration) : "--:--");
    lv_label_set_text(row.time_label, text.c_str());
    lv_bar_set_value(row.bar, s.duration > 0 ? (int32_t) ((int64_t) elapsed * 1000 / s.duration) : 0, LV_ANIM_OFF);
  }
}

}  // namespace emby_ui
}  // namespace esphome
