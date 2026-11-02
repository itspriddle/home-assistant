#include "model.h"

namespace esphome {
namespace dashboard {

// Scene dot colors roughly match each Hue scene's palette
static constexpr uint32_t DOT_RELAX = 0xf2b46b;
static constexpr uint32_t DOT_READ = 0xf4e6c4;
static constexpr uint32_t DOT_CONCENTRATE = 0xe9f0ff;
static constexpr uint32_t DOT_ENERGIZE = 0xcfe0ff;
static constexpr uint32_t DOT_REST = 0xe89a5b;
static constexpr uint32_t DOT_NIGHTLIGHT = 0xc46a4f;
static constexpr uint32_t DOT_DIMMED = 0xb98a5a;
static constexpr uint32_t DOT_BRIGHT = 0xffffff;

Model::Model() {
  this->rooms = {
      {"thermostat", "Thermostat"},
      {"office", "Office"},
      {"living_room", "Living Room"},
      {"kitchen", "Kitchen"},
      {"bedroom", "Bedroom"},
  };
  this->room("kitchen")->has_occupancy = false;

  this->lights = {
      {"living_room",
       "Living Room",
       "light.living_room_all",
       {
           {"Relax", "scene.living_room_relax", DOT_RELAX},
           {"Read", "scene.living_room_read", DOT_READ},
           {"Concentrate", "scene.living_room_concentrate", DOT_CONCENTRATE},
           {"Energize", "scene.living_room_energize", DOT_ENERGIZE},
           {"Rest", "scene.living_room_rest", DOT_REST},
           {"Nightlight", "scene.living_room_nightlight", DOT_NIGHTLIGHT},
           {"Osaka", "scene.living_room_osaka", 0xe48ab8},
           {"Rolling hills", "scene.living_room_rolling_hills", 0x9fd08a},
       }},
      {"kitchen",
       "Kitchen",
       "light.kitchen_all",
       {
           {"Relax", "scene.kitchen_relax", DOT_RELAX},
           {"Read", "scene.kitchen_read", DOT_READ},
           {"Concentrate", "scene.kitchen_concentrate", DOT_CONCENTRATE},
           {"Energize", "scene.kitchen_energize", DOT_ENERGIZE},
           {"Dimmed", "scene.kitchen_dimmed", DOT_DIMMED},
           {"Nightlight", "scene.kitchen_nightlight", DOT_NIGHTLIGHT},
       }},
      {"office",
       "Office",
       "light.office",
       {
           {"Concentrate", "scene.office_concentrate", DOT_CONCENTRATE},
           {"Energize", "scene.office_energize", DOT_ENERGIZE},
           {"Read", "scene.office_read", DOT_READ},
           {"Relax", "scene.office_relax", DOT_RELAX},
           {"Natural light", "scene.office_natural_light", 0xfff3d6},
           {"Rest", "scene.office_rest", DOT_REST},
           {"Dimmed", "scene.office_dimmed", DOT_DIMMED},
           {"Honolulu", "scene.office_honolulu", 0xff9f6e},
       }},
      {"bedroom",
       "Bedroom",
       "light.bedroom_all",
       {
           {"Bright", "scene.bedroom_bright", DOT_BRIGHT},
           {"Relax", "scene.bedroom_relax", DOT_RELAX},
           {"Read", "scene.bedroom_read", DOT_READ},
           {"Concentrate", "scene.bedroom_concentrate", DOT_CONCENTRATE},
           {"Energize", "scene.bedroom_energize", DOT_ENERGIZE},
           {"Dimmed", "scene.bedroom_dimmed", DOT_DIMMED},
           {"Nightlight", "scene.bedroom_nightlight", DOT_NIGHTLIGHT},
       }},
      {"hallway",
       "Hallway",
       "light.hallway_all",
       {
           {"Bright", "scene.hallway_bright", DOT_BRIGHT},
           {"Relax", "scene.hallway_relax", DOT_RELAX},
           {"Read", "scene.hallway_read", DOT_READ},
           {"Rest", "scene.hallway_rest", DOT_REST},
           {"Dimmed", "scene.hallway_dimmed", DOT_DIMMED},
           {"Nightlight", "scene.hallway_nightlight", DOT_NIGHTLIGHT},
       }},
  };
}

RoomClimate *Model::room(const std::string &key) {
  for (auto &r : this->rooms) {
    if (key == r.key)
      return &r;
  }
  return nullptr;
}

LightRoom *Model::light(const std::string &key) {
  for (auto &l : this->lights) {
    if (key == l.key)
      return &l;
  }
  return nullptr;
}

int Model::lights_on() const {
  int n = 0;
  for (const auto &l : this->lights) {
    if (l.on)
      n++;
  }
  return n;
}

}  // namespace dashboard
}  // namespace esphome
