# Contributing to Arduino Universal Benchmark Suite

Thanks for your interest in contributing. This guide covers three main ways to help: reporting issues, contributing code, and adding board results to the comparison spreadsheet.

---

## Creating Issues

### Bug Reports

When a benchmark produces incorrect results, crashes, or fails to compile on a supported board:

1. **Title:** Short description — e.g., "I2C benchmark hangs on Nano 33 BLE"
2. **Board info:** Board name, variant, and Arduino core version
3. **Arduino IDE version**
4. **Which sketch:** Part 1, Part 2, or both
5. **What happened:** Include the Serial Monitor output (copy-paste the relevant section)
6. **What you expected:** e.g., "Benchmark should complete without hanging"
7. **Steps to reproduce:** Any non-obvious setup (wiring, library versions, board manager URL)

### Feature Requests

For new benchmarks or new board support:

1. **Describe the benchmark or board** you'd like to see added
2. **Explain why it's useful** — what does it measure that existing tests don't?
3. **Link to datasheets or documentation** if proposing hardware-specific tests
4. **Note any prerequisites** — special libraries, hardware peripherals, wiring

### Spreadsheet Data Issues

If you notice incorrect or missing data in the results spreadsheets:

1. Identify the **board name**, **test category**, and **test name**
2. Provide the **correct value** with Serial Monitor output as evidence
3. Note the **Arduino core version** and **IDE version** used

---

## Contributing Code

### General Guidelines

- All benchmark code must compile in the Arduino IDE with no external dependencies beyond board-specific platform libraries (exception: the Multiduino board detection block requires the Adafruit RTClib library)
- Use `volatile` accumulators to prevent compiler optimization of benchmark loops
- Use the timing helper in `BenchmarkHelpers.h` (`runTimedLoop`) for consistent measurements
- Call `yield()` on RTOS-based platforms (ESP32, ESP8266, RP2040) inside long-running loops to avoid watchdog resets
- Print results to Serial at 115200 baud in a format consistent with existing output
- Wrap board-specific code in `#if defined(...)` / `#endif` preprocessor guards

### Adding a New Benchmark

1. Decide if it belongs in **Part 1** (universal — runs on all boards) or **Part 2** (platform-specific)
2. Write a function following the naming pattern: `benchmarkFeatureName()`
3. Use `runTimedLoop()` for throughput tests or `micros()` for latency tests
4. Print results with `SERIAL_OUT.print()` / `SERIAL_OUT.println()` matching the existing output format, wrapped in `#if !CSV_OUTPUT` / `#endif`:
   ```cpp
   #if !CSV_OUTPUT
   SERIAL_OUT.print(F_STR("Test Name: "));
   SERIAL_OUT.print(result);
   SERIAL_OUT.println(F_STR(" ops/ms"));
   #endif
   CSV_ROW("Test Name (ops/ms)", result);
   ```
5. For every board-conditional result path that does **not** run the benchmark, emit a `CSV_BLANK` so all boards always produce the same number of CSV rows:
   ```cpp
   #if defined(ESP32)
     // ... benchmark ...
     CSV_ROW("Test Name (ops/ms)", result);
   #else
     CSV_BLANK("Test Name (ops/ms)");
   #endif
   ```
6. Call your function from `setup()` in the appropriate sketch (after the existing benchmarks)
7. Guard any hardware-specific code in `#if defined(...)` / `#endif` preprocessor conditionals
8. Test on at least one physical board before submitting

### Adding Support for a New Board

1. Add a detection block in the board identification section using the board's compiler macros:
   ```cpp
   #elif defined(YOUR_BOARD_MACRO)
     boardName = "Your Board Name";
   ```
2. Add any board-specific `#if defined(...)` blocks for features unique to that board
3. Verify Part 1 and Part 2 both compile and run correctly
4. Run the full suite and submit your results for the spreadsheet (see below)

### Pull Request Process

1. Fork the repository and create a branch from `v1.0.1`
2. Make your changes
3. Test on physical hardware — simulator results are not accepted
4. Include Serial Monitor output in the PR description showing your changes work
5. If you added a new board, include full benchmark results
6. Open a pull request with a clear description of what changed and why

---

## Bumping the Benchmark Version

Any change to the benchmark code that could affect results (new tests, changed methodology, bug fixes in timing, etc.) **requires a version bump**. Adding new boards and **board specific** tests should NOT trigger a version bump. This ensures results from different benchmark versions are never mixed.

### When to Bump

