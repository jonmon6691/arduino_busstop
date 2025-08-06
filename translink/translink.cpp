#include <ArduinoJson.h>
#include "translink.h"

#define max_(a,b) ((a)>(b)?(a):(b))

extern String execute_http_request(const char* url, int *error_code);

const char *rootCACertificate = R"string_literal(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
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

void parse_schedule_json(String payload, int route_number, struct bus *out) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        Serial.printf("[JSON] JSON parse error: %s\n", error.c_str());
        out->error_code = ERR_JSON_PARSE_ERROR;
        return;
    }

    JsonVariant lu = doc[0]["lu"];
    JsonVariant response = doc[0]["r"][0]["t"][0];
    JsonVariant ut = response["ut"];
    JsonVariant dt = response["dt"];
    JsonVariant rt = response["rt"];

    if (lu.isNull() || rt.isNull() || ut.isNull() || dt.isNull()) {
        Serial.println("[JSON] No departure time or real time data found");
        out->error_code = ERR_JSON_MISSING_DATA;
        return;
    }

    out->route_number = route_number;
    strncpy(out->dep_time, (const char *)dt, 6);
    out->eta = max_((long)ut - (long)lu, 0);
    out->real_time = (bool)rt;
    out->error_code = ERR_NO_ERROR;
}


void fetch_schedule(int stop_number, int route_number, struct bus *out) {
	out->error_code = ERR_UNINITIALIZED;

	char url[128];
	snprintf(url, 128, "https://getaway.translink.ca/api/gtfs/stop/%d/route/%d/realtimeschedules?querySize=6", stop_number, route_number);

	int http_error_code = ERR_UNINITIALIZED;
	String payload = execute_http_request(url, &http_error_code);

	if (http_error_code != ERR_NO_ERROR) {
		out->error_code = http_error_code;
		return;
	}

	parse_schedule_json(payload, route_number, out);
}
