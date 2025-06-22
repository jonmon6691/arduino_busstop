# Arduino Blueprint
An Adafruit Feather based UV exposure meter for making cyanotypes and other alt process prints with repeatable consitency. Use Blueprint to find an exposure in controlled conditions, then it becomes possible to print in any condition, under sunlight, overcast, through a window, all without guesswork.

![Meter](docs/meter.jpg)

## Buttons
<img src="docs/buttons.jpg" width="400" rotate="left">

| Button | Function | Notes |
| --- | --- | --- |
| A | Clear Current Exposure | Press and hold for half a second to clear the current exposure. Keep holding for 2 seconds to clear the current target exposure and go back to raw exposure mode. |
| B | Set Target Exposure | Press and hold for half a second to set the current exposure as the target exposure. You can then clear back to 0% by pressing and holding Button A for half a second. |
| C | Change Display | Click to cycle through the display units while in Target Exposure mode. Cycles betwen percentage, ev stops, and raw exposure value. Press and hold to rotate the screen 180 degrees. |

## Raw Exposure mode
<img src="docs/raw_exposure_mode.jpg" width="200" align="left">

By default, Blueprint starts in Raw Exposure mode when it powers on. You can tell you're in raw exposure mode because the exposure value will _not_ have a unit after it.

Use this mode to determine an exposure value that works well for a given negative. Then note the value after exposing and make adjustments depending on the result of the print. Save this number to set up target exposure mode.

To enter raw exposure mode after setting a target, you can change the unit display by clicking the C button until the unitless raw exposure is displayed. 

## Target Exposure mode
| Under exposed | Over Exposed |   |
| --- | --- | --- |
| <img src="docs/target_percent_under.jpg" width="200"> | <img src="docs/target_percent_done.jpg" width="200"> | Percentage display |
| <img src="docs/target_evs_under.jpg" width="200"> |  <img src="docs/target_evs_done.jpg" width="200"> | EV Stops display |

Target exposure mode will take an exposure value determined in Raw Exposure mode and then display the exposure as a percentage of that raw value. Use this mode when printing a negative where you've already determined your desired exposure previously. You know you are in Target Exposure mode when the exposure value ends with a percent sign (%).

There are two ways to enter Target Exposure mode:

1. After arriving at a desired exposure in Raw Exposure mode, press and hold Button B for half a second. This will set the current exposure as the target.
2. Enter a cutsom exposure through the web interface. First make sure Blueprint is connected to wifi. There will be an antenna symbol in the upper right corner. Then, on a device connected to the same network, navigate to http://blueprint.local/set/10000 . This will set the target exposure to 10,000 units. To set a different value, edit the URL with the desired value. Exposures you use often can be bookmarked or kept as links in a table with information about your negatives for example.

To exit Target Exposure mode, press and hold Button A for 2 seconds. You will see the current exposure reset, keep holding until the percent sign goes away.

Click Button C to toggle between displaying the exposure as percentage, EV stops, and the current raw exposure. Note that 0 stops represents 100% exposure, -1 stop is 50% and 2 stops is 200%.

## Reseting exposure
The exposure can be reset in one of 3 ways:

1. Pressing and holding Button A for at least half a second
2. By navigating to http://blueprint.local/reset while the Blueprint is connected to wifi. 
3. Alternatively, pressing the hard reset button on the Blueprint will reboot the device thererby reseting the exposure as well as reconnecting to wifi

## Building (and Connecting to WiFi)
Copy the file `wifi_login.h.example` to `wifi_login.h`. Then edit it and change these lines, leaving the "quote marks" intact:
```wifi_login.h
#define WIFI_SSID "your ssid"
...
#define WIFI_PASSWORD "enter your wifi password here"
```

Build using the Arduino IDE or compatible IDE such as VS Code. After uploading the sketch, when the Blueprint powers on, it will show "Connecting to your_ssid..." on the screen. Ensure this matches the ssid that you set. While the Blueprint is connected to wifi, there will be a small antenna symbol in the upper right corner of the screen. Navigate to http://blueprint.local/ on a device connected to the same network to see a simple home page with example links to the set and reset functions.

Note: Blueprint uses a technology called "mdns" to make itself available on your local network via the "cooker.local" domain name. Unfortnately, as of spring 2022, mdns is not widely supported on Android devices. If you are having trouble connecting, search the internet to find out if your device supports mdns. 

# Hardware
- Adafruit HUZZAH32 – ESP32 Feather Board 
  - [https://www.adafruit.com/product/3405](https://www.adafruit.com/product/3405)
  - Note: Other wifi-enabled boards should be compatible but haven't been tested
- Adafruit FeatherWing OLED - 128x64 OLED Add-on For Feather - STEMMA QT / Qwiic 
  - [https://www.adafruit.com/product/4650](https://www.adafruit.com/product/4650)
- Adafruit LTR390 UV Light Sensor - STEMMA QT / Qwiic
  - [https://www.adafruit.com/product/4831](https://www.adafruit.com/product/48310
