#ifndef _HTTP_H_
#define _HTTP_H_

#include <HTTPClient.h>
#include <NetworkClientSecure.h>

#if defined(ESP8266)
#error "This sketch is not compatible with ESP8266"
#endif


// Error codes for struct bus error_code
#define ERR_NO_ERROR 0
#define ERR_UNKNOWN_ERROR -1
#define ERR_JSON_MISSING_DATA -2
#define ERR_JSON_PARSE_ERROR -3
#define ERR_HTTP_ERROR -4
#define ERR_CONNECTION_ERROR -5
#define ERR_CLIENT_CREATE_ERROR -6
#define ERR_UNINITIALIZED -7

#define max_(a,b) ((a)>(b)?(a):(b))

struct bus {
    int route_number;
    char dep_time[6]; // "HH:MM\x00"
    bool real_time;
    int error_code;
    int eta;
};


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


#endif // _HTTP_H_