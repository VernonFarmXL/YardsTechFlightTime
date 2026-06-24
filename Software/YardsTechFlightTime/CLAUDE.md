# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project type

Embedded firmware for an ESP32-based RFID/radar flight-time/timing panel ("YardsTech FlightTime"), built with PlatformIO and the Arduino framework. All source lives in `src/`; there is no desktop app or backend service in this repo.

## Build and test commands

- Build (default env): `platformio run`
- Build a specific environment: `platformio run -e esp32dev` or `platformio run -e denky32`
- Upload to hardware: `platformio run -e esp32dev -t upload`
- Serial monitor: `platformio device monitor -b 115200`
- Debug: `platformio debug --environment esp32dev` (or the VS Code PlatformIO debug configuration)
- There are no unit tests in this repo; verification is done by building, flashing, and observing serial log output (`log_i`) and hardware behavior (LEDs, buzzer, radar readings).

## Environments

`platformio.ini` defines two boards:
- `esp32dev` — uses a pinned custom `pioarduino` Espressif32 platform release, `min_spiffs.csv` partition table, and `extra_script.py` (prepends the Windows xtensa toolchain path to `PATH` — only relevant on Windows dev machines).
- `denky32` — uses the standard `espressif32` platform, no extra scripts.

## Architecture

### Global state + FreeRTOS task model

There's no central "app" object. State is shared via `extern` globals declared in `src/YTGlobals.h` and defined in `src/YTGlobals.cpp` (connection type, mode flags, BT/Wifi credentials, EEPROM-backed settings cache, etc.). Each subsystem is a `.cpp`/`.h` pair exposing a `XxxSetup()` and usually a `StartXxxTask()` that spawns a FreeRTOS task (`xTaskCreate`) rather than being driven from the main `loop()`. Tasks loop on guard flags like `!OTAUpdating && !ExternalPoweredOn && !PoweringDown` and self-delete (`vTaskDelete(NULL)`) on exit. Priorities are the `Priority` enum in `YTGlobals.h` (`LOW_PRI`..`HIGHEST_PRI`).

`src/main.cpp` only: defines pin-to-task glue (LD2450 read task, activity LED task, BT comms LED task), the power-on `setup()` sequence, and an almost-empty `loop()` that just polls the power button and external-power state — all real work happens in the spawned tasks.

### Power-on / mode selection sequence (`src/main.cpp::setup()`)

On boot the firmware decides between three mutually exclusive modes based on how long the power button is held during startup, gated by `TIME_BETWEEN_MODES`/`QUARTER_TIME_BETWEEN_MODES` delays:
- **Normal mode** — `StartNormalTasks()` (activity LED, buzzer, battery, LD2450 radar read task).
- **Setup mode** (`SetupMode = true`) — button held through one extra interval; switches `CurrentConnectionType` to `CONN_ACCESS_POINT` and runs `StartSafeModeTasks()` (adds Wifi loop + commands task on top of LED/buzzer/battery).
- **Safe mode** (`SafeMode = true`) — button held through a second extra interval; same task set as setup mode.
- If externally powered with the button not pressed, the unit stays in a low-power background state (`StartExternalPoweredOnTasks()` — battery task only).

If the power button isn't pressed and there's no external power, the device goes back to sleep (`GoToSleep()`) immediately.

Several subsystems (BT master/client, Wifi, Commands) are currently `#define`d/commented out of the active build in `main.cpp` (`StartNormalTasks`, `StartSafeModeTasks`) — check whether a feature is actually wired into a `StartXxxTasks()` function before assuming it runs.

### Subsystem modules

- `src/LD2450.cpp`/`.h` — driver for the Hi-Link LD2450 24GHz FMCW radar sensor over `Serial2` (256000 baud, pins `RXD2=16`/`TXD2=17`). `LD2450Task` in `main.cpp` polls `ld2450.read()` and tracks per-target start/stop-of-movement state in `targetArray`/`targetArrayTimes` (used for flight-time-style timing of detected targets).
- `src/BluetoothSerial.cpp`/`.h` — local fork/custom implementation of Bluetooth Serial (not the stock ESP32 Arduino core one). Gated behind `LOCAL_BT_IMPLMENTATION` in `YTGlobals.h`.
- `src/YTBTMaster.*` / `src/YTBTClient.*` — Bluetooth master/client connection roles.
- `src/YTWifi.*` — Wifi connection logic (AP / client / wait-then-AP per `CONN_*` constants).
- `src/YTCommands.*` — command/response handling for connected clients (tag/EID reporting, wifi client request/response queues).
- `src/YTBuzz.*` — buzzer feedback patterns (`BuzzOn`/`BuzzOff`, `BUZZ_POWER_ON`/`BUZZ_POWER_OFF` patterns).
- `src/YTBatt.*` — battery voltage monitoring/charging state task.
- `src/YTOTA.*` — OTA firmware update handling (`OTAUpdating` flag gates other tasks while active).
- `src/Crc16.h` — CRC16 used for validating raw tag/radar data (`ValidCRC`).
- `src/iirFilter.h` — simple IIR filter helper, used for smoothing analog readings (battery/antenna voltage etc).

### Persistent settings

Settings are stored via the ESP32 `Preferences` library under the `"Settings"` namespace (NVS-backed). Key names are the `*EEAddr` string `#define`s in `YTGlobals.h` (e.g. `GUIDEEAddr`, `SSIDEEAddr`, `ConnectionTypeEEAddr`). `DataModNoEEAddr` stores a settings schema version (`CURRENT_DATA_MOD`); `FactoryReset(DataModNo)` runs on every boot and is responsible for migrating/resetting settings when the schema version changes. **Do not repurpose or remove existing `*EEAddr` key strings** without an intentional migration in `FactoryReset` — doing so silently breaks settings compatibility for devices already in the field.

### Connection type model

`ConnectionType`/`CurrentConnectionType` (persisted, see above) select between `CONN_WAIT_YT_THEN_AP`, `CONN_YT`, `CONN_ACCESS_POINT`, `CONN_EXT_NETWORK`, `CONN_BT_CLIENT`, `CONN_BT_MASTER` (`YTGlobals.h`). Setup mode forces `CONN_ACCESS_POINT` regardless of the persisted value.

### Pin map

All GPIO pin assignments are centralized as `#define`s at the top of `src/YTGlobals.h`. This is hardware-specific (board revision `HARDWARE_VERSION 200`); if wiring changes, update here and verify against `src/main.cpp` `setup()`/task usage.

## Conventions and constraints

- Debug logging uses ESP-IDF's `log_i(...)` (printf-style, no explicit `Serial.print` needed) gated by per-subsystem `#define`s at the top of `YTGlobals.h` (`MAIN_DEBUG`, `BATT_DEBUG`, `BUZZ_DEBUG`, `CMD_DEBUG`, `HDX_DEBUG`, `BT_DEBUG`, `TAG_FINDER_DEBUG`, `TRANSMIT_DEBUG`, `WIFI_DEBUG`). Enable the relevant one rather than adding ad-hoc `Serial.print` calls.
- `Serial` (115200) is debug/log output; `Serial2` (256000) is dedicated to the LD2450 radar — don't repurpose either for general comms without checking both consumers.
- Firmware version string is `FIRMWARE_VERSION` in `YTGlobals.h` — bump it when cutting a release build (compiled binaries are checked into `Program/` named by date, e.g. `Program/2025.11.15-200.bin`).
- Prefer minimal, targeted edits over broad refactors — this is hardware-coupled embedded code where task timing and pin behavior are easy to break with non-obvious side effects.