- Adding, removing, or reordering benchmarks
- Changing how a benchmark measures or reports results
- Fixing a bug that changes output values
- Changing timing helpers in `BenchmarkHelpers.h`

You do **not** need to bump the version for:

- Adding board detection macros (new `#define BOARD_NAME` entries)
- Documentation-only changes
- Changes to spreadsheets or `BOARD_STATUS.md`

### How to Bump

1. **Create a new branch** from `v1.0.1` for your changes — do **not** work directly on the `v1.0.1` branch or any existing results branch
2. **Update `BENCHMARK_VERSION`** in both `Part1_CoreBenchmarks/BenchmarkHelpers.h` and `Part2_PlatformBenchmarks/BenchmarkHelpers.h` (e.g. `"1.0.1"` → `"1.1.0"`)
3. **Copy the previous spreadsheets as-is**: Copy `Part1_CoreBenchmarks_Results_v<OLD_VERSION>.xlsx` to `Part1_CoreBenchmarks_Results_v<NEW_VERSION>.xlsx` and similarly for Part 2 — keep all existing board columns and result data intact. Do **not** modify or delete the previous version's spreadsheets. In each new spreadsheet, add a **"Version"** row immediately below the board name row; enter the benchmark version that produced each board's data (previous boards keep their old version number until they are re-tested under the new version)
4. **Copy `BOARD_STATUS.md`** as-is — keep the board list and all existing results. When boards are re-tested under the new version, their rows will be overwritten with the new results
5. **Do not overwrite the `v1.0.1` branch** — previous benchmark versions and their results must be preserved

### Version Numbering

Follow semantic versioning:

- **Major** (e.g. `1.0.0` → `2.0.0`): Large-scale changes — restructured test suite, changed output format, removed benchmarks
- **Minor** (e.g. `1.0.0` → `1.1.0`): New benchmarks added, methodology improvements
- **Patch** (e.g. `1.0.0` → `1.0.1`): Bug fixes that affect reported values

---

## Adding Boards to the Spreadsheet

Benchmark results are tracked in versioned spreadsheet pairs — one per benchmark version (e.g. `Part1_CoreBenchmarks_Results_v1.0.0.xlsx` and `Part2_PlatformBenchmarks_Results_v1.0.0.xlsx`). Each pair corresponds to a specific benchmark version as defined by `BENCHMARK_VERSION` in `BenchmarkHelpers.h`. When the benchmark version changes, a new pair of spreadsheets is created and results for that version are recorded there. To add a new board or update existing results:

### Running the Benchmarks

1. Open the Arduino IDE and select your board
2. Upload `Part1_CoreBenchmarks.ino` — open Serial Monitor at **115200 baud** and let it run to completion
3. Copy the full Serial Monitor output and save it
4. Upload `Part2_PlatformBenchmarks.ino` — repeat the process
5. Keep a record of:
   - Exact board name and revision
   - Arduino core/platform version (from Boards Manager)
   - Arduino IDE version
   - Any non-default settings (clock speed, flash mode, etc.)

**Tip — CSV mode for faster entry:** Set `#define CSV_OUTPUT 1` in `BenchmarkHelpers.h` before uploading. Copy the `label,value` Serial output to a `.csv` file, then run `python tools/import_results.py output.csv Part1_CoreBenchmarks_Results_v<VERSION>.xlsx` to populate the spreadsheet automatically. Remember to set `CSV_OUTPUT` back to `0` when done.

### Formatting Results

- Check the benchmark version printed in your Serial Monitor output (look for "Benchmark Version: X.Y.Z")
- Open the matching versioned spreadsheets (e.g. `Part1_CoreBenchmarks_Results_v1.0.0.xlsx` and `Part2_PlatformBenchmarks_Results_v1.0.0.xlsx`)
- Add a new column for your board, or update an existing column
- In the **"Version"** row (just below the board name row), enter the benchmark version printed in your Serial Monitor output (e.g. `1.0.0`)
- Fill in each row with the corresponding value from your Serial Monitor output
- Use the same units as existing entries (ops/ms, microseconds, etc.)
- Leave cells blank for tests that don't apply to your board (e.g., WiFi on a Nano)
- Do not round or modify raw values — enter them as printed

### Submitting Results

You can submit results in two ways:

1. **Pull request:** Add the column to the spreadsheet and open a PR
2. **Issue:** If you can't edit the spreadsheet, open an issue titled "Benchmark results: [Board Name]" and paste the full Serial Monitor output from both Part 1 and Part 2. Include the board and IDE version info. A maintainer will add the data.

---

## Questions?

Open an issue with the "question" label if anything is unclear.
