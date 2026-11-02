#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace esphome {
namespace dashboard {

// Everything the UI renders. Setters on Dashboard write here and raise a
// dirty flag; screens read from it in update().

enum Dirty : uint32_t {
  DIRTY_CLOCK = 1 << 0,
  DIRTY_WEATHER = 1 << 1,
  DIRTY_ROOMS = 1 << 2,
  DIRTY_LIGHTS = 1 << 3,
  DIRTY_CLIMATE = 1 << 4,
  DIRTY_STATUS = 1 << 5,
  DIRTY_ALL = 0xFFFFFFFF,
};

struct ForecastDay {
  std::string day;  // "Wed"
  std::string condition;  // HA condition, e.g. "partlycloudy"
  float high{NAN};
  float low{NAN};
};

struct Weather {
  float temperature{NAN};
  std::string condition;
  float today_high{NAN};
  float today_low{NAN};
  std::array<ForecastDay, 4> days{};
};

// Rooms shown in the temperature strip and on the climate screen
struct RoomClimate {
  const char *key;
  const char *name;
  float temperature{NAN};
  float humidity{NAN};
  bool occupied{false};
  bool has_occupancy{true};
};

struct SceneDef {
  const char *name;
  const char *entity_id;
  uint32_t dot_color;
};

struct LightRoom {
  const char *key;
  const char *name;
  const char *entity_id;
  std::vector<SceneDef> scenes;
  bool on{false};
  uint8_t brightness_pct{0};
  int active_scene{-1};  // index into scenes; tracked locally, Hue doesn't report it
  uint32_t hold_until{0};  // millis(); ignore HA echoes until then
};

struct Climate {
  float current{NAN};
  float humidity{NAN};
  float setpoint{NAN};
  std::string mode;  // off / heat / cool / heat_cool / auto
  std::string action;  // idle / heating / cooling / off
  uint32_t hold_until{0};
};

struct Status {
  bool meeting{false};
  std::string next_event;  // "Design review · 5:00 PM", empty = hide
  std::string printer_status;  // Bambu print_status: idle / running / pause / finish / failed / offline
  float printer_progress{NAN};  // 0-100
  float printer_remaining{NAN};  // hours
};

struct Model {
  Weather weather;
  std::vector<RoomClimate> rooms;
  std::vector<LightRoom> lights;
  Climate climate;
  Status status;
  bool wifi_connected{false};
  bool api_connected{false};

  Model();

  RoomClimate *room(const std::string &key);
  LightRoom *light(const std::string &key);
  int lights_on() const;
};

}  // namespace dashboard
}  // namespace esphome
