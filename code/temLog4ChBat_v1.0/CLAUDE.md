# CLAUDE.md — temLog4ChBat v1.0
> ESP32-C6 WROOM-1 · ESP-IDF 5.2.2 · Comet Instrument · CD Industrial Block Library

## Project Locations

| Item | Path |
|---|---|
| Firmware (this project) | `F:\oneDriveBkup\ProjectGit\CometInstrument\temp4chEsp32C6_24Aug26\code\temLog4ChBat_v1.0\` |
| GitHub (firmware) | `https://github.com/brajeshChoudhary3854/temp4chEsp32C6_24Aug26` → `code/temLog4ChBat_v1.0/` |
| Product plan | `F:\oneDriveBkup\ProjectGit\cdProduct\cdProduct-blocks\CometInstrument\temLog4ChBat\v1.0\` |
| GitHub (product plan) | `https://github.com/brajeshChoudhary3854/cdProduct-blocks` → `CometInstrument/temLog4ChBat/v1.0/` |
| SW blocks repo (local) | `F:\oneDriveBkup\ProjectGit\cdProduct\cd-sw-blocks\ESP32_C6\` |
| HW blocks repo (local) | `F:\oneDriveBkup\ProjectGit\cdProduct\cd-hw-blocks\ESP32_C6\` |

## What This Project Is

4-channel temperature data logger for **Comet Instrument**.  
Reads DS18B20 sensors on 4 separate GPIOs every 60 seconds.  
Timestamps each reading using DS3231 RTC over I2C.  
Prints CSV-formatted log to serial monitor.  
Future phases: NVS flash storage + WiFi web dashboard.

## Hardware

| Component | Part | Interface |
|---|---|---|
| MCU | ESP32-C6 WROOM-1 | — |
| Temp sensor ×4 | DS18B20 | 1-Wire (one per GPIO) |
| RTC | DS3231 | I2C (0x68) |
| Battery | Li-Ion cell | TP4056 charger via USB-C |

## Pin Assignment

| Signal | GPIO | Notes |
|---|---|---|
| DS18B20 CH1 | GPIO 4  | 4.7 kΩ pull-up to 3.3 V |
| DS18B20 CH2 | GPIO 5  | 4.7 kΩ pull-up to 3.3 V |
| DS18B20 CH3 | GPIO 10 | 4.7 kΩ pull-up to 3.3 V |
| DS18B20 CH4 | GPIO 11 | 4.7 kΩ pull-up to 3.3 V |
| DS3231 SDA  | GPIO 15 | 4.7 kΩ pull-up to 3.3 V |
| DS3231 SCL  | GPIO 3  | 4.7 kΩ pull-up to 3.3 V |

All pin assignments are in `main/config.h` — change there only.

## SW Blocks Used (from cd-sw-blocks)

| Block | Version | Status | Source files |
|---|---|---|---|
| DS18B20_multisensor | v1.0 | EXISTING | `onewire.h/c`, `ds18b20.h/c`, `sensor_manager.h/c` |
| DS3231_RTC | v1.0 | EXISTING | `ds3231.h/c` |
| NVS_Logger | v1.0 | NEW (Phase 2) | not yet integrated |
| WiFi_WebServer | v1.0 | NEW (Phase 2) | not yet integrated |

## HW Blocks Used (from cd-hw-blocks)

| Block | Version | Status |
|---|---|---|
| DS18B20_4ch | v1.0 | NEW — schematic pending |
| DS3231_RTC | v1.0 | NEW — schematic pending |
| LiIon_Battery | v1.0 | NEW — schematic pending |

## File Structure

```
temLog4ChBat_v1.0/
├── CMakeLists.txt              top-level ESP-IDF project file
├── CLAUDE.md                   this file
├── .gitignore
├── .vscode/
│   └── settings.json           target=esp32c6, IDF=5.2.2
└── main/
    ├── CMakeLists.txt          component registration
    ├── config.h                ALL pins + intervals defined here
    ├── main.c                  app_main: init + 60s log loop
    ├── onewire.h / onewire.c   1-wire bus driver (bit-bang)
    ├── ds18b20.h / ds18b20.c   DS18B20 sensor driver
    ├── sensor_manager.h / .c   4-channel manager with auto-retry
    ├── ds3231.h / ds3231.c     DS3231 RTC driver
```

## Serial Output Format

```
[00001] 2026-09-01 10:30:00 | CH1:  25.25°C  CH2:  26.50°C  CH3: --N/A--  CH4:  24.75°C
```
- `--N/A--` = sensor not connected or CRC error (auto-retries every 15 s)
- Log index resets on reboot

## Build & Flash

```bash
# Set target (first time only)
idf.py set-target esp32c6

# Build
idf.py build

# Flash + monitor
idf.py -p COMx flash monitor
```

ESP-IDF version: **5.2.2** (set in `.vscode/settings.json`)

## Development Phases

| Phase | Description | Status |
|---|---|---|
| P1 (current) | DS18B20 + DS3231 — serial log output | In progress |
| P2 | Add NVS_Logger — store readings in flash | Pending |
| P3 | Add WiFi_WebServer — live dashboard + CSV download | Pending |
| P4 | Battery ADC monitor — show % on dashboard | Pending |

## Conventions

- ESP-IDF 5.2.2, target `esp32c6`
- All functions return `esp_err_t`
- No hardcoded GPIO numbers — always use `config.h`
- `ESP_LOGI` / `ESP_LOGW` / `ESP_LOGE` for all logging
- No code comments unless the WHY is non-obvious

## Updating SW Blocks (when you modify a driver here)

If you improve `ds18b20.c`, `ds3231.c`, `onewire.c`, or `sensor_manager.c`,
copy the updated file back to the block library and push:

```
cd-sw-blocks\ESP32_C6\DS18B20_multisensor\DS18B20_multisensorv1.0\main\
cd-sw-blocks\ESP32_C6\DS3231_RTC\DS3231_RTC_v1.0\main\
```
