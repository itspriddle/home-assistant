#pragma once

#include "esphome/core/component.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/font/font.h"
#include "lvgl.h"

namespace esphome {
namespace cyd_ui {

// Tailwind slate palette (hex values — use with lv_color_hex())
namespace colors {
static constexpr uint32_t SLATE_300 = 0xcbd5e1;
static constexpr uint32_t SLATE_500 = 0x64748b;
static constexpr uint32_t SLATE_700 = 0x334155;
static constexpr uint32_t SLATE_800 = 0x1e293b;
static constexpr uint32_t SLATE_900 = 0x0f172a;
static constexpr uint32_t EMERALD_400 = 0x34d399;
static constexpr uint32_t BLUE_400 = 0x60a5fa;
static constexpr uint32_t AMBER_400 = 0xfbbf24;
static constexpr uint32_t RED_400 = 0xf87171;
}  // namespace colors

class CydUi : public Component {
 public:
  void set_time(time::RealTimeClock *time) { this->time_ = time; }
  void set_font_large(font::Font *f) { this->font_large_ = f; }
  void set_font_medium(font::Font *f) { this->font_medium_ = f; }
  void set_font_small(font::Font *f) { this->font_small_ = f; }
  void set_font_icon(font::Font *f) { this->font_icon_ = f; }

  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::LATE; }
  void dump_config() override;

  /// Update a card's value text by its string ID.
  /// Call from YAML lambdas: id(my_ui).update_card("temperature", "72.1 F");
  void update_card(const std::string &card_id, const std::string &value);

  /// Update a card's value text color.
  void update_card_color(const std::string &card_id, lv_color_t color);

 protected:
  const lv_font_t *get_lv_font_(font::Font *f);
  void init_styles_();
  void create_status_bar_(lv_obj_t *parent);
  lv_obj_t *create_card_(lv_obj_t *parent, const std::string &id, const char *title, const char *initial_value);
  lv_obj_t *create_icon_card_(lv_obj_t *parent, const std::string &id, const char *title, const char *initial_value);

  time::RealTimeClock *time_{nullptr};

  // Font pointers (set from YAML config, nullptr = LV_FONT_DEFAULT)
  font::Font *font_large_{nullptr};
  font::Font *font_medium_{nullptr};
  font::Font *font_small_{nullptr};
  font::Font *font_icon_{nullptr};

  // Resolved LVGL font pointers (set in setup)
  const lv_font_t *lv_font_large_{nullptr};
  const lv_font_t *lv_font_medium_{nullptr};
  const lv_font_t *lv_font_small_{nullptr};
  const lv_font_t *lv_font_icon_{nullptr};

  // Reusable styles
  lv_style_t style_card_{};
  lv_style_t style_title_{};
  lv_style_t style_value_{};

  // Widget references for clock updates
  lv_obj_t *time_label_{nullptr};
  lv_obj_t *date_label_{nullptr};

  // Card registry — look up value labels by string ID
  static constexpr uint8_t MAX_CARDS = 8;
  struct Card {
    std::string id;
    lv_obj_t *value_label;
  };
  Card cards_[MAX_CARDS]{};
  uint8_t card_count_{0};

  uint8_t last_minute_{255};
};

}  // namespace cyd_ui
}  // namespace esphome
