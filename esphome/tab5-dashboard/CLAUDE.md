# CLAUDE.md

Project-specific instructions for the M5Stack Tab5 Home Assistant dashboard.

## Project Overview

ESPHome-based Home Assistant desk display on the M5Stack Tab5, landscape
1280x720 (USB port on the left). Three screens: Home (clock, weather, room
temps, status tiles), Lights (Hue rooms + scenes), Climate (thermostat dial).

YAML handles hardware and Home Assistant wiring; the UI is plain LVGL 9
built in C++ by the local `dashboard` component.

## Project Structure

```
tab5-dashboard/
├── tab5-dashboard.yaml      # Entry point: packages, lvgl, dashboard, fonts
├── packages/
│   ├── device.yaml          # Tab5 hardware: SoC, display, touch, backlight, RTC, battery
│   ├── network.yaml         # WiFi, HA API, OTA, web server, screenshot endpoint
│   ├── audio.yaml           # ES8388 codec, speaker, boot sound
│   └── homeassistant.yaml   # HA entities -> dashboard setters; dashboard triggers -> HA actions
├── components/
│   ├── dashboard/           # The UI (C++), see "Dashboard" below
│   └── lvgl_screenshot/     # Custom component: GET /screenshot returns a BMP
├── script/
│   ├── screenshot           # Capture display to screenshots/ (PNG)
│   └── ha                   # Query the Home Assistant REST API
├── docs/                    # Hardware reference (tab5.md, Tab5.pdf)
├── secrets.yaml             # Stub that includes the repo-root secrets.yaml
└── boot.flac                # Boot sound
```

Paths inside packages (e.g. `boot.flac`, `components`) resolve relative to
`tab5-dashboard.yaml`, not the package file.

Packages are merged into `tab5-dashboard.yaml`; lists concatenate, so `esphome.on_boot`
is written as a list in each package that needs it.

## Dashboard

`components/dashboard/`:

- `model.h/.cpp` — all UI state, plus the static room / Hue scene list.
- `dashboard.h/.cpp` — the `Dashboard` component. Input setters
  (`set_room_temperature`, `set_light_on`, ...) are called from
  `packages/homeassistant.yaml`; they update the model and set dirty flags.
  Output actions (`toggle_light`, `activate_scene`, `set_setpoint`, ...) are
  called by screens: they update the model optimistically, briefly ignore HA
  echoes (`hold_until`), debounce where needed, and fire YAML triggers
  (`on_light_toggle`, `on_scene`, `on_climate_temperature`, ...).
- `screen.h` — `Screen` base: `build()` once at boot, `update(model, dirty)`
  when visible state changes. `ScreenId` + `dash->show()` for navigation;
  idle timeout returns to Home.
- `home_screen`, `lights_screen`, `climate_screen`, `nav_rail` — the screens.
- `theme.h/.cpp` — palette, MDI codepoints, font roles, widget helpers. The
  helpers call `lv_obj_remove_style_all`, so styling is fully explicit.

Fonts are declared in `tab5-dashboard.yaml` and mapped to roles in `dashboard: fonts:`.
The big numeric fonts (`large`, `display`, `clock`) only contain the glyphs
listed for them. A new MDI icon needs its codepoint added to `theme.h` and to
the icon fonts' `glyphs:`.

Because widgets are created in C++, `__init__.py` enables the LVGL features
in use (`add_lv_use`); add to that list when using a new widget type.

Design reference: the "Vision UI Directions" canvas, round 2
(https://claude.ai/artifact/6CpeSoaGhb3CR8PPhDZ3eh).

## Commands

```bash
make compile     # Compile firmware
make upload      # Upload OTA to vision.local
make run         # Compile, upload, and show logs
make logs        # View device logs
make validate    # Validate config without compiling
make clean       # Clean build files

./script/screenshot          # Save current display to screenshots/ (needs WEB_* in .envrc)
./script/ha search <pattern> # Find HA entities (needs .envrc, see .envrc.example)
```

Use `./script/screenshot` to verify layout changes without physical access.

## Hardware: M5Stack Tab5

| Component | Details |
|-----------|---------|
| Main MCU | ESP32-P4 (RISC-V dual-core 400MHz) |
| WiFi | ESP32-C6 via SDIO (`esp32_hosted`) |
| Flash/PSRAM | 16MB Flash, 32MB PSRAM @ 200MHz |
| Display | 5" IPS, 720x1280 portrait, MIPI-DSI (`mipi_dsi` model `M5STACK-TAB5-ST7123`) |
| Touch | ST7123, I2C 0x55 |
| Battery | NP-F550, 2S Li-ion (6.0V empty, 8.4V full) |

### I2C Bus (GPIO31 SDA, GPIO32 SCL @ 400kHz)
| Device | Address | Purpose |
|--------|---------|---------|
| ST7123 | 0x55 | Touch controller |
| INA226 | 0x41 | Battery voltage/current |
| PI4IOE5V6408 #1 | 0x43 | GPIO expander |
| PI4IOE5V6408 #2 | 0x44 | GPIO expander |
| RX8130CE | 0x32 | RTC |
| BMI270 | 0x68 | IMU (unused) |
| ES8388 | 0x10 | Audio codec |
| ES7210 | 0x40 | Mic frontend (unused) |

### GPIO Expanders
- **0x43:** P1 speaker enable, P4 LCD reset, P5 touch reset
- **0x44:** P0 WiFi power, P6 charging status (input), P7 charge enable

### Other Pins
- Backlight PWM: GPIO22
- Touch interrupt: GPIO23
- I2S: LRCLK GPIO29, BCLK GPIO27, MCLK GPIO30, DOUT GPIO26

### Unused Hardware
Camera (SC2356, MIPI-CSI), BMI270 IMU, RS485, microSD, USB-A host, microphones.

## External Components

All hardware drivers (`mipi_dsi`, `st7123`, `rx8130`) are upstream in
ESPHome. The only external components are the local ones in `components/`.

## LVGL Notes

- ESPHome bundles LVGL 9 (9.5 as of ESPHome 2026.10). LVGL 8 names still
  compile through LVGL's compatibility layer but are deprecated; use the
  v9 API in new code.
- The UI must not block on Home Assistant. Render immediately with
  placeholders ("--°") and update as data arrives.
- Rotation is `rotation: 270` on `lvgl:` (ESPHome rejects it on the display
  when LVGL is used). LVGL rotates drawing and touch points in software, so
  the touchscreen has no `transform`.
- LVGL is not thread-safe: only touch it from the main loop. The screenshot
  endpoint's HTTP handler hands off to the component's `loop()`, which uses
  `lv_snapshot_take` (already landscape).
- Backlight is held off until `on_boot` priority -100 so the panel doesn't
  flash garbage before LVGL draws.

## Time

- Home Assistant time when the API is connected; each sync writes to the RX8130.
- RX8130 (`rtc_time`) keeps time across reboots / without HA; UI reads from it.

## Home Assistant

- The ESPHome integration for Vision must have "Allow the device to perform
  Home Assistant actions" enabled, or taps do nothing.
- HA-side template config the dashboard expects (`sensor.vision_next_event`)
  lives in `config/packages/vision.yaml` at the repo root.
