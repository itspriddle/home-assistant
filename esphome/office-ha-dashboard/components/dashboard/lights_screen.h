#pragma once

#include <array>
#include <cstddef>

#include "screen.h"

namespace esphome {
namespace dashboard {

class LightsScreen : public Screen {
 public:
  void build(lv_obj_t *root, Dashboard *dash) override;
  void update(const Model &model, uint32_t dirty) override;

 protected:
  static constexpr size_t MAX_ROOMS = 5;
  static constexpr size_t MAX_SCENES = 8;

  struct Tile {
    lv_obj_t *obj{nullptr};
    lv_obj_t *name{nullptr};
    lv_obj_t *status{nullptr};
    lv_obj_t *power{nullptr};
    lv_obj_t *power_icon{nullptr};
  };

  struct SceneButton {
    lv_obj_t *obj{nullptr};
    lv_obj_t *dot{nullptr};
    lv_obj_t *name{nullptr};
  };

  void build_header_(lv_obj_t *parent);
  void build_tiles_(lv_obj_t *parent);
  void build_scenes_(lv_obj_t *parent);
  void build_brightness_(lv_obj_t *parent);

  void select_(size_t index);
  void step_brightness_(int delta);
  void redraw_(const Model &model);

  Dashboard *dash_{nullptr};
  lv_obj_t *rail_{nullptr};
  size_t room_count_{0};
  size_t selected_{0};  // index into Model::lights; UI-local

  std::array<Tile, MAX_ROOMS> tiles_{};
  lv_obj_t *scenes_title_{nullptr};
  std::array<SceneButton, MAX_SCENES> scenes_{};
  lv_obj_t *bri_bar_{nullptr};
  lv_obj_t *bri_label_{nullptr};
};

}  // namespace dashboard
}  // namespace esphome
