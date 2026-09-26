# Shared Wi-Fi for ESP32 firmware

This library owns the home-network connection, fallback access point, current
page URL, and reconnection logic. Configure Wi-Fi only once in the Git-ignored
`src/wifi_secrets.h`. A blank local file is present in this checkout; fill in
`kWifiSsid` and `kWifiPassword` with your 2.4 GHz router credentials. In a new
checkout, copy `src/wifi_secrets.example.h` to `src/wifi_secrets.h` first.

To add Wi-Fi to any firmware project directly under `firmware/`, add this line
to its `platformio.ini`:

```ini
lib_extra_dirs = ../../libraries
```

For a project in `firmware/board-tests/`, use `../../../libraries` instead.
Then add this to the application:

```cpp
#include <PhotoboothWiFi.h>

void setup() {
  Serial.begin(115200);
  if (PhotoboothWiFi::begin("My-Setup-AP", "setup1234")) {
    Serial.println(PhotoboothWiFi::pageUrl());
  }
}

void loop() {
  PhotoboothWiFi::loop();
  // Handle web requests and other work here.
}
```

The access point name and password are optional fallback values; the library
uses the shared router credentials first. Without credentials or after a
15-second connection timeout, it starts the fallback access point. Pass no
fallback values if this firmware should require the router connection. The
web server and page remain firmware-specific; the Wi-Fi connection is shared.
Changing the private header requires rebuilding and uploading each firmware
that uses this library.

If the router connection fails, USB serial reports the disconnect reason and
whether the configured network was visible in a 2.4 GHz scan. A visible
network with `AUTH_FAIL` usually warrants checking the password and the
router's Wi-Fi security setting before changing firmware code.
