#include <stdlib.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>
#include <Adafruit_SH110X.h> // OLED Display 
#include <Adafruit_GFX.h> // Graphics library for the display
#include <WiFi.h>
#include "button.h" // Button debouncing and handling

#include "wifi_login.h" // Edit this file to set your WiFi credentials
#include "busstop_config.h" // Edit this file to set your bus stop configuration

// Include only ONE of these transit API headers  v
#include "translink.h"
// #include "trimet.h"
// -----------------------------------------------^

// Display object
Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);

// Fonts
// Customized with https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
#include "fonts/RobotoMono_Bold18pt7b.h"
#define FONT_BUS &RobotoMono_Bold18pt7b

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

struct button rotate_screen_button;
int screen_direction;
// Buttons on the OLED display
#define BUTTON_A 15
#define BUTTON_B 32
#define BUTTON_C 14

// Schedule fetch timer
hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
void ARDUINO_ISR_ATTR onTimer(){
    // Give a semaphore that we can check in the loop
    xSemaphoreGiveFromISR(timerSemaphore, NULL);
}

int update_timer = 58; // Incremented by timer ISR, reset to 0 after 60
struct bus bus_1, bus_2;

void setup() {
    Serial.begin(115200);

    bus_1.error_code = ERR_UNINITIALIZED;
    bus_2.error_code = ERR_UNINITIALIZED;

    // Init display
    screen_direction = 1;
    display.begin(0x3C, true); // Address 0x3C default
    display.clearDisplay();
    display.display();
    delay(250);

    display.setRotation(screen_direction ? 1 : 3);

    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0,0);
    display.clearDisplay();

    // Init wifi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    display.println("Connecting to:");
    display.println(ssid);
    display.display();
    for (int i = 0; WiFi.status() != WL_CONNECTED && i < 4; i++) {
        delay(500);
        display.print(".");
        display.display();
    }

    // Create semaphore to inform us when the timer has fired
    timerSemaphore = xSemaphoreCreateBinary();
    timer = timerBegin(1000000); // 1 MHz
    timerAttachInterrupt(timer, &onTimer);
    timerAlarm(timer, 500*1000, true, 0); // Start 0.5s repeating timer

    // Init buttons
    init_button(&rotate_screen_button, BUTTON_C);

    update_display();
}

void update_display() {
    display.clearDisplay();
    display.setRotation(screen_direction ? 1 : 3);

    // TODO: Deal with error_code for bus_1 and bus_2

    // Display bus 1 departure time
    display.setCursor(3,20);
    display.setFont(FONT_BUS);
    if (bus_1.real_time) {
        display.print("&  "); // & mapped to a bus icon in the font
    } else {
        display.print("(  "); // ( mapped to a bus icon in the font, non-real-time version
    }
    display.print(bus_1.eta / 60); // Display ETA in minutes
    display.print("m");

    // Display bus 2 departure time
    display.setCursor(3,56);
    if (bus_2.real_time) {
        display.print("'  "); // ' mapped to a bus icon in the font
    } else {
        display.print(")  "); // ) mapped to a bus icon in the font, non-real-time version
    }
    display.print(bus_2.eta / 60); // Display ETA in minutes
    display.print("m");

    display.setFont();
    // Show if wifi is connected with a little "antenna" in the corner
    if (WiFi.status() == WL_CONNECTED) {
        display.drawChar(display.width() - 6, 0, 0x1F, 1, 0, 1);
    }

    display.drawLine(0, 0, 0, 64 * update_timer / 60, SH110X_WHITE);
    
    display.display();
}

void loop() {
    // Check if the timer interrupt has set the semaphore
    if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE){
        update_display();
        update_timer++;
    }

    if (update_timer > 60) {
        update_timer = 0;
        // TODO: Indicate that we're fetching the schedule on the screen
        fetch_schedule(STOP_NUMBER, ROUTE_NUMBER_1, &bus_1);
        fetch_schedule(STOP_NUMBER, ROUTE_NUMBER_2, &bus_2);
        // TODO: Remove indication
        update_display();
    }

    switch (handle_button(&rotate_screen_button)) {
    case BUTTON_HELD_50MS:
        // Short press, update the display
        // TODO: Trigger an on-demand update of the display, including the bus schedules
        update_display();
        break;

    case BUTTON_HELD_500MS:
        // Long press, rotate the screen
        screen_direction = ! screen_direction;
        break;
    
    default: break;
    }

    // CPU needs this to handle IO
    delay(20);
    yield();
}
