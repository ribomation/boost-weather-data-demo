# Weather data with Boost — demo

Runnable companion code for the article
[*Väderdata med Boost*](https://www.ribomation.se/blog/2026/vaderdata-med-boost/) (in Swedish),
part two of the Boost series that started with
[*Kom igång med Boost*](https://www.ribomation.se/blog/2026/kom-igang-med-boost/).

## Goal

NOAA publishes daily weather observations from about 12 000 stations world-wide as one
archive per year, the [Global Summary of the Day](https://www.ncei.noaa.gov/data/global-summary-of-the-day/archive/)
(GSOD): `<year>.tar.gz`, containing one CSV file per station. For 2024 that is close to
900 MB and four million observations.

The program answers a simple question — *which stations had the warmest and the coldest
mean temperature of the year?* — without unpacking anything to disk or holding the data
in memory. Everything is processed as one stream:

```
HTTPS (Beast) → gzip (Iostreams) → tar (own TarReader) → CSV (Tokenizer) → statistics (Accumulators)
```

The program is built in four steps, `version-1` … `version-4`, each a complete and
buildable project that adds to the previous one.

## Contents

| Directory | What it adds | Boost libraries |
|---|---|---|
| [`version-1`](version-1) | Settings from an INI file and the command line | Property_tree, Program_options |
| [`version-2`](version-2) | `http_source`: an Iostreams source on top of an HTTPS connection; download and local cache of an archive | Beast, Asio, URL, Iostreams |
| [`version-3`](version-3) | The reading pipeline: gzip decompression, `TarReader`, `CSVReader` and `Observation` | Iostreams, Tokenizer, String_algo |
| [`version-4`](version-4) | `byte_counter` (a 64-bit counting filter), `Aggregator` with per-station statistics and ranking, `Reporter` | Iostreams, Accumulators |

Each version builds three targets: a static library `weather-data-core-vN`, the program
`weather-data-vN` and a Catch2 test suite `unit-test-vN`.

| File | Purpose |
|---|---|
| [`CMakeLists.txt`](CMakeLists.txt) | Top-level project; finds Boost, OpenSSL and Catch2 and adds all four versions |
| [`CMakePresets.json`](CMakePresets.json) | `debug` and `release` presets (Ninja) |
| [`weather-data.ini`](weather-data.ini) | Default configuration: archive URL, year, top-N, minimum number of days, temperature unit, cache settings |

## Requirements

Built and tested with **GCC 16.1**, **CMake 4.2**, **Boost 1.92.0** and **Catch2 3** on Ubuntu 26.04.

- A C++26 compiler (the code uses `std::format`, `std::print`, ranges and `std::function_ref`)
- CMake ≥ 3.28 and Ninja
- **Boost 1.92**, with the compiled libraries `program_options`, `iostreams` and `url`
  (the rest — Property_tree, Beast, Asio, Tokenizer, String_algo, Accumulators — are header-only).
  Boost.Iostreams must be built with zlib support.
- **OpenSSL** (development package) for HTTPS
- **zlib**, and bzip2/lzma which Boost.Iostreams pulls in
- **Catch2 3** for the unit tests

On Ubuntu/Debian the system dependencies are:

```bash
sudo apt install build-essential cmake ninja-build libssl-dev zlib1g-dev libbz2-dev liblzma-dev
```

## Build & run

The presets assume GCC in `/opt/gcc-16.1` and Boost and Catch2 installed under
`$HOME/Libs/{Boost,Catch2}/latest`. Adjust `CMakePresets.json` to your setup, or configure
without presets:

```bash
cmake -S . -B build -G Ninja \
      -DBoost_ROOT=/path/to/boost \
      -DCatch2_ROOT=/path/to/catch2      # omit when installed in a system location
cmake --build build
ctest --test-dir build
```

With presets:

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Run from the project directory, so the program finds `weather-data.ini` (with the presets the binaries end up in `build/debug/` or `build/release/` instead of `build/`):

```bash
./build/version-4/weather-data-v4 --help
./build/version-4/weather-data-v4 --year 2024              # stream the archive directly over HTTPS
./build/version-4/weather-data-v4 --year 2024 --download   # just download the archive to ./cache
./build/version-4/weather-data-v4 --year 2024 --top 10 --config my-settings.ini
```

Set `cache = true` in `weather-data.ini` to download an archive once and then process the
local copy on later runs. Archives exist for the years 1929 to 2025.
