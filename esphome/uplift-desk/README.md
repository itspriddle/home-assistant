# Josh's ESPHome UPLIFT Desk Controller

ESPHome firmware for controlling an UPLIFT v2 standing desk from Home Assistant
using an ESP32 DevKit V1. The controller sits between the desk control box and
the keypad. It reads the desk height over UART and sends up/down presses by
pulling the keypad button lines low.

Height decoding uses the `standing_desk_height` component from
[tjhorner/esphome-standing-desk](https://github.com/tjhorner/esphome-standing-desk).

## Hardware

- ESP32 DevKit V1 (USB-C)
- RJ45 breakout board / pass-through adapter
- 3D-printed enclosure: [`enclosure/`](enclosure/)

See [`docs/hardware.md`](docs/hardware.md) for part links and measurements.

## Wiring

RJ45 pins are numbered 1–8 on the breakout board. Only six are used.

| RJ45 pin | Signal                     | ESP32 DevKit V1 | Config          |
|----------|----------------------------|-----------------|-----------------|
| 2        | Desk TX (height data)      | D16 / RX2       | `uart.rx_pin`   |
| 3        | GND                        | GND             |                 |
| 4        | Handset TX (to desk)       | D17 / TX2       | `uart.tx_pin`   |
| 5        | +5V from desk              | VIN             |                 |
| 7        | Up                         | D19             | `hs0_up`        |
| 8        | Down                       | D18             | `hs1_down`      |

Notes:

- Up and down were found by testing on this desk. They are the opposite of
  the usual Jarvis/UPLIFT pinout (pin 8 = HS0 = up). If the buttons move the
  desk the wrong way, swap the two `pin:` values under `output:` in the config.
- The desk powers the ESP32 through VIN. Don't connect USB while the desk is
  plugged in unless you know your board has a diode protecting VIN. Flash over
  USB once with the RJ45 unplugged, then use OTA after that.
- The desk uses 5V logic and the ESP32 is a 3.3V part. Connecting them directly
  works but is outside the ESP32's spec.

## Setup

1. Fill in the shared `secrets.yaml` at the repo root (see
   `secrets.yaml.example`). `uplift-desk/secrets.yaml` just includes it.

2. Flash over USB (first time) or OTA, from the repo root:

   ```sh
   esphome run uplift-desk/uplift-desk.yaml
   ```

3. Add the device in Home Assistant. It exposes three entities:
   - **Desk Height**: a sensor, in inches
   - **Desk Up**: a button
   - **Desk Down**: a button
