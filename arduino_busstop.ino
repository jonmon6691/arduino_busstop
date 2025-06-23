#include <stdlib.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>
#include "button.h"

// OLED Headers
#include <Adafruit_SH110X.h>
#include <Adafruit_GFX.h>

// Fonts
#include "fonts/RobotoMono_Bold24pt7b.h"
#include "fonts/RobotoMono_Bold22pt7b.h"
#include "fonts/RobotoMono_Bold18pt7b.h"
#include "fonts/RobotoMono_Bold15pt7b.h"
#define FONT_PCT &RobotoMono_Bold24pt7b
#define FONT_EVS &RobotoMono_Bold22pt7b
#define FONT_RAW &RobotoMono_Bold18pt7b
#define FONT_RAW_E6 &RobotoMono_Bold15pt7b

// Light sensor header
#include "Adafruit_LTR390.h"

// Wifi and server headers
#include <WiFi.h>
#include <WiFiClient.h>
#include "wifi_login.h"

// Buttons on the OLED display
#define BUTTON_A 15
#define BUTTON_B 32
#define BUTTON_C 14

struct button clear_button, set_target_button, units_button;

int screen_direction;

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

// Display object
Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);

// UV read timer
hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;

unsigned long intensity;
unsigned long exposure;
unsigned long target_exposure;
unsigned long millis_start;
unsigned long millis_end;

void setup() {
	// Init display
	display.begin(0x3C, true); // Address 0x3C default
	display.clearDisplay();
	display.display();
	delay(250);

	screen_direction = 1;
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
	timer = timerBegin(1000000);
	timerAttachInterrupt(timer, &onTimer);
	timerAlarm(timer, 500000, true, 0); // true means repeat

	// Init buttons
	init_button(&clear_button, BUTTON_A);
	init_button(&set_target_button, BUTTON_B);
	init_button(&units_button, BUTTON_C);

}

void ARDUINO_ISR_ATTR onTimer(){
	// Give a semaphore that we can check in the loop
	xSemaphoreGiveFromISR(timerSemaphore, NULL);
}

void update_display() {
	display.clearDisplay();

	display.setRotation(screen_direction ? 1 : 3);
	display.setCursor(1,1);
	display.print("Intensity: ");

	display.setCursor(1,9);
	display.println("Exposure: ");

	// Reset to system font
	display.setFont();

	// Display exposure time
	display.setCursor(1,display.height()-8);
	display.print("Time: "); 
	
	// Show if wifi is connected with a little "antenna" in the corner
	if (WiFi.status() == WL_CONNECTED) {
		display.drawChar(display.width() - 6, 0, 0x1F, 1, 0, 1);
	}
	
	display.display();
}

void loop() {
	// Check if the timer interrupt has set the semaphore
	if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE){
		update_display();
	}

	switch (handle_button(&clear_button)) {
	case BUTTON_HELD_500MS:
		break;
	
	case BUTTON_HELD_2000MS:
		break;

	default: break;
	}

	switch (handle_button(&set_target_button)) {
	case BUTTON_HELD_500MS:
		break;
	
	default: break;
	}

	switch (handle_button(&units_button)) {
	case BUTTON_HELD_50MS:
		break;

	case BUTTON_HELD_500MS:
		screen_direction = ! screen_direction;
		break;
	
	default: break;
	}

	// CPU needs this to handle IO
	delay(20);
	yield();
}
