# Browser-controlled blink test

This firmware serves a page that controls the onboard blue GPIO8 LED's on and
off durations without recompiling. It can join your home Wi-Fi like the older
Shutter-Test project. Your router then gives the ESP32 an address such as
`10.0.0.x`, and you open that exact address from a device on the same network.

## Configure home Wi-Fi

Edit the local `include/wifi_secrets.h` file with your 2.4 GHz Wi-Fi name and
password. It has empty values now and is ignored by Git. On another checkout,
copy `include/wifi_secrets.example.h` to that name first. Do not put credentials
in `platformio.ini` or tracked source. A change to this file requires rebuilding
and uploading the firmware. If the SSID is empty, the private file is absent,
or the router cannot be reached within 15 seconds, the ESP32 starts its own
`ESP32-Blink-Test` Wi-Fi network instead.

## Upload

From the repository root:

```powershell
.\tools\flash.ps1 firmware/web-blink-test
```

## Open the controls

After uploading, read the `Open: http://.../` line from USB serial at 115200
baud. If the board joined your home Wi-Fi, keep your browser device on the same
network and open that URL. The address may change after a router restart.

If serial says it started the fallback access point, connect your phone or
computer to **ESP32-Blink-Test** using password **blinktest**, stay connected if
warned about no internet, and open **http://192.168.4.1/**.

Enter LED-on and LED-off durations in milliseconds and select **Apply**.

Accepted values are 10 through 60,000 ms. Changes take effect immediately but
reset to the 100 ms defaults whenever the ESP32 restarts. Wi-Fi credentials are
compiled from the private header; do not share firmware binaries if you do not
want those credentials embedded in them.

USB serial at 115200 baud reports the access-point address and every accepted
timing change. It also prints a `WEB BLINK TEST running` status with the current
IP address every five seconds, so you can find the page without resetting the
board.
