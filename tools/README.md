# Development tools

`flash.ps1` builds and uploads any firmware project. It automatically selects
the ESP32-C3 when exactly one matching USB device is connected:

```powershell
.\tools\flash.ps1 firmware/board-tests/esp32-c3-blink-fast
```

When multiple boards are connected, select one explicitly:

```powershell
.\tools\flash.ps1 firmware/shutter-test -Port COM5
```
