#include <stdlib.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>
#define max_(a,b) ((a)>(b)?(a):(b))

#include "button.h"
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
// This is a DigiCert Global Root G2 cert, the root Certificate Authority that
// signed the server certificate for the TransLink server https://getaway.translink.ca
// This certificate is valid until Jan 15 00:00:00 2038 GMT
const char *rootCACertificate = R"string_literal(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)string_literal";

// For decoding the translink API data
#include <ArduinoJson.h>

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
} bus_110, bus_144;
int update_timer = 58;

void setup() {
	Serial.begin(115200);

	bus_110.error_code = ERR_UNINITIALIZED;
	bus_144.error_code = ERR_UNINITIALIZED;

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

	// TODO: Deal with error_code for bus_110 and bus_144

	// Display bus 110 departure time
	display.setCursor(3,20);
	display.setFont(FONT_BUS);
	if (bus_110.real_time) {
		display.print("&  "); // & mapped to a bus icon in the font
	} else {
		display.print("(  "); // ( mapped to a bus icon in the font, non-real-time version
	}
	display.print(bus_110.eta / 60); // Display ETA in minutes
	display.print("m");

	// Display bus 144 departure time
	display.setCursor(3,56);
	if (bus_144.real_time) {
		display.print("'  "); // ' mapped to a bus icon in the font
	} else {
		display.print(")  "); // ) mapped to a bus icon in the font, non-real-time version
	}
	display.print(bus_144.eta / 60); // Display ETA in minutes
	display.print("m");

	display.setFont();
	// Show if wifi is connected with a little "antenna" in the corner
	if (WiFi.status() == WL_CONNECTED) {
		display.drawChar(display.width() - 6, 0, 0x1F, 1, 0, 1);
	}

	display.drawLine(0, 0, 0, 64 * update_timer / 60, SH110X_WHITE);
	
	display.display();
}


int fetch_schedule(int stop_number, int route_number, struct bus *out) {
	NetworkClientSecure *client = new NetworkClientSecure;
	int ret = ERR_UNKNOWN_ERROR;
	if (client) {
		client->setCACert(rootCACertificate);
		{
			// Add a scoping block for HTTPClient https to make sure it is destroyed before NetworkClientSecure *client is
			HTTPClient https;

			// Headers stolen from my browser, not sure what the minimum required set is
			https.setUserAgent("Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:139.0) Gecko/20100101 Firefox/139.0");
			https.addHeader("Sec-Fetch-Dest", "document", false, false);
			https.addHeader("Sec-Fetch-Mode", "navigate", false, false);
			https.addHeader("Sec-Fetch-Site", "none", false, false);
			https.addHeader("Sec-Fetch-User", "?1");

			char url[128];
			snprintf(url, 128, "https://getaway.translink.ca/api/gtfs/stop/%d/route/%d/realtimeschedules?querySize=6", stop_number, route_number);

			if (https.begin(*client, url)) {  // HTTPS
				Serial.print("[HTTPS] GET...\n");
				// start connection and send HTTP header
				int httpCode = https.GET();

				// httpCode will be negative on error
				if (httpCode > 0) {
					// HTTP header has been send and Server response header has been handled
					Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

					// file found at server
					if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
						String payload = https.getString();
						JsonDocument doc;
						DeserializationError error = deserializeJson(doc, payload);
						if (!error) {
							JsonVariant lu = doc[0]["lu"];
							JsonVariant response = doc[0]["r"][0]["t"][0];
							JsonVariant ut = response["ut"];
							JsonVariant dt = response["dt"];
							JsonVariant rt = response["rt"];
							if (lu.isNull() == false && rt.isNull() == false && ut.isNull() == false && dt.isNull() == false) {
								out->route_number = route_number;
								strncpy(out->dep_time, (const char *)dt, 6);
								out->eta = max_((long)ut - (long)lu, 0);
								out->real_time = (bool)rt;
								ret = ERR_NO_ERROR;
							} else {
								Serial.println("[JSON] No departure time or real time data found");
								ret = ERR_JSON_MISSING_DATA;
							}
						} else {
							Serial.printf("[JSON] JSON parse error: %s\n", error.c_str());
							ret = ERR_JSON_PARSE_ERROR;
						}
					}
				} else {
					Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
					ret = ERR_HTTP_ERROR;
				}

				https.end();
			} else {
				Serial.printf("[HTTPS] Unable to connect\n");
				ret = ERR_CONNECTION_ERROR;
			}
			// End extra scoping block
		}
		delete client;
	} else {
		Serial.println("Unable to create client");
		ret = ERR_CLIENT_CREATE_ERROR;
	}
	out->error_code = ret;
	return ret;
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
		fetch_schedule(52598, 110, &bus_110);
		fetch_schedule(52598, 144, &bus_144);
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
