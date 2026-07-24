# Cap-LoRa-1262-GPS

GNSS status viewer for M5Stack CardputerZero and the ATGM336H receiver built
into the Cap LoRa-1262 accessory.

## Features

- Show latitude, longitude, altitude, speed, course, UTC time, and fix state
- Show satellite count and HDOP/PDOP/VDOP diagnostics
- Parse common NMEA 0183 RMC, GGA, GLL, GSA, GSV, VTG, and ZDA sentences
- Report UART, receiver, sentence, and checksum status
- Use an SDL mock receiver for desktop development without GPS hardware

## Dependencies

Run the bootstrap script once after cloning this repository:

```bash
./bootstrap.sh
```

It fetches `lvgl`, `spdlog`, `smooth_ui_toolkit`, and `minmea` under
`dependencies/`. SDL builds require SDL2 development files.

## Build

For SDL desktop testing:

```bash
cmake -S . -B build/sdl -DCAP_GPS_USE_SDL=ON
cmake --build build/sdl -j8
```

For a native CardputerZero build:

```bash
cmake -S . -B build/cp0 -DCAP_GPS_USE_SDL=OFF
cmake --build build/cp0 -j8
```

The desktop and device binaries are written to `dist/sdl/` and `dist/device/`
respectively.

Run the tests with:

```bash
cmake -S . -B build/tests -DCAP_GPS_USE_SDL=ON -DBUILD_TESTING=ON
cmake --build build/tests -j8
ctest --test-dir build/tests --output-on-failure
```

## Usage

Run the SDL build with:

```bash
CAP_GPS_SDL_ZOOM=2 ./dist/sdl/M5CardputerZero-Cap-LoRa-1262-GPS
```

Key controls:

- `Z`/`C` or Left/Right: switch between Position and Details
- `F`/`X` or Up/Down: scroll Details
- Enter: retry receiver initialization after an error
- Esc: close a dialog or exit

The desktop mock starts without a fix and then supplies a sample fix.
Set `CAP_GPS_MOCK_INIT_FAIL_COUNT=N` to fail its first `N` initialization
attempts when testing the error dialog and retry flow.

## Hardware

The device build reads NMEA 0183 data from the Cap's ATGM336H receiver through
`/dev/ttyS0` at 115200 baud, 8N1. It enables Cap power through the
`ext_5v_out` LED-class interface. The GPS app does not initialize the SX1262,
use SPI, or change the LoRa reset pin.

The Debian package launches the hardware app as root through a non-interactive,
command-specific sudo rule for members of the `gpio` group. The rule permits
only the installed binary with no command arguments. This is currently needed
for `ext_5v_out`; UART access alone normally works for members of `dialout`.

A cold fix can take about 30 seconds and usually requires a clear view of the
sky. Hardware initialization errors are shown in the app.

## Package

Build the CardputerZero `arm64` Debian package on an x86 Linux or WSL2 host with
the Docker wrapper:

```bash
./packaging/docker/package_deb.sh
```

To package natively on a CardputerZero instead, run:

```bash
./packaging/deb/package_deb.sh
```

The generated package is written to `dist/`:

```text
dist/m5cardputerzero-cap-lora-1262-gps_<version>_m5stack1_arm64.deb
```

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for dependency licenses.
