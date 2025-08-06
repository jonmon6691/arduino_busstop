#include <stdlib.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>
#define max_(a,b) ((a)>(b)?(a):(b))

#include "button.h"
#include "network_config.h"
#include "translink/translink.h"
// #include "trimet/trimet.h"

struct button rotate_screen_button;
int screen_direction;

// OLED Headers
#include <Adafruit_SH110X.h>
#include <Adafruit_GFX.h>
// Display object
Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);

// Fonts
// Customized with https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
#include "fonts/RobotoMono_Bold18pt7b.h"
#define FONT_BUS &RobotoMono_Bold18pt7b

// Wifi and HTTP headers
#include "wifi_login.h"
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

#include <WiFi.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>

#if defined(ESP8266)
#error "This sketch is not compatible with ESP8266"
#endif

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

// Error codes for struct bus error_code
#define ERR_NO_ERROR 0
#define ERR_UNKNOWN_ERROR -1
#define ERR_JSON_MISSING_DATA -2
#define ERR_JSON_PARSE_ERROR -3
#define ERR_HTTP_ERROR -4
#define ERR_CONNECTION_ERROR -5
#define ERR_CLIENT_CREATE_ERROR -6
#define ERR_UNINITIALIZED -7

struct bus {
	int route_number;
	char dep_time[6]; // "HH:MM\x00"
	bool real_time;
	int error_code;
	int eta;
} bus_1, bus_2;
int update_timer = 58;

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

extern const char *rootCACertificate;

String execute_http_request(const char* url, int *error_code) {
    NetworkClientSecure *client = new NetworkClientSecure;
    if (!client) {
        Serial.println("Unable to create client");
        *error_code = ERR_CLIENT_CREATE_ERROR;
        return "";
    }
    client->setCACert(rootCACertificate);

    String payload = "";
    // Add a scoping block for HTTPClient https to make sure it is destroyed before NetworkClientSecure *client is
    {
        HTTPClient https;

        // Headers stolen from my browser, not sure what the minimum required set is
        https.setUserAgent("Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:139.0) Gecko/20100101 Firefox/139.0");
        https.addHeader("Sec-Fetch-Dest", "document", false, false);
        https.addHeader("Sec-Fetch-Mode", "navigate", false, false);
        https.addHeader("Sec-Fetch-Site", "none", false, false);
        https.addHeader("Sec-Fetch-User", "?1");

        if (!https.begin(*client, url)) {
            Serial.printf("[HTTPS] Unable to connect\n");
            *error_code = ERR_CONNECTION_ERROR;
            delete client;
            return "";
        }

        Serial.print("[HTTPS] GET...\n");
        int httpCode = https.GET();

        if (httpCode <= 0) {
            Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
            *error_code = ERR_HTTP_ERROR;
        } else if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            payload = https.getString();
            *error_code = ERR_NO_ERROR;
        } else {
            Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
            *error_code = ERR_HTTP_ERROR;
        }

        https.end();
    }
    delete client;
    return payload;
}

void pp(struct bus* printme) {
	Serial.print(printme->route_number);
	Serial.print(" leaves at ");
	Serial.print(printme->dep_time);
	if (printme->real_time) {
		Serial.print("*");
	}
	Serial.println();
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
