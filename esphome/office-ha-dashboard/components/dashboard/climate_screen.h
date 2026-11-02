#pragma once

#include <array>
#include <vector>

#include "screen.h"

namespace esphome {
namespace dashboard {

class ClimateScreen : public Screen {
 public:
  void build(lv_obj_t *root, Dashboard *dash) override;
  void update(const Model &model, uint32_t dirty) override;

 protected:
  struct ModeButton {
    const char *mode;  // HA hvac mode sent on tap
    lv_obj_t *btn;
    lv_obj_t *text;
  };
  struct RoomRow {
    lv_obj_t *dot;
    lv_obj_t *name;
    lv_obj_t *note;
    lv_obj_t *temp;
  };

  void build_dial_(lv_obj_t *parent);
  void build_side_(lv_obj_t *parent);
  void update_climate_(const Climate &c);
  void update_rooms_(const Model &model);
  void step_setpoint_(float delta);

  Dashboard *dash_{nullptr};
  lv_obj_t *rail_{nullptr};

  lv_obj_t *arc_{nullptr};
  lv_obj_t *action_{nullptr};
  lv_obj_t *setpoint_{nullptr};
  lv_obj_t *inside_{nullptr};
  lv_obj_t *minus_{nullptr};
  lv_obj_t *plus_{nullptr};

  std::array<ModeButton, 4> modes_{};
  std::vector<RoomRow> rows_;
};

}  // namespace dashboard
}  // namespace esphome
