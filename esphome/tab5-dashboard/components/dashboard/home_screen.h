#pragma once

#include <array>

#include "screen.h"

namespace esphome {
namespace dashboard {

class HomeScreen : public Screen {
 public:
  void build(lv_obj_t *root, Dashboard *dash) override;
  void update(const Model &model, uint32_t dirty) override;

 protected:
  static constexpr size_t FORECAST_DAYS = 4;
  static constexpr size_t ROOM_TILES = 5;

  struct ForecastCol {
    lv_obj_t *col{nullptr};
    lv_obj_t *day{nullptr};
    lv_obj_t *high{nullptr};
    lv_obj_t *low{nullptr};
  };

  struct RoomTile {
    lv_obj_t *tile{nullptr};
    lv_obj_t *dot{nullptr};
    lv_obj_t *name{nullptr};
    lv_obj_t *temp{nullptr};
    lv_obj_t *humidity{nullptr};
  };

  void build_hero_(lv_obj_t *parent);
  void build_weather_(lv_obj_t *parent);
  void build_rooms_(lv_obj_t *parent);
  void build_tiles_(lv_obj_t *parent);

  void update_clock_();
  void update_weather_(const Model &model);
  void update_rooms_(const Model &model);
  void update_lights_(const Model &model);
  void update_climate_(const Model &model);
  void update_status_(const Model &model);

  Dashboard *dash_{nullptr};

  // Clock / status
  lv_obj_t *clock_{nullptr};
  lv_obj_t *date_{nullptr};
  lv_obj_t *status_row_{nullptr};
  lv_obj_t *meeting_chip_{nullptr};
  lv_obj_t *next_event_{nullptr};

  // Weather
  lv_obj_t *weather_temp_{nullptr};
  lv_obj_t *weather_condition_{nullptr};
  lv_obj_t *weather_hilo_{nullptr};
  lv_obj_t *weather_icon_{nullptr};
  std::array<ForecastCol, FORECAST_DAYS> forecast_{};

  // Rooms
  std::array<RoomTile, ROOM_TILES> rooms_{};

  // Bottom tiles
  lv_obj_t *lights_tile_{nullptr};
  lv_obj_t *lights_icon_{nullptr};
  lv_obj_t *lights_title_{nullptr};
  lv_obj_t *lights_sub_{nullptr};
  lv_obj_t *climate_icon_{nullptr};
  lv_obj_t *climate_sub_{nullptr};
  lv_obj_t *printer_title_{nullptr};
  lv_obj_t *printer_pct_{nullptr};
  lv_obj_t *printer_bar_{nullptr};
};

}  // namespace dashboard
}  // namespace esphome
