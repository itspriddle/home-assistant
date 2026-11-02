#pragma once

#include <cstdint>

#include "lvgl.h"
#include "model.h"

namespace esphome {
namespace dashboard {

class Dashboard;

enum class ScreenId : uint8_t { HOME = 0, LIGHTS, CLIMATE, CLOCK, COUNT };

// One full-screen page. Built once at boot, then update() is called with
// the dirty flags whenever the model changes (and once with DIRTY_ALL
// right after build, and again whenever the screen is shown).
class Screen {
 public:
  virtual ~Screen() = default;

  // `root` is an lv screen object (1280x720, BG color, no padding).
  virtual void build(lv_obj_t *root, Dashboard *dash) = 0;
  virtual void update(const Model &model, uint32_t dirty) = 0;

  lv_obj_t *root() const { return this->root_; }
  void set_root(lv_obj_t *root) { this->root_ = root; }

 protected:
  lv_obj_t *root_{nullptr};
};

// Shared left nav rail for Lights / Climate. Returns the rail; the
// caller lays the rest of the screen out to its right (RAIL_W wide).
// Owns its own clock label, refreshed via update_nav_rail().
lv_obj_t *build_nav_rail(lv_obj_t *root, Dashboard *dash, ScreenId active);
void update_nav_rail(lv_obj_t *rail, Dashboard *dash);

}  // namespace dashboard
}  // namespace esphome
