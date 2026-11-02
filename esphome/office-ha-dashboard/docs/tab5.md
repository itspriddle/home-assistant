# M5Stack Tab5 Hardware Reference

SKU: C145 (Tab5) / K145 (Tab5 Kit)

A portable smart-IoT terminal development device featuring ESP32-P4 dual-core MCU with rich peripheral integration.

## System Architecture

```mermaid
graph TB
    subgraph "Main Processing"
        P4[ESP32-P4<br/>RISC-V Dual-Core 400MHz]
        C6[ESP32-C6<br/>WiFi 6 / Thread / Zigbee]
    end

    subgraph "Memory"
        FLASH[16MB Flash]
        PSRAM[32MB PSRAM<br/>200MHz]
    end

    subgraph "Display & Input"
        LCD[5" IPS TFT<br/>1280x720]
        TOUCH[ST7123 Touch<br/>I2C 0x55]
        CAM[SC2356 Camera<br/>2MP MIPI-CSI]
    end

    subgraph "Audio"
        CODEC[ES8388 Codec]
        AEC[ES7210 AEC]
        MIC[Dual Mics]
        SPK[NS4150B<br/>1W Speaker]
    end

    subgraph "Sensors & Timing"
        IMU[BMI270<br/>6-Axis IMU]
        RTC[RX8130CE<br/>RTC]
    end

    subgraph "Power"
        INA[INA226<br/>Monitor]
        CHG[IP2326<br/>Charger]
        BAT[NP-F550<br/>7.4V 2000mAh]
    end

    P4 <--> FLASH
    P4 <--> PSRAM
    P4 <-->|SDIO| C6
    P4 <-->|MIPI-DSI| LCD
    P4 <-->|I2C| TOUCH
    P4 <-->|MIPI-CSI| CAM
    P4 <-->|I2S| CODEC
    P4 <-->|I2S| AEC
    CODEC --> SPK
    AEC <-- MIC
    P4 <-->|I2C| IMU
    P4 <-->|I2C| RTC
    P4 <-->|I2C| INA
    CHG --> BAT
```

## Core Specifications

| Component | Specification |
|-----------|--------------|
| Main MCU | ESP32-P4NRW32 - RISC-V 32-bit Dual-core @ 400MHz + LP core @ 40MHz |
| Wireless Module | ESP32-C6-MINI-1U |
| Flash | 16MB |
| PSRAM | 32MB @ 200MHz |
| WiFi | 2.4 GHz WiFi 6, Thread, Zigbee |
| Antenna | Built-in 3D antenna + 2x MMCX external ports |
| Operating Temp | 0 ~ 40°C |
| Dimensions | 128.0 x 80.0 x 12.0mm (Tab5) / 128.0 x 80.0 x 26.7mm (Kit) |
| Weight | 124.5g (Tab5) / 277.4g (Kit with battery) |

## Display System

| Parameter | Value |
|-----------|-------|
| Type | 5" IPS TFT |
| Resolution | 1280 x 720 (720P) |
| Interface | MIPI-DSI (2-lane) |
| Driver IC | ST7123 (integrated display + touch) |
| Backlight Control | GPIO22 (LEDA) |

### Display Pin Mapping

| ESP32-P4 | Function |
|----------|----------|
| G22 | Backlight Enable (LEDA) |
| DSI_CLKN/P | Clock differential pair |
| DSI_DATAN0/P | Data lane 0 |
| DSI_DATAN1/P | Data lane 1 |

### Touch Controller

| Parameter | Value |
|-----------|-------|
| IC | ST7123 (integrated) or GT911 (older units) |
| I2C Address | 0x55 (ST7123) / 0x14 (GT911) |
| Interrupt | G23 (TP_INT) |
| I2C Bus | G31 (SDA), G32 (SCL) |

> **Note**: As of October 14, 2025, Tab5 units use the integrated ST7123 driver. Older units with ILI9881C display driver and GT911 touch controller require updated M5Unified/M5GFX libraries.

## Camera

| Parameter | Value |
|-----------|-------|
| Sensor | SC2356 |
| Resolution | 2MP (1600 x 1200) |
| Interface | MIPI-CSI |

### Camera Pin Mapping

| ESP32-P4 | Function |
|----------|----------|
| G32 | CAM_SCL |
| G31 | CAM_SDA |
| G36 | CAM_MCLK |
| CSI_DATAP1/N1 | Data lane 1 |
| CSI_DATAP0/N0 | Data lane 0 |
| CSI_CLKP/N | Clock differential |

## Audio System

