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
#include <WebServer.h>
#include <ESPmDNS.h>
#include <uri/UriBraces.h>
#include "wifi_login.h"

// Served back to the client after any valid request
const char index_html[] PROGMEM = \
"<body style=\"float:left\"><ul><meta name=\"viewport\" content=\"width=device-width, initial-scale=2\">"
"<li><a href=\"/reset\">Reset Exposure</a></li>"
"<li><a href=\"/set/0\">Raw Exposure mode</a></li>"
"<li><a href=\"/set/10000\">Target Exposure mode: 10k units</a></li>"
"</ul></body>\n";

// Buttons on the OLED display
#define BUTTON_A 15
#define BUTTON_B 32
#define BUTTON_C 14

struct button clear_button, set_target_button, units_button;

int units_mode;
#define UNITS_MODE_PERCENT 0
#define UNITS_MODE_STOPS 1
#define UNITS_MODE_RAW 2

int screen_direction;

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

WebServer server(80);

// Display object
Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);

// UV sensor object
Adafruit_LTR390 ltr = Adafruit_LTR390();

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
	MDNS.begin("blueprint");

	// Init UV sensor
	while ( ! ltr.begin() ) {
		display.clearDisplay();
		display.setCursor(0,0);
		display.println("Connect UV sensor to begin");
		display.display();
		delay(500);
	}
	ltr.setMode(LTR390_MODE_UVS);
	ltr.setGain(LTR390_GAIN_18);
	ltr.setResolution(LTR390_RESOLUTION_20BIT);

	// Create semaphore to inform us when the timer has fired
	timerSemaphore = xSemaphoreCreateBinary();
	timer = timerBegin(1000000);
	timerAttachInterrupt(timer, &onTimer);
	timerAlarm(timer, 500000, true, 0); // true means repeat

	// Init exposure
	exposure = 0;
	target_exposure = 0;
	millis_start = 0;

	// Init buttons
	init_button(&clear_button, BUTTON_A);
	init_button(&set_target_button, BUTTON_B);
	init_button(&units_button, BUTTON_C);

	units_mode = UNITS_MODE_PERCENT;

	// Set http server endpoints
	server.on("/", serve_index);
	server.on("/reset", serve_reset_exposure);
	server.on(UriBraces("/set/{}"), serve_set_exposure);
	server.begin();
}

void serve_index() {
	server.send(200, "text/html", index_html);
}

void serve_reset_exposure() {
	exposure = 0;
	millis_start = 0;
	serve_index();
}

void serve_set_exposure() {
	target_exposure = strtoul(server.pathArg(0).c_str(), NULL, 10);
	serve_index();
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
	display.println(intensity);

	display.setCursor(1,9);
	display.println("Exposure: ");

	// Raw Exposure mode
	if ( ! target_exposure || units_mode == UNITS_MODE_RAW) {
		if (exposure < 1E4) 
			{display.setCursor(1,47); display.setFont(FONT_PCT); display.println(exposure);}
		else if (exposure < 1E6)
			{display.setCursor(1,42); display.setFont(FONT_RAW); display.println(exposure);}
		else
			{display.setCursor(1,39); display.setFont(FONT_RAW_E6); display.println(exposure);}
	// Target Exposure mode, percent units
	} else if (units_mode == UNITS_MODE_PERCENT) {
		float progress = (float)exposure * 100.0 / (float)target_exposure;
		display.setCursor(1,47);
		display.setFont(FONT_PCT); 
		if (progress < 10.0) 
			{display.print(progress, 2); display.println("%");}
		else if (progress < 100.0) 
			{display.print(progress, 1); display.println("%");}
		else 
			{display.print(progress, 0); display.println("%");}
	// Target Exposure mode, ev stops units
	} else if (units_mode == UNITS_MODE_STOPS) {
		float progress = log2f( (float)exposure / (float)target_exposure);
		display.setCursor(1,45);
		display.setFont(FONT_EVS);
		// Note FONT_EVS uses a custom '&' character to display the ev symbol
		if (progress < -99.9) 
			{display.print("-99.9"); display.println("&");} 
		else if (progress < -9.99) 
			{display.print(progress, 1); display.println("&");}
		else {
			if (progress >= 0) // Explicitly print the '+' sign
				{display.print("+");}
			display.print(progress, 2); display.println("&");
		}
	} 

	// Reset to system font
	display.setFont();

	// Display exposure time
	display.setCursor(1,display.height()-8);
	display.print("Time: "); 
	if (millis_start > 0) {
		display.print( (float)(millis_end - millis_start) / 1000.0, 1);
		display.println("s");
	}
	
	// Show if wifi is connected with a little "antenna" in the corner
	if (WiFi.status() == WL_CONNECTED) {
		display.drawChar(display.width() - 6, 0, 0x1F, 1, 0, 1);
	}
	
	display.display();
}

void loop() {
	// Check if the timer interrupt has set the semaphore
	if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE){
		// Read UV sensor data
		if (ltr.newDataAvailable()) intensity = ltr.readUVS();
		
		// Accumulate exposure
		exposure += intensity;

		// start the clock only once some UV is detected
		if(intensity > 0) {
			millis_end = millis();
			if (millis_start == 0) millis_start = millis_end;
		}

		// Invert the display once target exposure is reached
		display.invertDisplay(target_exposure && exposure > target_exposure);
		
		update_display();
	}

	switch (handle_button(&clear_button)) {
	case BUTTON_HELD_500MS:
		exposure = 0;
		millis_start = 0;
		break;
	
	case BUTTON_HELD_2000MS:
		target_exposure = 0;
		break;

	default: break;
	}

	switch (handle_button(&set_target_button)) {
	case BUTTON_HELD_500MS:
		target_exposure = exposure;
		break;
	
	default: break;
	}

	switch (handle_button(&units_button)) {
	case BUTTON_HELD_50MS:
		units_mode = (units_mode + 1) % 3;
		update_display();
		break;

	case BUTTON_HELD_500MS:
		screen_direction = ! screen_direction;
		break;
	
	default: break;
	}
	
	// Process http requests
	server.handleClient();

	// CPU needs this to handle IO
	delay(20);
	yield();
}
