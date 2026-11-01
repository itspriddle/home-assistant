#pragma once

#include <string>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/components/json/json_util.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/font/font.h"
#include "lvgl.h"

namespace esphome {
namespace emby_ui {

// Tailwind palette (hex values — use with lv_color_hex())
namespace colors {
static constexpr uint32_t SLATE_200 = 0xe2e8f0;
static constexpr uint32_t SLATE_400 = 0x94a3b8;
static constexpr uint32_t SLATE_500 = 0x64748b;
static constexpr uint32_t SLATE_700 = 0x334155;
static constexpr uint32_t SLATE_800 = 0x1e293b;
static constexpr uint32_t SLATE_950 = 0x020617;
static constexpr uint32_t EMERALD_400 = 0x34d399;
static constexpr uint32_t AMBER_400 = 0xfbbf24;
}  // namespace colors

class EmbyUi : public Component {
 public:
  void set_time(time::RealTimeClock *time) { this->time_ = time; }
  void set_font_title(font::Font *f) { this->font_title_ = f; }
  void set_font_body(font::Font *f) { this->font_body_ = f; }
  void set_font_small(font::Font *f) { this->font_small_ = f; }
  void set_font_icon(font::Font *f) { this->font_icon_ = f; }

  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::LATE; }
  void dump_config() override;

  /// Replace the session list from the emby_now_playing_sessions script response.
  /// Call from YAML: id(emby).set_sessions(response["response"]["sessions"]);
  void set_sessions(JsonArrayConst sessions);

  /// Show a status message in place of the session list (e.g. action errors).
  void show_message(const std::string &message);

  /// Advance to the next page of sessions, wrapping around.
  void next_page();

 protected:
  static constexpr uint8_t PER_PAGE = 2;

  struct Session {
    std::string type;
    std::string title;
    std::string subtitle;
    std::string device;
    bool playing{false};
    int32_t duration{0};
    int32_t position{0};
    int64_t updated{0};
  };

  // Widgets for one visible session, refreshed every second by tick_().
  struct Row {
    const Session *session;
    lv_obj_t *bar;
    lv_obj_t *time_label;
  };

  void init_styles_();
  void create_header_(lv_obj_t *parent);
  void render_();
  void render_empty_(const char *icon, const std::string &message);
  void create_card_(lv_obj_t *parent, const Session &s);
  void tick_();
  int32_t elapsed_(const Session &s) const;
  uint8_t page_count_() const;

  time::RealTimeClock *time_{nullptr};
  font::Font *font_title_{nullptr};
  font::Font *font_body_{nullptr};
  font::Font *font_small_{nullptr};
  font::Font *font_icon_{nullptr};

  const lv_font_t *lv_font_title_{nullptr};
  const lv_font_t *lv_font_body_{nullptr};
  const lv_font_t *lv_font_small_{nullptr};
  const lv_font_t *lv_font_icon_{nullptr};

  lv_style_t style_card_{};
  lv_style_t style_muted_{};

  lv_obj_t *clock_label_{nullptr};
  lv_obj_t *count_label_{nullptr};
  lv_obj_t *more_button_{nullptr};
  lv_obj_t *more_label_{nullptr};
  lv_obj_t *list_{nullptr};

  std::vector<Session> sessions_;
  std::vector<Row> rows_;
  uint8_t page_{0};
  bool loaded_{false};
  uint32_t last_tick_{0};
  int last_minute_{-1};
};

}  // namespace emby_ui
}  // namespace esphome
