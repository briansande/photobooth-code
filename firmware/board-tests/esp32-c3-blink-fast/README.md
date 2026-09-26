# ESP32-C3 SuperMini fast blink

Known-good upload test for the SuperMini HW-466AB. The onboard blue GPIO8 LED
is on for 100 ms and off for 100 ms: five complete blinks per second. USB serial
prints `FAST blink test: N` at 115200 baud after each blink.

From the repository root:

```powershell
.\tools\flash.ps1 firmware/board-tests/esp32-c3-blink-fast
```

No external wiring or libraries are required. The red power LED stays on.
