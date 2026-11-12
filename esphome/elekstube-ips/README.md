# Elekstube IPS Clock

- 4 screens, 4 buttons. Hardware notes: [docs/hardware.md](docs/hardware.md)
- Board config: `../common/hardware/elekstube-ips.yaml`
- Rendering, modes and animation: `components/elekstube_ui`
- Settings are HA entities (Mode, Face, Transition, 24 Hour, Seconds Bar,
  Display, Brightness); the buttons act on those entities

## Buttons

| Button | Tap | Hold |
|---|---|---|
| POWER (right) | next mode | back to Clock |
| MODE | Clock: show the date for 5 s | toggle 12/24 h |
| LEFT / RIGHT | previous / next face | – |

## Modes

- Clock mode, default
    - font faces (Oswald, Fredoka gradient, Orbitron), switchable on device
    - 12/24hr toggle
    - keep leading 0s, `0130` not ` 130`
    - seconds bar across all four panels
    - digit roll / fade transitions
    - status pip on the right panel: amber = HA API down, red = no Wi-Fi
- Date mode: `MM DD`, month and weekday under the inner panels
- Weather mode (todo)
    - Icon
    - Temp
    - buttons
        1. show weekly?
        2. ??
        3. ??
        4. Next mode
- Network stats mode (todo)
    - last speedtest
    - dns1/dns2 queries
    - connected client count
