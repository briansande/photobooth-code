# ESP32-C3 SuperMini USB blink test

For the SuperMini HW-466AB with the onboard blue LED on GPIO8 (active-low).
The LED is on for 100 ms, then off for 100 ms (five blinks per second). The red power
LED stays on. USB serial prints `SuperMini blink test: 1`, `2`, etc. once
per blink at 115200 baud. No external wiring or libraries are needed.

## Build and upload with PlatformIO

Open this folder as a PlatformIO project, then use Build and Upload.
Or, from the repository root in PowerShell:

```powershell
$pio = "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"
& $pio run --project-dir esp32-c3-blink
& $pio run --project-dir esp32-c3-blink --target upload --upload-port COM5
& $pio device monitor --port COM5 --baud 115200
```

COM5 was detected during initial setup. Run `& $pio device list` if Windows
assigns a different port. Close any serial monitor before uploading.
The first build downloads any missing ESP32-C3 compiler tools.
Uploading replaces the program currently on the board.

If upload gets stuck connecting, hold **BOOT**, tap and release **RESET**,
then release **BOOT** and upload again. Tap **RESET** afterward if needed.
Use a USB cable that supports data if no COM port appears.

## Arduino IDE alternative

Create a sketch named `SuperMiniBlink` and copy `src/main.cpp` into it.
Install **esp32 by Espressif Systems** in Boards Manager, then select:

- Board: **ESP32C3 Dev Module**
- USB CDC On Boot: **Enabled**
- Flash Size: **4MB**
- Flash Mode: **DIO**
- Port: the board's COM port

Click Upload. Open Serial Monitor at **115200 baud** for the heartbeat.

References: [SuperMini GPIO8 LED](https://nuttx.apache.org/docs/latest/platforms/risc-v/esp32c3/boards/esp32c3-supermini/index.html),
[Espressif USB upload settings](https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/cdc_dfu_flash.html).
