# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

Home Assistant config plus the ESPHome device projects that talk to it, kept in one repo so all the code is in one place.

- **Home Assistant** runs on `corellia` via `compose.yaml` (deployed to `/opt/stacks/homeassistant`). It uses host networking, but is always reached through Nginx Proxy Manager at `https://home.priddle.network`, never directly on port 8123.
- **ESPHome** projects live in `esphome/`. ESPHome is also installed on corellia in its own compose project, but it's rarely used; firmware is compiled and flashed locally on the Mac.
- **Zigbee2MQTT / Mosquitto** are installed on the server but are not part of this compose project or configured here, and nothing meaningful runs on them right now.

Hue lights and remotes are paired to the Hue bridge and come in through the `hue` integration.

## Commands

```bash
# Deploy: git pull on corellia (restarts containers only if compose.yaml changed; -r forces)
script/deploy [-r]

# Query the HA REST API (needs HA_URL and HA_TOKEN in .envrc)
script/ha search <pattern>
script/ha state <entity_id>

# Validate Home Assistant config (on corellia)
docker exec homeassistant hass --script check_config
```

## Investigating vs. changing

- Free to use: `script/ha` queries, read-only `ssh corellia ...` commands, and `script/deploy`.
- **Ask first** before changing anything on the server or in HA (service calls, UI/entity edits, container restarts beyond deploy, file edits on corellia) unless explicitly asked.

## Layout

### Home Assistant (`config/`, mounted to `/config`)
- `configuration.yaml` - Global config only: Lovelace resources/dashboards, HomeKit filter, package include
- `packages/` - Hand-written config, one file per feature (holiday, meeting, thermostat, vision, weather). Each file can mix domains (`template:`, `sensor:`, `automation:`, ...). New YAML config goes here.
- `automations.yaml`, `scripts.yaml`, `scenes.yaml` - Owned by the HA UI editors; don't hand-edit
- `dashboards/bubble_card.yaml` - Main dashboard (YAML mode, Bubble Card); `bubble_card/` holds Bubble Card modules
- `custom_components/emby_now_playing/` - Hand-written Emby integration (read-only media players per client + `sensor.emby_now_playing`); other `custom_components/` are HACS-managed and gitignored
- `www/emby-now-playing-card.js` - Lovelace card for `sensor.emby_now_playing`
- `themes/`, `blueprints/` - Themes and stock blueprints

### ESPHome (`esphome/`)
- `common/` - Shared packages: `base.yaml` (wifi, API, web server), `kauf-plug.yaml`, `hardware/` (board definitions)
- Single-file devices at the top level: `kauf-plug-*.yaml`, `speedtest-gauge.yaml`, `treemote.yaml`
- Larger projects in their own directories: `tab5-dashboard/`, `cyd-weather/`, `dot-matrix/`, `uplift-desk/` (some have their own README/CLAUDE.md and custom components)
- `tab5-dashboard/` - M5Stack Tab5 desk dashboard; reads `sensor.forecast_*`, `sensor.vision_next_event`, `light.*_all` and others, backed by `config/packages/vision.yaml`. Check it before renaming or removing entities.

### Secrets
- `config/secrets.yaml` - HA secrets, referenced with `!secret key_name`; gitignored, lives only on the server
- `esphome/secrets.yaml` - Real ESPHome secrets; gitignored (`secrets.yaml.example` documents the keys)
- `esphome/*/secrets.yaml` - Committed, but only one-line includes of the real (gitignored) secrets file so each project directory resolves `!secret`. Don't put real values in them.

## YAML Conventions

- Home Assistant uses Jinja2 templating in `{{ }}` blocks
- Use current syntax: `triggers:`/`trigger:`, `actions:`/`action:` (not `platform:`/`service:`)
- Give template entities a `unique_id`
