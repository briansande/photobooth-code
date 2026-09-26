# Firmware projects

Each child directory containing `platformio.ini` is a self-contained
PlatformIO application. Build or upload it by passing that directory to
PlatformIO, or use `tools/flash.ps1` from the repository root.

- `board-tests/` contains known-good firmware for checking boards and uploads.
- `web-blink-test/` provides a browser-controlled onboard LED test.
- `shutter-test/` is for isolated shutter development.
- `paper-feed-test/` is for isolated paper-feed development.
- `photobooth-controller/` will combine proven components.

Keep application entry points small. Move reusable shutter, paper-feed, sensor,
and protocol behavior into libraries once the hardware interface is understood.
