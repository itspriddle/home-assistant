#include "cyd_ui.h"
#include "esphome/core/log.h"

namespace esphome {
namespace cyd_ui {

static const char *const TAG = "cyd_ui";

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void CydUi::setup() {
  ESP_LOGCONFIG(TAG, "Setting up CYD UI...");

  // Resolve ESPHome fonts to LVGL font pointers
  this->lv_font_large_ = this->get_lv_font_(this->font_large_);
  this->lv_font_medium_ = this->get_lv_font_(this->font_medium_);
  this->lv_font_small_ = this->get_lv_font_(this->font_small_);
  this->lv_font_icon_ = this->get_lv_font_(this->font_icon_);

  this->init_styles_();

  // Create and load a fresh screen so we own it completely,
  // regardless of what ESPHome's LVGL component initializes.
  lv_obj_t *scr = lv_obj_create(nullptr);
  lv_scr_load(scr);
  lv_obj_set_style_bg_color(scr, lv_color_hex(colors::SLATE_900), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(scr, 8, 0);
  lv_obj_set_style_pad_row(scr, 6, 0);
  lv_obj_set_style_border_width(scr, 0, 0);
  lv_obj_set_layout(scr, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);

  // Build the UI
  this->create_status_bar_(scr);

  // Weather cards — values updated at runtime via update_card() from YAML lambdas.
  this->create_icon_card_(scr, "weather_icon", "Cond", "\xF3\xB0\x96\x95");
  this->create_card_(scr, "weather_cond", "Condition", "---");
  this->create_card_(scr, "weather_temp", "Temperature", "--\xC2\xB0");
  this->create_card_(scr, "weather_high", "High", "--\xC2\xB0");
  this->create_card_(scr, "weather_low", "Low", "--\xC2\xB0");

  ESP_LOGCONFIG(TAG, "CYD UI ready (%d cards)", this->card_count_);
}

void CydUi::loop() {
  if (this->time_ == nullptr)
    return;

  auto now = this->time_->now();
  if (!now.is_valid())
    return;

  // Only redraw the clock once per minute
  if (now.minute == this->last_minute_)
    return;
  this->last_minute_ = now.minute;

  // Time — 12-hour format
  if (this->time_label_ != nullptr) {
    int hour = now.hour % 12;
    if (hour == 0)
      hour = 12;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d:%02d %s", hour, now.minute, now.hour < 12 ? "AM" : "PM");
    lv_label_set_text(this->time_label_, buf);
  }

  // Date
  if (this->date_label_ != nullptr) {
    auto date_str = now.strftime("%a, %b %d");
    lv_label_set_text(this->date_label_, date_str.c_str());
  }
}

void CydUi::dump_config() {
  ESP_LOGCONFIG(TAG, "CYD UI:");
  ESP_LOGCONFIG(TAG, "  Font large: %s", this->font_large_ ? "custom" : "default");
  ESP_LOGCONFIG(TAG, "  Font medium: %s", this->font_medium_ ? "custom" : "default");
  ESP_LOGCONFIG(TAG, "  Font small: %s", this->font_small_ ? "custom" : "default");
  ESP_LOGCONFIG(TAG, "  Font icon: %s", this->font_icon_ ? "custom" : "default");
  ESP_LOGCONFIG(TAG, "  Cards: %d", this->card_count_);
}

// ---------------------------------------------------------------------------
// Public API — call from YAML lambdas
// ---------------------------------------------------------------------------

void CydUi::update_card(const std::string &card_id, const std::string &value) {
  for (uint8_t i = 0; i < this->card_count_; i++) {
    if (this->cards_[i].id == card_id) {
      lv_label_set_text(this->cards_[i].value_label, value.c_str());
      return;
    }
  }
  ESP_LOGW(TAG, "Card not found: %s", card_id.c_str());
}

void CydUi::update_card_color(const std::string &card_id, lv_color_t color) {
  for (uint8_t i = 0; i < this->card_count_; i++) {
    if (this->cards_[i].id == card_id) {
      lv_obj_set_style_text_color(this->cards_[i].value_label, color, 0);
      return;
    }
  }
}

// ---------------------------------------------------------------------------
// Fonts
// ---------------------------------------------------------------------------

const lv_font_t *CydUi::get_lv_font_(font::Font *f) {
  if (f != nullptr) {
    return f->get_lv_font();
  }
  return LV_FONT_DEFAULT;
}

// ---------------------------------------------------------------------------
// Styles — defined once, applied to every card
// ---------------------------------------------------------------------------

void CydUi::init_styles_() {
  // Card container
  lv_style_init(&this->style_card_);
  lv_style_set_bg_color(&this->style_card_, lv_color_hex(colors::SLATE_800));
  lv_style_set_bg_opa(&this->style_card_, LV_OPA_COVER);
  lv_style_set_radius(&this->style_card_, 12);
  lv_style_set_pad_hor(&this->style_card_, 12);
  lv_style_set_pad_ver(&this->style_card_, 10);
  lv_style_set_border_width(&this->style_card_, 0);
  lv_style_set_width(&this->style_card_, lv_pct(100));
  lv_style_set_height(&this->style_card_, LV_SIZE_CONTENT);

  // Card title (left side, muted)
  lv_style_init(&this->style_title_);
  lv_style_set_text_color(&this->style_title_, lv_color_hex(colors::SLATE_500));
  lv_style_set_text_font(&this->style_title_, this->lv_font_small_);

  // Card value (right side, bright)
  lv_style_init(&this->style_value_);
  lv_style_set_text_color(&this->style_value_, lv_color_hex(colors::SLATE_300));
  lv_style_set_text_font(&this->style_value_, this->lv_font_medium_);
}

// ---------------------------------------------------------------------------
// Widget builders
// ---------------------------------------------------------------------------

void CydUi::create_status_bar_(lv_obj_t *parent) {
  lv_obj_t *bar = lv_obj_create(parent);
  lv_obj_set_width(bar, lv_pct(100));
  lv_obj_set_height(bar, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(bar, 0, 0);
  lv_obj_set_style_pad_all(bar, 0, 0);
  lv_obj_set_layout(bar, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // Time label (left, large font)
  this->time_label_ = lv_label_create(bar);
  lv_label_set_text(this->time_label_, "--:--");
  lv_obj_set_style_text_color(this->time_label_, lv_color_hex(colors::SLATE_300), 0);
  lv_obj_set_style_text_font(this->time_label_, this->lv_font_large_, 0);

  // Date label (right, small font)
  this->date_label_ = lv_label_create(bar);
  lv_label_set_text(this->date_label_, "---");
  lv_obj_set_style_text_color(this->date_label_, lv_color_hex(colors::SLATE_500), 0);
  lv_obj_set_style_text_font(this->date_label_, this->lv_font_small_, 0);
}

lv_obj_t *CydUi::create_card_(lv_obj_t *parent, const std::string &id, const char *title,
                               const char *initial_value) {
  if (this->card_count_ >= MAX_CARDS) {
    ESP_LOGW(TAG, "Max cards reached, ignoring: %s", id.c_str());
    return nullptr;
  }

  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_add_style(card, &this->style_card_, 0);
  lv_obj_set_layout(card, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // Title (left)
  lv_obj_t *title_label = lv_label_create(card);
  lv_label_set_text(title_label, title);
  lv_obj_add_style(title_label, &this->style_title_, 0);

  // Value (right)
  lv_obj_t *value_label = lv_label_create(card);
  lv_label_set_text(value_label, initial_value);
  lv_obj_add_style(value_label, &this->style_value_, 0);

  // Register for lookup
  this->cards_[this->card_count_] = {id, value_label};
  this->card_count_++;

  return card;
}

lv_obj_t *CydUi::create_icon_card_(lv_obj_t *parent, const std::string &id, const char *title,
                                    const char *initial_value) {
  if (this->card_count_ >= MAX_CARDS) {
    ESP_LOGW(TAG, "Max cards reached, ignoring: %s", id.c_str());
    return nullptr;
  }

  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_add_style(card, &this->style_card_, 0);
  lv_obj_set_layout(card, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // Title (left)
  lv_obj_t *title_label = lv_label_create(card);
  lv_label_set_text(title_label, title);
  lv_obj_add_style(title_label, &this->style_title_, 0);

  // Value (right) — uses icon font
  lv_obj_t *value_label = lv_label_create(card);
  lv_label_set_text(value_label, initial_value);
  lv_obj_add_style(value_label, &this->style_value_, 0);
  lv_obj_set_style_text_font(value_label, this->lv_font_icon_, 0);

  // Register for lookup
  this->cards_[this->card_count_] = {id, value_label};
  this->card_count_++;

  return card;
}

}  // namespace cyd_ui
}  // namespace esphome
