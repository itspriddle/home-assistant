#pragma once

#include <array>
#include <cstdint>
#include <cstring>

#include "esphome/core/color.h"
#include "esphome/components/font/font.h"

namespace esphome {
namespace elekstube_ui {

static constexpr int PANEL_COUNT = 4;
static constexpr int PANEL_W = 135;
static constexpr int PANEL_H = 240;

// Vertical layout of a panel, top to bottom.
static constexpr int GLYPH_AREA_H = 200;  // big glyph is centred in 0..GLYPH_AREA_H
static constexpr int LABEL_Y = 214;       // centre line of the small label
static constexpr int BAR_Y = 232;         // seconds bar
static constexpr int BAR_H = 4;

enum class Pip : uint8_t { NONE, API_DOWN, WIFI_DOWN };

enum PanelFlags : uint8_t {
  FLAG_BAR_TRACK = 1 << 0,  // draw the empty part of the seconds bar
};

// Everything one panel shows. Plain data, compared field-wise to decide redraws.
struct PanelContent {
  char glyph[4]{};  // big text, usually one digit; empty = nothing
  char label[6]{};  // small text under the glyph; empty = nothing
  uint8_t bar_px{0};
  uint8_t flags{0};
  Pip pip{Pip::NONE};

  bool same_glyph(const PanelContent &o) const { return std::strcmp(glyph, o.glyph) == 0; }
  // True when only the seconds bar differs, so a strip redraw is enough.
  bool same_except_bar(const PanelContent &o) const {
    return same_glyph(o) && std::strcmp(label, o.label) == 0 && pip == o.pip;
  }
  bool operator==(const PanelContent &o) const {
    return same_glyph(o) && std::strcmp(label, o.label) == 0 && bar_px == o.bar_px && flags == o.flags &&
           pip == o.pip;
  }
  bool operator!=(const PanelContent &o) const { return !(*this == o); }
};

using Frame = std::array<PanelContent, PANEL_COUNT>;

// A big-digit style: one font plus colours.
struct Face {
  font::Font *font;
  Color top;
  Color bottom;
  Color accent;  // seconds bar, labels
  int y_offset;
  bool gradient;  // top and bottom differ
};

enum class Transition : uint8_t { ROLL, FADE, NONE };

// Settings driven by the HA-facing entities in YAML.
struct Settings {
  bool use_24h{false};
  bool seconds_bar{true};
};

}  // namespace elekstube_ui
}  // namespace esphome
