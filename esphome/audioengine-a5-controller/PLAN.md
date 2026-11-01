# Audioengine A5+ Controller — Plan

An ESP32 that sits inside the left (powered) A5+ speaker, between the front
controls (knob, button, IR receiver, LED) and the speaker's main board. Home
Assistant gets real Power / Mute / Volume entities that always reflect the
speaker's actual state, and the physical controls and IR remote keep working.

Based on [`audioengine-a5-controller/`](audioengine-a5-controller/README.md)
(reference copy of the original project). We follow its approach, with:

- Pin assignments changed to avoid the ESP32's serial and strapping pins
  (needed for the USB mod; see [Pin map](#pin-map))
- The USB port mod: the speaker's rear USB-A port becomes the ESP32's USB port
  for flashing/logs without opening the speaker
- Config rewritten for current ESPHome and this repo's conventions
  (`common/base.yaml`, secrets, encryption)

## How it works

```
                 ┌──────────────── inside the A5+ ────────────────┐
 front controls  │                                                │
 ┌───────────┐   │   ┌─────────┐   ┌──────────────┐   ┌─────────┐ │
 │ knob      │───┼──▶│         │──▶│ level shifter│──▶│ A5 main │ │
 │ button    │   │   │  ESP32  │   │  3.3V ↔ 5V   │   │  board  │ │
 │ IR recv   │   │   │         │   └──────────────┘   │  (MCU)  │ │
 │ LED       │◀──┼───│         │◀──── 5V power ───────│ USB 5V  │ │
 └───────────┘   │   └────┬────┘                      └─────────┘ │
                 │        │ USB data                              │
                 │   rear USB-A port ◀── laptop (flash / logs)    │
                 └────────────────────────────────────────────────┘
                          │ Wi-Fi
                          ▼
                   Home Assistant
```

- **Controls side (3.3V, direct):** the ESP32 reads the knob, button and IR
  receiver and drives the front LED itself.
- **Speaker side (5V, via level shifter):** the ESP32 fakes knob turns, button
  presses and IR signals into the speaker's MCU. The A5 board has 5V pull-ups
  on these inputs, so they must go through the level shifter or they'll damage
  the ESP32.
- **State tracking:** the speaker has no way to report its state, so the ESP32
  keeps its own copy (power, mute, volume) and resyncs on boot by sending
  volume-down 55× (forces 0) and then stepping back up.

## Hardware

### Have

| Part | Use |
| --- | --- |
| ESP32 DevKit V1 (USB-C) | Controller |
| 4-ch bidirectional level shifter (BSS138, 12 pack) | ESP32 → A5 board signals. One needed; a second is spare for the optional LED-sense idea |
| 2 × 0.1µF capacitors | Debounce knob A/B lines |
| 100Ω resistor | Front LED current limit |
| 1kΩ resistor | Debug LED current limit |
| LED | Debug LED showing the A5's original LED output |
| 5×7 cm protoboard | Controller board |
| Multimeter | Verifying pinouts, voltages, continuity |

### Need to get

| Part | Why |
| --- | --- |
| JST XH 2.54mm 9-pin kit (male PCB header + pre-wired female pigtail) — *ordered* | Male header soldered to our board: the front-control cable plugs into it. Pigtail wires soldered to our board, its female housing plugs into the A5 board's header. No crimping needed. **Confirm pitch/pin count on the actual cable first** |
| Female pin headers (2 × 15-pin) | Socket the ESP32 so it's removable |
| USB-C cable with data lines (USB 2.0 is fine) to sacrifice | USB mod wiring from the devkit to the rear port |
| Schottky diode 1N5817 — *ordered*, see [Power](#power) | Stops the ESP32's USB power back-feeding the speaker's 5V rail |
| Hookup wire (22–26 AWG), heat shrink | Wiring |
| M3 standoffs + screws | Mounting the board inside the speaker |
| USB-A male ↔ USB-A male cable, + USB-C → USB-A **female** adapter for the Mac | Laptop → speaker's rear port after the mod |

## Pin map

The original project uses GPIO3 (UART0 RX) for the IR receiver and GPIO5/15
(strapping pins) for other signals. GPIO3 is wired to the USB-serial chip, so
once the USB mod is in, the IR receiver and the serial chip would fight over
that pin. We move everything onto pins with no boot or serial role.

| Signal | Original | **Ours** | Notes |
| --- | --- | --- | --- |
| **Front controls (3.3V, direct)** | | | |
| Knob A | GPIO5 | **GPIO32** | + 0.1µF to GND, internal pull-up |
| Knob B | GPIO18 | **GPIO33** | + 0.1µF to GND, internal pull-up |
| Knob button | GPIO19 | **GPIO25** | Internal pull-up |
| IR receiver out | GPIO3 | **GPIO26** | IR VCC from 3V3 |
| Front LED + | GPIO23 | **GPIO27** | Through 100Ω |
| **A5 board (via level shifter LV side)** | | | |
| Knob A | GPIO4 | **GPIO19** | Shifter ch 1 |
| Knob B | GPIO16 | **GPIO18** | Shifter ch 2 |
| Knob button | GPIO17 | **GPIO17** | Shifter ch 3 |
| IR signal | GPIO15 | **GPIO16** | Shifter ch 4 |
| **Other** | | | |
| Status LED | GPIO2 | GPIO2 | Onboard LED |

The front-control pins are all on one header row and the A5-side pins on the
other, which keeps the protoboard wiring short. Verify against the silkscreen
on your specific board before soldering; USB-C clones sometimes shuffle pins.

### A5 9-pin connector (from the original README — verify)

| Pin | Front cable (controls) | A5 board side |
| --- | --- | --- |
| 1 | Knob A | Knob A input |
| 2 | Knob B | Knob B input |
| 3 | Button | Button input |
| 4 | GND | GND (not needed, common GND via pin 6) |
| 5 | IR out | IR input |
| 6 | GND | GND |
| 7 | IR VCC | 5V (not used, ESP32 powers the IR receiver) |
| 8 | LED + | LED + output → 1kΩ → debug LED |
| 9 | LED − | LED − → debug LED |

## Power

The A5's rear USB port is fed from an internal 5V supply. We take that 5V for
the ESP32 (`VIN`/`5V` pin) and the level shifter's HV side, and repurpose the
USB port for data.

```
A5 5V rail ──▶|── ESP32 VIN ──┬── shifter HV
           Schottky           │
laptop VBUS ──▶|── (devkit's own diode, verify) ──┘
ESP32 3V3 ───── shifter LV, IR receiver VCC
GND (A5) ────── ESP32 GND, shifter GND, IR GND, knob GND
```

Two diodes keep the two 5V sources (speaker and laptop) from feeding each
other. The original author skipped the speaker-side diode and just switched
the speaker off before plugging in the laptop. A diode is cheap insurance.

**To check in Phase 0:** whether the USB-C devkit already has a diode between
the USB connector's VBUS and the `VIN` pin (multimeter diode mode, see below).

## USB port mod

1. Cut the sacrificial USB-C cable, leaving ~15 cm on the C end.
2. Identify the 4 wires with continuity mode, not by colour (VBUS, D−, D+,
   GND). Cheap cables don't always follow red/white/green/black.
3. Unsolder or cut the factory lead from the rear USB-A jack to the 5V supply
   (reuse that lead and connector for powering our board, like the original).
4. Solder the 4 wires to the jack's pins in matching order. USB-A pinout,
   looking into the port with the contacts on top: 1 VBUS, 2 D−, 3 D+, 4 GND.
5. Plug the C end into the devkit.

Result: speaker switched **off**, plug the Mac in through the C→A(female)
adapter and A↔A cable, and `esphome run` / `esphome logs` work over serial.
Day-to-day flashing still goes over Wi-Fi (OTA).

## Multimeter crash course (what we'll actually use)

| Mode | Symbol | Use here |
| --- | --- | --- |
| Continuity | 🔊 / `•)))` | Beeps if two points are connected. Mapping cable wires, checking solder joints, checking for shorts (VIN↔GND must **not** beep). Power off. |
| DC voltage | `V⎓` | Measuring the 5V rail and pin voltages. Black probe on GND, red on the point. Power on. |
| Diode | `→|` | Checking for the devkit's VBUS diode. ~0.2–0.4V one way, nothing the other way means there's a Schottky. |

Rule of thumb: continuity/diode/resistance with power **off**, voltage with
power **on**.

## Phases

Each phase ends with a checkpoint to photograph or screenshot.

### Phase 0 — Open up & verify

- Open the powered speaker; photograph the main board, the 9-pin cable and
  connector, the rear USB jack, and the mounting spot.
- Confirm connector pitch and pin count (JST XH 2.54mm, 9-pin?).
- With the speaker on: measure the USB 5V rail; measure each pin on the A5
  board side of the connector with the front cable unplugged. Expect ~5V on
  pins 1, 2, 3, 5 (pull-ups) and 7, and 0V on 4/6.
- Optional: measure pin 8 voltage when the LED is on, off and blinking (for the
  LED-sense idea).
- Check the devkit's VBUS → VIN diode.
- ✅ Checkpoint: photos plus a table of measured voltages.

### Phase 1 — Firmware on the bench

- Create `audioengine-a5.yaml` using repo conventions: `common/base.yaml`,
  `esp32:` block (not the deprecated `platform:` key), `ota: - platform:
  esphome`, `api` encryption key `audioengine_a5_api_encryption_key` in
  `secrets.yaml(.example)`, local `secrets.yaml` stub.
- Port the controller logic from the original, with:
  - New pin map
  - Volume `number` with `mode: slider` (no custom Lovelace plugin needed)
  - IR codes recaptured from the real remote with `dump: nec`. ESPHome changed
    NEC address/command encoding after the original was written, so its codes
    may not match.
- Flash over USB on the desk. Confirm Wi-Fi, HA discovery and entities.
- ✅ Checkpoint: HA device page screenshot.

### Phase 2 — Build the board

- Lay out the protoboard: ESP32 on headers, level shifter, two caps, two
  resistors, debug LED, two 9-pin JST connectors (to controls, to A5), power
  input, diode.
- Solder in stages, checking continuity and shorts after each stage.
- Power from USB on the desk and check 3V3 and 5V at the shifter.
- ✅ Checkpoint: board photo (top and bottom).

### Phase 3 — Test in the speaker (case open)

- Plug the board between the front cable and the A5 board, with power from the
  A5 5V lead.
- Test, in order: knob/button/IR read correctly (logs) → front LED → fake knob
  turns change volume → fake button toggles power → fake IR mute → boot resync.
- ✅ Checkpoint: logs + HA controls working.

### Phase 4 — USB mod & final install

- Wire the rear USB jack to the devkit (see above).
- Test serial flashing/logs through the rear port with the speaker off.
- Mount on standoffs, tidy cables, close up.
- ✅ Checkpoint: installed photo, HA dashboard card.

### Phase 5 — HA integration

- Add the device to a dashboard (Bubble Card).
- Optional: wrap Power/Mute/Volume in a template `media_player` (e.g. in
  `config/packages/`) for a nicer single entity.
- Automations: off-at-night idle timer (on-device like the original, or in HA).

## Optional ideas

- **LED sense:** read the A5's LED output (pin 8) through the spare shifter
  channel to get the speaker's *real* power/standby state instead of only
  trusting our own copy. Depends on Phase 0 voltage readings.
- Long-press preset volume and extra IR commands (in the original already).

## Diagrams to make (next step)

1. System block diagram (above, but prettier)
2. ESP32 pinout with our assignments highlighted
3. Front-controls wiring (knob, caps, IR, LED)
4. A5-side wiring through the level shifter
5. Power + USB mod
6. Protoboard layout (component placement, top and bottom views)

## Repo housekeeping

- `audioengine-a5-controller/` is a nested git clone. Before committing,
  either delete its `.git/` (keep it as a vendored reference) or leave the
  folder out of the repo. Otherwise git records it as an empty submodule.
