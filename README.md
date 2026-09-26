# Photobooth Code

A multi-project repository for ESP32-C3 photobooth firmware, reusable component
libraries, hardware notes, and development tools.

## Projects

Each directory under `firmware/` is an independent PlatformIO project and can
be built or uploaded without affecting the source for the other projects.

| Project | Purpose | Status |
| --- | --- | --- |
| [`firmware/board-tests/esp32-c3-blink-fast`](firmware/board-tests/esp32-c3-blink-fast/README.md) | Five-blink-per-second board and USB test | Working |
| [`firmware/board-tests/esp32-c3-blink-slow`](firmware/board-tests/esp32-c3-blink-slow/README.md) | Half-speed firmware-switching test | Working |
| [`firmware/shutter-test`](firmware/shutter-test/README.md) | Isolated shutter development | Safe scaffold |
| [`firmware/paper-feed-test`](firmware/paper-feed-test/README.md) | Isolated paper-feed development | Safe scaffold |
| [`firmware/photobooth-controller`](firmware/photobooth-controller/README.md) | Future combined controller | Safe scaffold |

Reusable component code belongs in [`libraries/`](libraries/README.md), wiring
and hardware details in [`hardware/`](hardware/README.md), and helper scripts in
[`tools/`](tools/README.md).

## Flash a project

Connect one ESP32-C3 SuperMini by USB and run this from PowerShell. The helper
finds its current COM port, builds the selected project, and uploads it:

```powershell
.\tools\flash.ps1 firmware/board-tests/esp32-c3-blink-fast
.\tools\flash.ps1 firmware/board-tests/esp32-c3-blink-slow
```

Pass `-Port COM5` when more than one compatible board is connected. See each
project's README for its expected hardware behavior and serial output.
