# Arduino Bus Stop
An Adafruit Feather based Bus stop arrival display

![](media/busstop.gif)

## Buttons
| Button | Function | Notes |
| --- | --- | --- |
| A | | |
| B | | |
| C | Rotate Display | Press and hold to rotate the screen 180 degrees. |

## Building (and Connecting to WiFi)
Copy the file `wifi_login.h.example` to `wifi_login.h`. Then edit it and change these lines, leaving the "quote marks" intact:
```wifi_login.h
#define WIFI_SSID "your ssid"
...
#define WIFI_PASSWORD "enter your wifi password here"
```

Build using the Arduino IDE or compatible IDE such as VS Code. After uploading the sketch, when the Blueprint powers on, it will show "Connecting to your_ssid..." on the screen. Ensure this matches the ssid that you set. While the Blueprint is connected to wifi, there will be a small antenna symbol in the upper right corner of the screen.

## Configuring the stops

This project supports the following transit networks:
* Translink (Vancouver, BC)
* TriMet (Portland, OR) (Untested)

To switch between networks, you need to modify `arduino_busstop.ino` to include the correct network module, and `busstop_config.h` to set the correct stop and route numbers.

For example, to use the Translink network, you would include `translink.h` in `arduino_busstop.ino`:
```c++
#include "translink.h"
```

To use the TriMet network, you would include `trimet.h` in `arduino_busstop.ino`:
```c++
#include "trimet.h"
```

Then, you need to edit `busstop_config.h` to set the desired stop and route numbers.

# Hardware
- Adafruit HUZZAH32 – ESP32 Feather Board 
  - [https://www.adafruit.com/product/3405](https://www.adafruit.com/product/3405)
  - Note: Other wifi-enabled boards should be compatible but haven't been tested
- Adafruit FeatherWing OLED - 128x64 OLED Add-on For Feather - STEMMA QT / Qwiic 
  - [https://www.adafruit.com/product/4650](https://www.adafruit.com/product/4650)
