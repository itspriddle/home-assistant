#pragma once

#include "screen.h"

namespace esphome {
namespace dashboard {

// Idle screen: big clock, date, and a one-line weather / next-event
// summary. Has no controls; any touch wakes the dashboard (see
// Dashboard::update_idle_).
class ClockScreen : public Screen {
 public:
  void build(lv_obj_t *root, Dashboard *dash) override;
  void update(const Model &model, uint32_t dirty) override;

 protected:
  void update_clock_();
  void update_summary_(const Model &model);

  Dashboard *dash_{nullptr};

  lv_obj_t *clock_{nullptr};
  lv_obj_t *date_{nullptr};
  lv_obj_t *weather_icon_{nullptr};
  lv_obj_t *weather_temp_{nullptr};
  lv_obj_t *separator_{nullptr};
  lv_obj_t *next_event_{nullptr};
};

}  // namespace dashboard
}  // namespace esphome
