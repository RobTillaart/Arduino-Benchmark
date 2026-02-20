# Arduino Universal Benchmark Suite

A comprehensive benchmarking framework for Arduino-compatible boards. The goal is to support every Arduino-compatible board — upload a sketch, open Serial Monitor, and get detailed performance metrics for any board you have on hand.

## What Is This?

This is a standardized benchmark suite that measures the real-world performance of Arduino-compatible boards. It runs a set of CPU, memory, I/O, and platform-specific tests, then prints the results over Serial so you can compare boards side by side.

The suite is split into two sketches. Part 1 covers core benchmarks that apply to any board (integer math, floating-point, memory, digital/analog I/O). Part 2 covers platform-specific features like WiFi, BLE, cryptography, multi-core, and other capabilities that vary from board to board. Results are collected in versioned spreadsheets for easy comparison.

Each benchmark release has a version number (e.g. `1.0.0`) defined in `BenchmarkHelpers.h` and printed in the Serial Monitor output. When the benchmark code changes in ways that affect results, the version is bumped and a new pair of spreadsheets is created. This means you can see exactly which version of the benchmark produced each result, and older results remain valid as long as the benchmark version hasn't changed. Any code change that affects benchmark results **must** bump the version, copy the previous spreadsheets as-is (renaming them to the new version), reset `BOARD_STATUS.md`, and be done on a new branch — never overwrite the `v1.0.0` branch. See [CONTRIBUTING.md](CONTRIBUTING.md#bumping-the-benchmark-version) for the full process.

Boards are detected automatically at compile time using preprocessor macros — no manual configuration needed. If your board isn't detected yet, it's straightforward to add (see [CONTRIBUTING.md](CONTRIBUTING.md)).

## Repository Structure

```
├── Part1_CoreBenchmarks/
│   ├── Part1_CoreBenchmarks.ino    # Core performance tests
│   └── BenchmarkHelpers.h          # Shared timing utilities + benchmark version
├── Part2_PlatformBenchmarks/
│   ├── Part2_PlatformBenchmarks.ino # Platform-specific tests
│   └── BenchmarkHelpers.h          # Shared timing utilities + benchmark version
├── tools/
│   └── import_results.py           # CSV-to-spreadsheet import script
├── Part1_CoreBenchmarks_Results_v1.0.1.xlsx  # Core results (benchmark v1.0.1)
├── Part2_PlatformBenchmarks_Results_v1.0.1.xlsx # Platform results (benchmark v1.0.1)
├── reports/                          # Raw Serial Monitor output per board
├── BOARD_STATUS.md                   # Board testing tracker (status + benchmark version)
├── CHANGELOG.md                      # Version history and notable changes
├── COMPILE_LIST.md                   # Arduino AVR board compile identifiers reference
├── CONTRIBUTING.md                   # Guide for contributors
└── CONTRIBUTORS.md                   # List of contributors
```

## What Gets Benchmarked

### Part 1 — Core Benchmarks

Tests that run on every board:

- **CPU:** Integer ops, floating-point ops, string ops, DSP-enhanced operations including 64-bit math (on DSP/FPU-capable boards: Uno Q, Cortex-M33/M7, Teensy 4.x)
- **Memory:** SRAM read/write speed, EEPROM read/write/erase, PSRAM (if present)
- **I/O:** Digital read/write (API and direct register), analog read/write
- **Hardware:** Flash info, PWM, interrupt latency, SPI communication

### Part 2 — Platform Benchmarks

Advanced and board-specific tests:

- **CPU stress testing** with temperature tracking
- **Communication:** I2C (Wire)
- **Connectivity:** WiFi throughput, BLE/Bluetooth, LED matrix (Uno R4)
- **Cryptography:** SHA-1 (RP2040), AES/MD5/hardware RNG (ESP32), hardware-accelerated where available
- **Multi-core:** Dual-core performance (ESP32, Portenta H7)
- **Board-specific:** ESP32 DAC/touch/sleep, RP2040 DMA/PIO, Yun Bridge, and more

## How to Use

### Human-readable output (default)

1. Open **Arduino IDE**
2. Select your board and port
3. Open `Part1_CoreBenchmarks/Part1_CoreBenchmarks.ino`
4. Upload to your board
5. Open **Serial Monitor** at **115200 baud**
6. Wait for all tests to complete
7. Repeat with `Part2_PlatformBenchmarks/Part2_PlatformBenchmarks.ino`
8. Save the full Serial Monitor output to `reports/<board>-<version>.txt`
9. Record Part 1 results in `Part1_CoreBenchmarks_Results_v<BENCHMARK_VERSION>.xlsx` and Part 2 results in `Part2_PlatformBenchmarks_Results_v<BENCHMARK_VERSION>.xlsx` (the benchmark version is printed in the Serial Monitor output)

Results are printed as operations per millisecond (ops/ms), timing in microseconds, and other metrics depending on the test.

### CSV output + automatic spreadsheet import

For faster data entry, set `#define CSV_OUTPUT 1` in `BenchmarkHelpers.h` before uploading. The sketch will emit `label,value` rows instead of the human-readable output. Copy the Serial Monitor output to a `.csv` file, then run:

```sh
python tools/import_results.py output.csv Part1_CoreBenchmarks_Results_v1.0.1.xlsx
```

The script finds the board column by name (or creates a new one), matches each label to its spreadsheet row, and writes all numeric values automatically. Repeat for Part 2 with the Part 2 `.csv` and spreadsheet. Remember to set `CSV_OUTPUT` back to `0` when you want the normal output.

See [BOARD_STATUS.md](BOARD_STATUS.md) for which boards have been tested and which still need results.

## Hardware Requirements

- **Arduino IDE 2.x** recommended — some boards in this suite require IDE 2.x features
- **Board Manager URLs** — third-party boards (ESP32, RP2040, Teensy, STM32, etc.) require their respective Board Manager URLs to be added in Arduino IDE preferences before the board can be installed
- **External library** — the Multiduino board requires the [Adafruit RTClib](https://github.com/adafruit/RTClib) library (`RTClib.h`); all other boards use only platform-bundled libraries

> **Compile list note:** All boards listed as test-compiled in [COMPILE_LIST.md](COMPILE_LIST.md) were compiled using Arduino IDE **2.3.7** with the latest available board core versions at the time. No guarantees are made that they will compile on any other IDE or core version.

## Measurement Methodology

- **Volatile accumulators** prevent the compiler from optimizing away operations
- **Checksummed results** verify actual execution
- **Minimum-duration loops** ensure stable timing (`runTimedLoop`)
- **Register-level access** measured alongside API calls to quantify abstraction overhead
- **Jitter analysis** (5-trial runs) for timing precision benchmarks
- **yield() calls** on RTOS-based boards (ESP32, ESP8266, RP2040) to prevent watchdog resets

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on creating issues, submitting code, and adding new boards to the spreadsheet. See [CONTRIBUTORS.md](CONTRIBUTORS.md) for a list of contributors.

## License

MIT License — see [LICENSE](LICENSE) for details.