### ES8388 Audio Codec (I2C: 0x10)

Full-duplex audio codec for playback and recording.

| ESP32-P4 | ES8388 Function |
|----------|-----------------|
| G30 | MCLK |
| G27 | SCLK (I2S bit clock) |
| G26 | DSDIN (I2S data in) |
| G29 | LRCK (word select) |
| G32 | SCL |
| G31 | SDA |

### ES7210 AEC Frontend (I2C: 0x40)

Acoustic echo cancellation for dual-mic array.

| ESP32-P4 | ES7210 Function |
|----------|-----------------|
| G30 | MCLK |
| G27 | SCLK |
| G28 | ASDOUT (ADC data out) |
| G29 | LRCK |
| G32 | SCL |
| G31 | SDA |

### Speaker Amplifier

| Component | Value |
|-----------|-------|
| Amplifier | NS4150B |
| Output | 1W @ 8Ω |
| Enable | PI4IOE5V6408-1 P1 (SPK_EN) |

### Audio Output

- 3.5mm headphone jack with detection (HP_DET on PI4IOE5V6408-1 P7)

## I2C Bus Devices

All on the main I2C bus: G31 (SDA), G32 (SCL)

| Device | I2C Address | Function |
|--------|-------------|----------|
| ST7123 | 0x55 | Touch controller |
| INA226 | 0x41 | Power monitor |
| PI4IOE5V6408-1 | 0x43 | GPIO expander |
| PI4IOE5V6408-2 | 0x44 | GPIO expander |
| RX8130CE | 0x32 | Real-time clock |
| BMI270 | 0x68 | 6-axis IMU |
| ES8388 | 0x10 | Audio codec |
| ES7210 | 0x40 | AEC microphone frontend |

## GPIO Expanders

### PI4IOE5V6408-1 (0x43)

| Pin | Function | Description |
|-----|----------|-------------|
| P0 | RF_INT_EXT_SWITCH | Antenna select (LOW=internal, HIGH=external) |
| P1 | SPK_EN | Speaker amplifier enable |
| P2 | EXT5V_EN | 5V output enable for M5-Bus/HY2.0-4P |
| P4 | LCD_RST | Display reset |
| P5 | TP_RST | Touch controller reset |
| P6 | CAM_RST | Camera reset |
| P7 | HP_DET | Headphone detection (input) |

### PI4IOE5V6408-2 (0x44)

| Pin | Function | Description |
|-----|----------|-------------|
| P0 | WLAN_PWR_EN | ESP32-C6 power enable |
| P3 | USB5V_EN | USB-A 5V output enable |
| P4 | PWROFF_PLUSE | Power off control |
| P5 | nCHG_QC_EN | Charge QC mode enable |
| P6 | CHG_STAT_LED | Charging status LED |
| P7 | CHG_EN | Charge enable |

## Motion Sensor (BMI270)

| Parameter | Value |
|-----------|-------|
| Type | 6-axis (accelerometer + gyroscope) |
| I2C Address | 0x68 |
| Features | Interrupt wake-up support |

### Interrupt Wakeup

The BMI270 and RX8130CE interrupts route through a PMS150G-U06 microcontroller for wake functionality:

| Source | Signal |
|--------|--------|
| BMI270 (0x68) | INT → PMS150G PA6 (E_TRG) |
| RX8130CE | INT → PMS150G PA6 (E_TRG) |

## Real-Time Clock (RX8130CE)

| Parameter | Value |
|-----------|-------|
| I2C Address | 0x32 |
| Backup | 70000μF/3.3V supercapacitor (Φ4.8×1.4mm) |
| Features | Timed interrupt wake-up |

## Power Management

### Battery

| Parameter | Value |
|-----------|-------|
| Type | NP-F550 removable Li-ion |
| Voltage | 7.4V nominal (6.0V empty → 8.4V full) |
| Capacity | 2000mAh (14.8 Wh) |
| Runtime | ~6 hours (50% brightness, WiFi on) |

### Power Monitoring (INA226)

| Parameter | Value |
|-----------|-------|
| I2C Address | 0x41 |
| Function | Bus voltage and current monitoring |

### Charging (IP2326)

- Charging only works when device is powered on and initialized
- Controlled via PI4IOE5V6408-2 pins (CHG_EN, nCHG_QC_EN)

### Power Regulation

| Chip | Function |
|------|----------|
| MP4560 | Buck-boost converter |
| IP2326 | Charge management |

## Wireless Communication

### ESP32-C6 Connection

Connected to ESP32-P4 via SDIO:

