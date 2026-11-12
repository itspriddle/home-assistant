#include "elekstube_ui.h"

#include <algorithm>
#include <cmath>

#include "esphome/core/log.h"

#ifdef USE_API
#include "esphome/components/api/api_server.h"
#endif
#ifdef USE_WIFI
#include "esphome/components/wifi/wifi_component.h"
#endif

namespace esphome {
namespace elekstube_ui {

static const char *const TAG = "elekstube_ui";

static constexpr uint32_t COMPOSE_INTERVAL_MS = 100;
static constexpr uint32_t ANIM_DURATION_MS = 400;
static constexpr uint32_t STATS_INTERVAL_MS = 60000;
static constexpr int GRADIENT_BANDS = 10;

// ST7789 commands
static constexpr uint8_t CMD_SLPIN = 0x10;
static constexpr uint8_t CMD_SLPOUT = 0x11;
static constexpr uint8_t CMD_DISPOFF = 0x28;
static constexpr uint8_t CMD_DISPON = 0x29;
static constexpr uint32_t SLPOUT_DELAY_MS = 120;

static const Color BAR_TRACK_COLOR(0x26, 0x26, 0x26);
static const Color PIP_API_COLOR(0xFF, 0xA0, 0x00);
static const Color PIP_WIFI_COLOR(0xFF, 0x20, 0x20);

// Ease-out with a small overshoot, so a rolled-in digit settles with a bounce.
static float ease_out_back(float t) {
  constexpr float c1 = 1.70158f;
  constexpr float c3 = c1 + 1.0f;
  const float u = t - 1.0f;
  return 1.0f + c3 * u * u * u + c1 * u * u;
}

// --- Settings ---

void ElekstubeUi::set_mode(size_t index) {
  if (index >= static_cast<size_t>(ModeId::COUNT))
    return;
  this->mode_ = static_cast<ModeId>(index);
  this->peek_until_ = 0;
  this->last_compose_ = 0;
}

void ElekstubeUi::set_face(size_t index) {
  if (index >= this->faces_.size() || index == this->face_index_)
    return;
  this->face_index_ = index;
  this->mark_all_dirty_();
}

void ElekstubeUi::set_transition(size_t index) {
  if (index <= static_cast<size_t>(Transition::NONE))
    this->transition_ = static_cast<Transition>(index);
}

void ElekstubeUi::set_use_24h(bool use_24h) {
  this->settings_.use_24h = use_24h;
  this->last_compose_ = 0;
}

void ElekstubeUi::set_seconds_bar(bool enabled) {
  this->settings_.seconds_bar = enabled;
  this->last_compose_ = 0;
}

void ElekstubeUi::set_brightness(float brightness) {
  const auto level = static_cast<uint8_t>(std::lround(std::clamp(brightness, 0.0f, 1.0f) * 255.0f));
  if (level == this->brightness_)
    return;
  this->brightness_ = level;
  this->mark_all_dirty_();
}

void ElekstubeUi::set_display_on(bool on) { this->want_on_ = on; }

void ElekstubeUi::peek_date(uint32_t duration_ms) {
  this->peek_until_ = millis() + duration_ms;
  if (this->peek_until_ == 0)
    this->peek_until_ = 1;  // 0 means "no peek"
  this->last_compose_ = 0;
}

// --- Lifecycle ---

void ElekstubeUi::setup() {
  if (this->faces_.empty()) {
    this->mark_failed(LOG_STR("No faces configured"));
    return;
  }
  this->bus_.setup();
  this->spi_setup();
  // The writer clears for full redraws itself, so bar-only updates touch (and
  // flush) just the bar strip. Needs a 100% display buffer: with a partial one,
  // mipi_spi stops at the first buffer pass that draws nothing.
  this->display_->set_auto_clear(false);
  this->display_->set_writer([this](display::Display &it) { this->draw_(it); });
  this->stat_since_ = millis();
}

void ElekstubeUi::dump_config() {
  ESP_LOGCONFIG(TAG,
                "EleksTube UI:\n"
                "  Faces: %u\n"
                "  Panels: %d x %dx%d",
                static_cast<unsigned>(this->faces_.size()), PANEL_COUNT, PANEL_W, PANEL_H);
  LOG_PIN("  DC Pin: ", this->dc_pin_);
}

void ElekstubeUi::loop() {
  const uint32_t now = millis();

  this->apply_power_();
  if (this->power_ != Power::ON)
    return;

  if (this->boot_clear_) {
    // Panel RAM holds noise after init: clear every panel in one pass.
    this->drawing_ = PanelContent{};
    this->drawing_anim_ = nullptr;
    this->drawing_bar_only_ = false;
    this->render_(PanelBus::ALL);
    this->shown_.fill(PanelContent{});
    this->boot_clear_ = false;
    return;
  }

  if (this->last_compose_ == 0 || now - this->last_compose_ >= COMPOSE_INTERVAL_MS) {
    this->compose_();
    this->last_compose_ = now == 0 ? 1 : now;
  }

  // One panel frame per loop keeps each call short; round-robin so several
  // animating panels share the frames.
  for (int i = 0; i < PANEL_COUNT; i++) {
    const int panel = (this->next_panel_ + i) % PANEL_COUNT;
    if (this->needs_work_(panel)) {
      this->work_(panel, now);
      this->next_panel_ = (panel + 1) % PANEL_COUNT;
      break;
    }
  }

  if (now - this->stat_since_ >= STATS_INTERVAL_MS) {
    if (this->stat_frames_ > 0) {
      ESP_LOGD(TAG, "Rendered %u frames, avg %.1f ms, max %.1f ms", static_cast<unsigned>(this->stat_frames_),
               this->stat_total_us_ / 1000.0f / this->stat_frames_, this->stat_max_us_ / 1000.0f);
    }
    this->stat_frames_ = this->stat_total_us_ = this->stat_max_us_ = 0;
    this->stat_since_ = now;
  }
}

// --- Composition ---

void ElekstubeUi::compose_() {
  ModeId view = this->mode_;
  if (this->peek_until_ != 0) {
    if (static_cast<int32_t>(millis() - this->peek_until_) < 0) {
      view = ModeId::DATE;
    } else {
      this->peek_until_ = 0;
    }
  }

  Frame frame{};
  this->modes_[static_cast<size_t>(view)]->compose(this->time_->now(), this->settings_, frame);
  frame[PANEL_COUNT - 1].pip = this->status_pip_();
  this->desired_ = frame;
}

Pip ElekstubeUi::status_pip_() const {
#ifdef USE_WIFI
  if (wifi::global_wifi_component != nullptr && !wifi::global_wifi_component->is_connected())
    return Pip::WIFI_DOWN;
#endif
#ifdef USE_API
  if (api::global_api_server != nullptr && !api::global_api_server->is_connected())
    return Pip::API_DOWN;
#endif
  return Pip::NONE;
}

// --- Rendering ---

bool ElekstubeUi::needs_work_(int panel) const {
  return this->anims_[panel].active || this->dirty_[panel] || this->shown_[panel] != this->desired_[panel];
}

void ElekstubeUi::work_(int panel, uint32_t now) {
  Anim &anim = this->anims_[panel];
  const PanelContent &target = this->desired_[panel];
  const PanelContent &shown = this->shown_[panel];

  // Settings changes (dirty) redraw in place; only content changes animate.
  if (!anim.active && !this->dirty_[panel] && this->transition_ != Transition::NONE && shown.glyph[0] != '\0' &&
      !shown.same_glyph(target)) {
    anim.active = true;
    anim.start = now;
    anim.from = shown;
  }

  this->drawing_ = target;
  this->drawing_anim_ = nullptr;
  this->drawing_bar_only_ = !anim.active && !this->dirty_[panel] && shown.same_except_bar(target);
  if (anim.active) {
    const float progress = static_cast<float>(now - anim.start) / ANIM_DURATION_MS;
    if (progress >= 1.0f) {
      anim.active = false;  // this frame draws the settled glyph
    } else {
      this->drawing_anim_ = &anim;
      this->drawing_progress_ = progress;
    }
  }

  this->render_(1 << panel);
  this->shown_[panel] = target;
  this->dirty_[panel] = false;
}

void ElekstubeUi::render_(uint8_t mask) {
  const uint32_t start = micros();
  this->bus_.select(mask);
  this->display_->update();
  this->bus_.deselect();
  const uint32_t elapsed = micros() - start;
  this->stat_frames_++;
  this->stat_total_us_ += elapsed;
  this->stat_max_us_ = std::max(this->stat_max_us_, elapsed);
}

Color ElekstubeUi::scale_(Color color, uint8_t level) const {
  const uint32_t l = static_cast<uint32_t>(level) * this->brightness_ / 255;
  return Color(color.r * l / 255, color.g * l / 255, color.b * l / 255);
}

// Display writer; everything it reads is fixed before update() starts.
void ElekstubeUi::draw_(display::Display &it) {
  const PanelContent &c = this->drawing_;
  const Face &face = this->face_();

  if (this->drawing_bar_only_) {
    this->draw_bar_(it, c);
    return;
  }
  it.fill(Color::BLACK);

  // Keep rolling digits out of the label and bar area.
  it.start_clipping(0, 0, PANEL_W, GLYPH_AREA_H);
  if (this->drawing_anim_ == nullptr) {
    this->draw_glyph_(it, c.glyph, 0, 255);
  } else {
    const char *from = this->drawing_anim_->from.glyph;
    const float t = this->drawing_progress_;
    if (this->transition_ == Transition::FADE) {
      if (t < 0.5f) {
        this->draw_glyph_(it, from, 0, static_cast<uint8_t>((1.0f - 2.0f * t) * 255));
      } else {
        this->draw_glyph_(it, c.glyph, 0, static_cast<uint8_t>((2.0f * t - 1.0f) * 255));
      }
    } else {
      const float e = ease_out_back(t);
      this->draw_glyph_(it, from, static_cast<int>(-e * GLYPH_AREA_H), 255);
      this->draw_glyph_(it, c.glyph, static_cast<int>((1.0f - e) * GLYPH_AREA_H), 255);
    }
  }
  it.end_clipping();

  if (c.label[0] != '\0' && this->label_font_ != nullptr) {
    it.print(PANEL_W / 2, LABEL_Y, this->label_font_, this->scale_(face.accent, 255), display::TextAlign::CENTER,
             c.label);
  }

  this->draw_bar_(it, c);

  if (c.pip != Pip::NONE) {
    const Color color = c.pip == Pip::WIFI_DOWN ? PIP_WIFI_COLOR : PIP_API_COLOR;
    it.filled_circle(PANEL_W - 10, 10, 4, this->scale_(color, 255));
  }
}

// Paints the whole strip, so it also erases a bar that was switched off.
void ElekstubeUi::draw_bar_(display::Display &it, const PanelContent &c) {
  const Color track = (c.flags & FLAG_BAR_TRACK) ? this->scale_(BAR_TRACK_COLOR, 255) : Color::BLACK;
  it.filled_rectangle(0, BAR_Y, PANEL_W, BAR_H, track);
  if (c.bar_px > 0)
    it.filled_rectangle(0, BAR_Y, c.bar_px, BAR_H, this->scale_(this->face_().accent, 255));
}

void ElekstubeUi::draw_glyph_(display::Display &it, const char *text, int dy, uint8_t level) {
  if (text[0] == '\0' || level == 0)
    return;
  const Face &face = this->face_();
  font::Font *font = face.font;
  const int cap = font->get_capheight();
  const int baseline = (GLYPH_AREA_H + cap) / 2 + face.y_offset + dy;
  const int x = PANEL_W / 2;

  if (!face.gradient) {
    it.print(x, baseline, font, this->scale_(face.top, level), display::TextAlign::BASELINE_CENTER, text);
    return;
  }

  // Vertical gradient: print the glyph once per horizontal band, clipped to
  // that band, in the band's colour. The outer bands extend past the glyph.
  const int top = baseline - cap;
  for (int band = 0; band < GRADIENT_BANDS; band++) {
    const int y0 = band == 0 ? top - PANEL_H : top + cap * band / GRADIENT_BANDS;
    const int y1 = band == GRADIENT_BANDS - 1 ? baseline + PANEL_H : top + cap * (band + 1) / GRADIENT_BANDS;
    const float f = (band + 0.5f) / GRADIENT_BANDS;
    const Color color = face.top.gradient(face.bottom, static_cast<uint8_t>(f * 255));
    it.start_clipping(0, y0, PANEL_W, y1);
    it.print(x, baseline, font, this->scale_(color, level), display::TextAlign::BASELINE_CENTER, text);
    it.end_clipping();
  }
}

// --- Panel power ---

void ElekstubeUi::command_(uint8_t cmd) {
  this->dc_pin_->digital_write(false);
  this->enable();
  this->write_byte(cmd);
  this->disable();
}

// Sleep keeps the panels' init state, unlike cutting panel power, so waking
// needs no re-init.
void ElekstubeUi::apply_power_() {
  if (this->want_on_ && this->power_ == Power::OFF) {
    this->bus_.select_all();
    this->command_(CMD_SLPOUT);
    this->bus_.deselect();
    this->power_ = Power::WAKING;
    this->set_timeout("wake", SLPOUT_DELAY_MS, [this]() {
      this->bus_.select_all();
      this->command_(CMD_DISPON);
      this->bus_.deselect();
      this->power_ = Power::ON;
      this->mark_all_dirty_();
    });
  } else if (!this->want_on_ && this->power_ == Power::ON) {
    this->bus_.select_all();
    this->command_(CMD_DISPOFF);
    this->command_(CMD_SLPIN);
    this->bus_.deselect();
    this->power_ = Power::OFF;
    for (auto &anim : this->anims_)
      anim.active = false;
  }
}

}  // namespace elekstube_ui
}  // namespace esphome
