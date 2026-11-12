#pragma once

#include <array>
#include <memory>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/display/display.h"
#include "esphome/components/font/font.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/time/real_time_clock.h"

#include "modes.h"
#include "panel_bus.h"
#include "types.h"

namespace esphome {
namespace elekstube_ui {

// Drives the four panels through one shared display component: picks what each
// panel should show (via the active mode), diffs against what it shows now, and
// redraws changed panels one at a time, animating digit changes.
//
// It is also an SPI device on the panels' bus (no CS of its own; the 595
// selects panels) so it can send raw panel commands such as sleep and wake.
class ElekstubeUi : public Component,
                    public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_HIGH,
                                          spi::CLOCK_PHASE_TRAILING, spi::DATA_RATE_10MHZ> {
 public:
  void set_display(display::Display *display) { this->display_ = display; }
  void set_time(time::RealTimeClock *time) { this->time_ = time; }
  void set_label_font(font::Font *font) { this->label_font_ = font; }
  void set_dc_pin(GPIOPin *pin) { this->dc_pin_ = pin; }
  void set_bus_pins(GPIOPin *data, GPIOPin *clock, GPIOPin *latch, GPIOPin *power) {
    this->bus_.set_pins(data, clock, latch, power);
  }
  void add_face(font::Font *font, uint32_t top, uint32_t bottom, uint32_t accent, int y_offset) {
    this->faces_.push_back(Face{font, Color(top), Color(bottom), Color(accent), y_offset, top != bottom});
  }

  // Settings, driven by the HA-facing entities in YAML. Safe to call before setup.
  void set_mode(size_t index);
  void set_face(size_t index);
  void set_transition(size_t index);
  void set_use_24h(bool use_24h);
  void set_seconds_bar(bool enabled);
  void set_brightness(float brightness);  // 0..1
  void set_display_on(bool on);

  // Show the date view for a while, then return to the active mode.
  void peek_date(uint32_t duration_ms = 5000);

  void setup() override;
  void loop() override;
  void dump_config() override;
  // After the SPI bus, before the display: the panels must be powered and all
  // selected when the display runs its reset and init sequence.
  float get_setup_priority() const override { return setup_priority::BUS - 1.0f; }

 protected:
  struct Anim {
    bool active{false};
    uint32_t start{0};
    PanelContent from{};
  };

  enum class Power : uint8_t { ON, OFF, WAKING };

  void compose_();
  bool needs_work_(int panel) const;
  void work_(int panel, uint32_t now);
  void render_(uint8_t mask);
  void draw_(display::Display &it);
  void draw_glyph_(display::Display &it, const char *text, int dy, uint8_t level);
  void draw_bar_(display::Display &it, const PanelContent &c);
  Color scale_(Color color, uint8_t level) const;
  Pip status_pip_() const;
  void command_(uint8_t cmd);
  void apply_power_();
  void mark_all_dirty_() { this->dirty_.fill(true); }
  const Face &face_() const { return this->faces_[this->face_index_]; }

  display::Display *display_{nullptr};
  time::RealTimeClock *time_{nullptr};
  font::Font *label_font_{nullptr};
  GPIOPin *dc_pin_{nullptr};
  PanelBus bus_;
  std::vector<Face> faces_;

  ClockMode clock_mode_;
  DateMode date_mode_;
  std::array<const Mode *, static_cast<size_t>(ModeId::COUNT)> modes_{&clock_mode_, &date_mode_};

  // Settings
  ModeId mode_{ModeId::CLOCK};
  size_t face_index_{0};
  Transition transition_{Transition::ROLL};
  Settings settings_;
  uint8_t brightness_{255};
  bool want_on_{true};
  Power power_{Power::ON};
  uint32_t peek_until_{0};

  // Panel state
  Frame desired_{};
  Frame shown_{};
  std::array<bool, PANEL_COUNT> dirty_{};
  std::array<Anim, PANEL_COUNT> anims_{};
  int next_panel_{0};
  bool boot_clear_{true};
  uint32_t last_compose_{0};

  // What the display writer draws during the current update()
  PanelContent drawing_{};
  const Anim *drawing_anim_{nullptr};
  float drawing_progress_{1.0f};
  bool drawing_bar_only_{false};

  // Render timing, logged once a minute
  uint32_t stat_frames_{0};
  uint32_t stat_total_us_{0};
  uint32_t stat_max_us_{0};
  uint32_t stat_since_{0};
};

}  // namespace elekstube_ui
}  // namespace esphome
