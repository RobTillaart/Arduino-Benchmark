# Change Log Arduino-Benchmark

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/)
and this project adheres to [Semantic Versioning](https://semver.org/).

## [1.0.1] - 2026-02-20

### Added
- **CSV output mode** — compile-time `#define CSV_OUTPUT 1` flag (default 0) in
  `BenchmarkHelpers.h` switches both sketches from human-readable Serial output to
  `label,value` CSV rows, one per metric; all 252 result prints across Part 1 and
  Part 2 are wrapped in `#if !CSV_OUTPUT` guards with matching `CSV_ROW` /
  `CSV_BLANK` calls so every board always emits the same fixed row count
- **`tools/import_results.py`** — Python script that reads the CSV Serial output,
  locates the board column in a versioned `.xlsx` spreadsheet (or creates a new
  one), matches each label to its spreadsheet row by name, and writes the numeric
  values automatically — no manual row lookup needed. Usage:
  ```
  # 1. Set #define CSV_OUTPUT 1 in BenchmarkHelpers.h, upload, copy Serial output
  python tools/import_results.py output.csv Part1_CoreBenchmarks_Results_v1.0.1.xlsx
  ```

### Fixed
- BLE scan now runs on ArduinoBLE-capable boards (Nano 33 BLE, RP2040 Connect,
  SAMD Nano 33 IoT, Portenta H7/C33, Giga R1, Wio Terminal) instead of printing
  a placeholder message
- `runTimedLoop` no longer divides by zero when `elapsedMicros` is 0; affected
  boards where the callback completed before `micros()` advanced (coarse timer
  resolution); `opsPerMs` now reports 0 instead of crashing or printing `inf`
- Fixed divide-by-zero on ten fixed-iteration timing variables (`addTime`,
  `faddTime`, `fmulTime`, `fdivTime`, `dmulTime`, `ddivTime`, `macTime`,
  `satTime`, `pwmTime`, `rampTime`); fast boards (ESP32 at 240 MHz, Teensy 4.1
  at 600 MHz) can complete these loops in under 1 µs, producing a 0 µs result
- Fixed ESP32 Arduino core v3 `ledcWrite()` calls using channel number instead
  of pin number; the v3 API changed the first argument from channel to pin, so
  PWM and analog-out benchmarks were driving GPIO0 instead of the correct pin
- Fixed EEPROM commit median calculation: was incorrectly reporting the upper
  middle value (`commitTimes[5]`) instead of the true median (average of
  `commitTimes[4]` and `commitTimes[5]`) from the sorted 10-sample array
- Fixed interrupt latency test using `INPUT_PULLDOWN` on SAMD boards (Zero,
  MKR series); SAMD does not support `INPUT_PULLDOWN` — added `ARDUINO_ARCH_SAMD`
  to the `INPUT_PULLUP` guard alongside AVR and ARC32

## [1.0.0] - 2026-02-16

First official release on the `v1.0.0` branch.

### Added
- Split benchmark into two parts: Part 1 (Core Benchmarks) and Part 2 (Platform Benchmarks)
- Benchmark versioning system (`BENCHMARK_VERSION` in BenchmarkHelpers.h)
- Version bump workflow documented in CONTRIBUTING.md
- `digitalWriteFast()` benchmark for direct register-write comparison
- `delay()` and `delayMicroseconds()` timing accuracy benchmarks
- Arduino Yun support with Bridge, WLAN, and WAN benchmarks
- Arduino Uno Q (Zephyr RTOS) support: flash info, die temperature, watchdog
- Comprehensive benchmarks for ESP32, RP2040, and Uno R4
- Versioned result spreadsheets (Part1 and Part2 `_Results_v1.0.0.xlsx`)
- BOARD_STATUS.md tracker for board compatibility
- CONTRIBUTING.md with guidelines and version bump workflow
- `reports/` directory with per-board serial output logs

### Fixed
- ESP32-C3 GPIO and touch benchmark issues
- ESP32 PWM performance (channel-based LEDC writes)
- Flash overflow on ATmega32U4 (`BOARD_SMALL_FLASH` gating)
- Serial output on Arduino Yun and other native USB boards
- Zephyr build issues (stack budget, sbrk, Kconfig guards)
- Stack depth test rewritten to discover actual limits before timing

### Changed
- Optimized flash usage for AVR boards (itoa over snprintf, trimmed strings)
- Scoped `BOARD_SMALL_FLASH` to Yun only, not all AVR boards

### Removed
- SD card functionality tests (present in v0.0.0, dropped during the Part 1/Part 2 split)

## [0.0.0] - 2025-01-29

Initial version on the `v0.0.0` branch.

### Added
- Single-file universal Arduino benchmark sketch
- Integer, floating-point, string, and 64-bit math benchmarks
- SRAM, EEPROM, and PSRAM speed tests
- Digital and analog I/O benchmarks
- Flash info, PWM, interrupt latency, and SPI tests
- WiFi throughput, BLE/Bluetooth benchmarks
- Cryptography benchmarks (SHA-1, AES, MD5)
- Multi-core, DAC, touch, and sleep benchmarks (ESP32)
- DMA and PIO benchmarks (RP2040)
- LED matrix benchmarks (Uno R4)
- SD card functionality tests
- Volatile accumulators and checksummed results for accuracy
- `runForAtLeastUs` / `runTimedLoop` timing helpers
- Jitter analysis (5-trial runs) for timing benchmarks
- `yield()` calls for RTOS boards (ESP32, ESP8266, RP2040)
