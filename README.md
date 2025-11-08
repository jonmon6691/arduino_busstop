# Arduino Bus Stop
An Adafruit Feather based Bus stop arrival display

![](media/busstop.gif)

## Buttons
| Button | Function | Notes |
| --- | --- | --- |
| A | | |
| B | | |
| C | Rotate Display | Press and hold to rotate the screen 180 degrees. |

## Configuration
Copy the file `user_config.h.example` to `user_config.h`. Then edit it to customize the display.

### WiFi
and change these lines, leaving the "quote marks" intact:
```user_config.h
#define WIFI_SSID "your ssid"
#define WIFI_PASSWORD "enter your wifi password here"
```
### Transit network and bus stop
This project supports the following transit networks:
* Translink (Vancouver, BC)
* TriMet (Portland, OR) (Untested)

To choose a network, uncomment the correct network module header file.

Then set the bus stop id and routes to display
``` user_config.h
#define STOP_NUMBER 52598
#define ROUTE_NUMBER_1 110
#define ROUTE_NUMBER_2 144
```

## Building
Build using the Arduino IDE or compatible IDE such as VS Code. After uploading the sketch, when the Blueprint powers on, it will show "Connecting to your_ssid..." on the screen. Ensure this matches the ssid that you set. While the Blueprint is connected to wifi, there will be a small antenna symbol in the upper right corner of the screen.

# Hardware
- Adafruit HUZZAH32 – ESP32 Feather Board 
  - [https://www.adafruit.com/product/3405](https://www.adafruit.com/product/3405)
  - Note: Other wifi-enabled boards should be compatible but haven't been tested
- Adafruit FeatherWing OLED - 128x64 OLED Add-on For Feather - STEMMA QT / Qwiic 
  - [https://www.adafruit.com/product/4650](https://www.adafruit.com/product/4650)
