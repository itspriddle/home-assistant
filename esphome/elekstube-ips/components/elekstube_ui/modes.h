#pragma once

#include "esphome/core/time.h"
#include "types.h"

namespace esphome {
namespace elekstube_ui {

// Index order matches the Mode select options in YAML.
enum class ModeId : uint8_t { CLOCK, DATE, COUNT };

// A mode turns the current time and settings into what each panel shows.
// Status (the pip) is filled in by the caller afterwards.
class Mode {
 public:
  virtual ~Mode() = default;
  virtual void compose(const ESPTime &now, const Settings &settings, Frame &frame) const = 0;
};

class ClockMode : public Mode {
 public:
  void compose(const ESPTime &now, const Settings &settings, Frame &frame) const override;
};

class DateMode : public Mode {
 public:
  void compose(const ESPTime &now, const Settings &settings, Frame &frame) const override;
};

// Shared by modes that need the time: dashes on every panel.
void compose_no_time(Frame &frame);

}  // namespace elekstube_ui
}  // namespace esphome
