#ifndef _TRIMET_H_
#define _TRIMET_H_

#ifdef SYSTEM_MUTEX
#error "Multiple transit system headers included, only include one"
#else
#define SYSTEM_MUTEX
#endif

#include <ArduinoJson.h>
#include "transit_api.h"

// Digicert G2, used by developer.trimet.org as of July 2025. Expires 2038
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

    JsonVariant resultSet = doc["resultSet"];
    JsonVariant arrival = resultSet["arrival"][0];
    JsonVariant scheduled = arrival["scheduled"];
    JsonVariant estimated = arrival["estimated"];
    JsonVariant shortSign = arrival["shortSign"];


    if (scheduled.isNull() || estimated.isNull() || shortSign.isNull()) {
        Serial.println("[JSON] No departure time or real time data found");
        out->error_code = ERR_JSON_MISSING_DATA;
        return;
    }

    long now = resultSet["queryTime"];
    long sch = scheduled;
    long est = estimated;

    out->route_number = route_number;
    strftime(out->dep_time, 6, "%H:%M", localtime(&sch));
    out->eta = max_(est - now, 0);
    out->real_time = est > 0;
    out->error_code = ERR_NO_ERROR;
}


void fetch_schedule(int stop_number, int route_number, struct bus *out) {
    out->error_code = ERR_UNINITIALIZED;

    char url[128];
    snprintf(url, 128, "https://developer.trimet.org/ws/V2/arrivals?locIDs=%d&json=true&appID=01A4A3D242C07049003BA35D8", stop_number);

    int http_error_code = ERR_UNINITIALIZED;
    String payload = execute_http_request(url, &http_error_code);

    if (http_error_code != ERR_NO_ERROR) {
        out->error_code = http_error_code;
        return;
    }

    parse_schedule_json(payload, route_number, out);
}

#endif // _TRIMET_H_