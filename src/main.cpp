#include "Arduino.h"
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

const unsigned long eventInterval = 2000;
unsigned long previousTime = 0;

const char* deviceShortName = "";
const char* token = "";

const char* rootCACertificate = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n" \
"MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\n" \
"2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\n" \
"1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\n" \
"q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\n" \
"tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\n" \
"vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\n" \
"BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\n" \
"5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\n" \
"1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\n" \
"NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\n" \
"Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\n" \
"8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\n" \
"pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n" \
"MrY=\n" \
"-----END CERTIFICATE-----\n";

void setup() {
    WiFi.mode(WIFI_STA);
    
    Serial.begin(115200);
    
    WiFiManager wm;

    bool res;
    res = wm.autoConnect("AutoConnectAP","password");
 
    if(!res) {
        Serial.println("Failed to connect");
    } 
    else {
        Serial.println("connected...yeey :)");
    }   
}

void sendData(WiFiClientSecure *client, float values[], String sensors[], int size) {
    HTTPClient https;
    StaticJsonDocument<1024> doc;

    doc["deviceToken"] = String(token);
    JsonArray sensor = doc.createNestedArray("sensors");
    for (int i=0; i< size; i++) sensor.add(sensors[i]);
    JsonArray value = doc.createNestedArray("data");
    for (int i=0; i< size; i++) value.add(values[i]);

    String data;
    serializeJson(doc, data);

    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, "https://cec.azurewebsites.net/api/telemetry/add?deviceShortName="+String(deviceShortName))) {
        Serial.print("[HTTPS] POST...\n");

        https.addHeader("Host", "cec.azurewebsites.net");
        https.addHeader("Content-Type", "application/json");
        https.addHeader("Content-Length", String(data.length()));
        https.addHeader("Accept-Language", "en-US,en;q=0.9" );
        https.addHeader("Accept-Encoding", "gzip,deflate" );
        https.addHeader("Keep-Alive", "300" );
        https.addHeader("Connection", "keep-alive");

        int httpCode = https.POST(data);

        if (httpCode > 0) {
        Serial.printf("[HTTPS] POST... code: %d\n", httpCode);

        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = https.getString();
            Serial.println(payload);
        }
        } else {
            Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }

        https.end();
    } else {
        Serial.printf("[HTTPS] Unable to connect\n");
    }
}

void loop() {
    int arraySize = 2;
    float values[arraySize] = { 0.4, 0.1 };
    String sensors[arraySize] = { "temp-01", "temp-02"};

    unsigned long currentTime = millis();

    if (currentTime - previousTime >=  eventInterval) {
        WiFiClientSecure *client = new WiFiClientSecure;
        if(client) {
            client -> setCACert(rootCACertificate);

            sendData(client, values, sensors, arraySize);
        
            delete client;
        } else {
            Serial.println("Unable to create client");
        }
        
        previousTime = currentTime;
    }
}