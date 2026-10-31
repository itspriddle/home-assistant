# homeassistant

My Home Assistant config and the ESPHome devices that talk to it, kept in one
repo. This is a personal setup, not a template, so it's tied to my hardware,
entities, and network.

## Home Assistant

Runs in Docker on my home server (`compose.yaml`), behind Nginx Proxy Manager.
Hue lights and remotes come in through the Hue bridge.

- `config/configuration.yaml` - Global config: dashboards, Lovelace resources,
  HomeKit filter, and the package include
- `config/packages/` - Hand-written config, one file per feature:
  - `holiday.yaml` - Holiday plug schedule
  - `meeting.yaml` - "In a meeting" sensor from work calendars and Mac cameras
  - `thermostat.yaml` - HVAC state and daily heating/cooling runtime
  - `vision.yaml` - Next calendar event for the Tab5 dashboard
  - `weather.yaml` - Daily forecast sensors for the ESPHome displays
- `config/dashboards/bubble_card.yaml` - Main dashboard, built with
  [Bubble Card](https://github.com/Clooos/Bubble-Card)
- `config/custom_components/emby_now_playing/` - Custom Emby integration: a
  read-only media player per client plus a "now playing" sensor
- `config/www/emby-now-playing-card.js` - Lovelace card for that sensor
- `config/automations.yaml`, `scripts.yaml`, `scenes.yaml` - Managed by the HA
  UI editors

## ESPHome devices

Firmware lives in `esphome/`, with shared packages in `esphome/common/`.

- [`tab5-dashboard/`](esphome/tab5-dashboard/) - M5Stack Tab5 desk dashboard
  (clock, weather, calendar, lights, thermostat), with a custom LVGL UI in C++
- [`uplift-desk/`](esphome/uplift-desk/) - Controller for an UPLIFT v2
  standing desk, with a 3D-printed enclosure
- [`cyd-weather/`](esphome/cyd-weather/) - Weather display on a "Cheap Yellow
  Display" ESP32 board
- [`dot-matrix/`](esphome/dot-matrix/) - MAX7219 LED dot-matrix displays
- `speedtest-gauge.yaml` - Small Waveshare display showing internet speed test
  results
- `treemote.yaml` - Remote control for the Christmas tree lights
- `kauf-plug-*.yaml` - Kauf smart plugs (rack, rack light, 3D printer)

## Scripts

- `script/deploy` - Pull the latest config on the server
- `script/ha` - Query the Home Assistant REST API
- `script/update-manifest` - Generate a list of installed integrations

## License

[MIT](LICENSE)
