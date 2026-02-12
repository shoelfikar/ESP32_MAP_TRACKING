#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "gps_module.h"

enum class WiFiNetworkStatus : uint8_t {
    DISCONNECTED = 0,
    CONNECTING,
    CONNECTED,
    ERROR
};

struct HttpResponse {
    int16_t statusCode;
    bool success;
};

class WiFiNetworkModule {
public:
    bool begin(const char* ssid, const char* password, uint32_t timeoutMs = 10000) {
        _status = WiFiNetworkStatus::CONNECTING;

        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);

        uint32_t startTime = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - startTime > timeoutMs) {
                _status = WiFiNetworkStatus::ERROR;
                return false;
            }
            delay(500);
        }

        _status = WiFiNetworkStatus::CONNECTED;
        return true;
    }

    void maintain() {
        if (WiFi.status() != WL_CONNECTED) {
            _status = WiFiNetworkStatus::DISCONNECTED;
        }
    }

    bool isConnected() const {
        return WiFi.status() == WL_CONNECTED;
    }

    void getLocalIP(char* buffer, size_t bufferSize) const {
        IPAddress ip = WiFi.localIP();
        snprintf(buffer, bufferSize, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    }

    WiFiNetworkStatus getStatus() const {
        return _status;
    }

    HttpResponse sendGPSData(const char* host, const char* path, uint16_t port,
                             const char* deviceId, const GPSData& gpsData) {
        HttpResponse response = {0, false};

        if (!isConnected()) {
            Serial.println("[HTTP] WiFi not connected");
            return response;
        }

        Serial.printf("[HTTP] Connecting to %s:%d...\n", host, port);

        if (!_client.connect(host, port)) {
            Serial.println("[HTTP] Connection failed!");
            _client.stop();
            return response;
        }

        Serial.println("[HTTP] Connected, sending POST...");

        char jsonBuffer[384];
        buildJsonPayload(jsonBuffer, sizeof(jsonBuffer), deviceId, gpsData);

        Serial.printf("[HTTP] Payload: %s\n", jsonBuffer);

        sendHttpPost(host, path, jsonBuffer);

        response = readHttpResponse();

        Serial.printf("[HTTP] Response: %d (success=%d)\n", response.statusCode, response.success);

        _client.stop();

        return response;
    }

private:
    WiFiNetworkStatus _status = WiFiNetworkStatus::DISCONNECTED;
    WiFiClient _client;

    void buildJsonPayload(char* buffer, size_t bufferSize,
                          const char* deviceId, const GPSData& gpsData) {
        StaticJsonDocument<384> doc;

        doc["device_id"] = deviceId;

        if (gpsData.valid) {
            doc["status"] = "online";
            doc["latitude"] = gpsData.latitude;
            doc["longitude"] = gpsData.longitude;
            doc["speed"] = gpsData.speed;
            doc["altitude"] = gpsData.altitude;
            doc["course"] = gpsData.course;
            doc["satellites"] = gpsData.satellites;
            doc["timestamp"] = gpsData.datetime;
        } else {
            doc["status"] = "no_fix";
            doc["satellites"] = gpsData.satellites;
        }

        char ipBuffer[16];
        getLocalIP(ipBuffer, sizeof(ipBuffer));
        doc["ip"] = ipBuffer;
        doc["uptime_sec"] = millis() / 1000;
        doc["free_heap"] = ESP.getFreeHeap();

        serializeJson(doc, buffer, bufferSize);
    }

    void sendHttpPost(const char* host, const char* path, const char* payload) {
        const size_t payloadLen = strlen(payload);

        _client.print(F("POST "));
        _client.print(path);
        _client.println(F(" HTTP/1.1"));

        _client.print(F("Host: "));
        _client.println(host);

        _client.println(F("Content-Type: application/json"));
        _client.println(F("Connection: close"));

        _client.print(F("Content-Length: "));
        _client.println(payloadLen);

        _client.println();
        _client.print(payload);
    }

    HttpResponse readHttpResponse() {
        HttpResponse response = {0, false};
        const uint32_t timeout = 5000;
        const uint32_t startTime = millis();

        Serial.println("[HTTP] Waiting for response...");

        while (_client.available() == 0) {
            if (millis() - startTime > timeout) {
                Serial.println("[HTTP] Response timeout!");
                return response;
            }
            yield();
        }

        if (_client.available()) {
            char statusLine[64];
            size_t len = _client.readBytesUntil('\n', statusLine, sizeof(statusLine) - 1);
            statusLine[len] = '\0';

            Serial.printf("[HTTP] Status line: %s\n", statusLine);

            char* codeStart = strchr(statusLine, ' ');
            if (codeStart) {
                response.statusCode = atoi(codeStart + 1);
                response.success = (response.statusCode >= 200 && response.statusCode < 300);
            }
        }

        while (_client.available()) {
            _client.read();
        }

        return response;
    }
};

#endif // WIFI_MODULE_H
