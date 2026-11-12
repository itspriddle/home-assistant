#pragma once

#include <cstdint>

#include "esphome/core/hal.h"

namespace esphome {
namespace elekstube_ui {

// Panel power and chip selects. Each panel's CS is a 74HC595 output (active
// low), Q0 = leftmost panel. All panels share SPI, DC and RST.
class PanelBus {
 public:
  void set_pins(GPIOPin *data, GPIOPin *clock, GPIOPin *latch, GPIOPin *power) {
    this->data_ = data;
    this->clock_ = clock;
    this->latch_ = latch;
    this->power_ = power;
  }

  // Power the panels and select all of them, so the display's reset and init
  // sequence reach every panel at once. Must run before the display sets up.
  void setup();

  // Bit n set = panel n selected.
  void select(uint8_t mask);
  void select_all() { this->select(ALL); }
  void deselect() { this->select(0x00); }

  static constexpr uint8_t ALL = 0x0F;

 protected:
  GPIOPin *data_{nullptr};
  GPIOPin *clock_{nullptr};
  GPIOPin *latch_{nullptr};
  GPIOPin *power_{nullptr};
};

}  // namespace elekstube_ui
}  // namespace esphome