| ESP32-P4 | ESP32-C6 |
|----------|----------|
| G11 | SDIO2_D0 |
| G10 | SDIO2_D1 |
| G9 | SDIO2_D2 |
| G8 | SDIO2_D3 |
| G13 | SDIO2_CMD |
| G12 | SDIO2_CK |
| G15 | RESET |
| G14 | IO2 |

### Antenna System

- Built-in 3D antenna (default)
- 2x MMCX external antenna ports
- Switch via PI4IOE5V6408-1 P0 (LOW=internal, HIGH=external)

## Expansion Interfaces

### RS485 (SIT3088)

| ESP32-P4 | Function |
|----------|----------|
| G21 | RX |
| G20 | TX |
| G34 | DIR |

- 120Ω switchable terminal resistor
- Power supply range: 6-24V

### HY2.0-4P (PORT.A)

| Pin | Color | Function | GPIO |
|-----|-------|----------|------|
| 1 | Black | GND | - |
| 2 | Red | 5V | - |
| 3 | Yellow | I/O | G53 |
| 4 | White | I/O | G54 |

### microSD Card

| ESP32-P4 | SPI Mode | SDIO Mode |
|----------|----------|-----------|
| G39 | MISO | DAT0 |
| G40 | - | DAT1 |
| G41 | - | DAT2 |
| G42 | CS | DAT3 |
| G43 | SCK | CLK |
| G44 | MOSI | CMD |

### M5-Bus (30-pin)

| Pin | Left | Right | Pin |
|-----|------|-------|-----|
| 1 | GND | G16 (GPIO) | 2 |
| 3 | GND | G17 (PB_IN) | 4 |
| 5 | GND | RST/EN | 6 |
| 7 | G18 (MOSI) | G45 (GPIO) | 8 |
| 9 | G19 (MISO) | G52 (PB_OUT) | 10 |
| 11 | G5 (SCK) | 3V3 | 12 |
| 13 | G38 (RXD0) | G37 (TXD0) | 14 |
| 15 | G7 (PC_RX) | G6 (PC_TX) | 16 |
| 17 | G31 (Int SDA) | G32 (Int SCL) | 18 |
| 19 | G3 (GPIO) | G4 (GPIO) | 20 |
| 21 | G2 (GPIO) | G48 (GPIO) | 22 |
| 23 | G47 (GPIO) | G35 (GPIO) | 24 |
| 25 | HVIN | G51 (GPIO) | 26 |
| 27 | HVIN | 5V | 28 |
| 29 | HVIN | BAT | 30 |

### STAMP Expansion Pads

Reserved pads for expansion modules:
- Cat.M
- NB-IoT
- LoRaWAN

## Physical Features

- 1/4"-20 tripod mount nut on side
- M3 mounting holes
- Reset/Boot button (hold ~2s for download mode)
- Power button (single press = on, double press = off)

## USB Ports

| Port | Type | Function |
|------|------|----------|
| USB-C | USB 2.0 OTG | Programming, data, power input |
| USB-A | Host | Peripherals (mouse, keyboard, etc.) |

## Development Platforms

- UiFlow2 (graphical programming)
- Arduino IDE (with M5Unified/M5GFX libraries)
- ESP-IDF
- PlatformIO

### PlatformIO Configuration

```ini
[env:esp32p4_pioarduino]
platform = https://github.com/pioarduino/platform-espressif32.git#54.03.21
upload_speed = 1500000
monitor_speed = 115200
build_type = debug
framework = arduino
board = esp32-p4-evboard
board_build.mcu = esp32p4
board_build.flash_mode = qio
build_flags =
    -DBOARD_HAS_PSRAM
    -DCORE_DEBUG_LEVEL=5
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DARDUINO_USB_MODE=1
lib_deps =
    https://github.com/M5Stack/M5Unified.git
    https://github.com/M5Stack/M5GFX.git
```

## Important Notes

### Power Supply
- Perform shutdown before disconnecting power or replacing battery
- If power is disconnected directly, wait 5 seconds before powering on (IMU may fail to initialize otherwise)

### Charging
- Device must be powered on and initialized to charge
- Cannot charge while device is off

### Download Mode
1. Connect USB or have battery installed
2. Press and hold reset button (~2 seconds)
3. Wait for internal green LED to flash rapidly
4. Release button - device enters download mode

## Version History

| Date | Change |
|------|--------|
| October 14, 2025 | Screen driver changed from ILI9881C + GT911 to integrated ST7123 |
| May 9, 2025 | First product release |
