#include "panel_bus.h"

namespace esphome {
namespace elekstube_ui {

void PanelBus::setup() {
  for (auto *pin : {this->data_, this->clock_, this->latch_, this->power_}) {
    pin->setup();
    pin->digital_write(false);
  }
  // Panel power is held on: cutting it wipes the panels' init state.
  this->power_->digital_write(true);
  this->select_all();
}

void PanelBus::select(uint8_t mask) {
  const uint8_t out = ~mask;  // CS is active low
  this->latch_->digital_write(false);
  for (int bit = 7; bit >= 0; bit--) {  // MSB first: byte bit n -> Qn
    this->data_->digital_write((out >> bit) & 1);
    this->clock_->digital_write(true);
    this->clock_->digital_write(false);
  }
  this->latch_->digital_write(true);
}

}  // namespace elekstube_ui
}  // namespace esphome
