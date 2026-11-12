#include "modes.h"

#include <algorithm>
#include <cstdio>

namespace esphome {
namespace elekstube_ui {

namespace {

constexpr int BAR_TOTAL = PANEL_W * PANEL_COUNT;

void set_digits(Frame &frame, int left_pair, int right_pair) {
  std::snprintf(frame[0].glyph, sizeof frame[0].glyph, "%d", left_pair / 10);
  std::snprintf(frame[1].glyph, sizeof frame[1].glyph, "%d", left_pair % 10);
  std::snprintf(frame[2].glyph, sizeof frame[2].glyph, "%d", right_pair / 10);
  std::snprintf(frame[3].glyph, sizeof frame[3].glyph, "%d", right_pair % 10);
}

void set_label(PanelContent &panel, const char *text) {
  std::snprintf(panel.label, sizeof panel.label, "%s", text);
}

}  // namespace

void compose_no_time(Frame &frame) {
  for (auto &panel : frame) {
    panel = PanelContent{};
    std::snprintf(panel.glyph, sizeof panel.glyph, "-");
  }
}

void ClockMode::compose(const ESPTime &now, const Settings &settings, Frame &frame) const {
  if (!now.is_valid()) {
    compose_no_time(frame);
    return;
  }

  int hour = now.hour;
  if (!settings.use_24h)
    hour = hour % 12 == 0 ? 12 : hour % 12;
  set_digits(frame, hour, now.minute);  // leading zero kept: 01:30 -> 0130

  if (!settings.use_24h)
    set_label(frame[3], now.hour < 12 ? "AM" : "PM");

  if (settings.seconds_bar) {
    // The bar fills across all four panels over the minute, ~9 px per second,
    // so only the panel holding its leading edge changes each second.
    const int fill = now.second * BAR_TOTAL / 60;
    for (int p = 0; p < PANEL_COUNT; p++) {
      frame[p].bar_px = static_cast<uint8_t>(std::clamp(fill - p * PANEL_W, 0, PANEL_W));
      frame[p].flags |= FLAG_BAR_TRACK;
    }
  }
}

void DateMode::compose(const ESPTime &now, const Settings & /*settings*/, Frame &frame) const {
  static const char *const MONTHS[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                       "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
  static const char *const WEEKDAYS[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

  if (!now.is_valid()) {
    compose_no_time(frame);
    return;
  }

  set_digits(frame, now.month, now.day_of_month);
  // Labels sit under the inner panels so the layout stays symmetric.
  set_label(frame[1], MONTHS[(now.month - 1) % 12]);
  set_label(frame[2], WEEKDAYS[(now.day_of_week - 1) % 7]);
}

}  // namespace elekstube_ui
}  // namespace esphome
