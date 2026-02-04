#ifndef NETWORK_MODULE_H
#define NETWORK_MODULE_H

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoJson.h>
#include "gps_module.h"

/**
 * Network Status Enum
 */
enum class NetworkStatus : uint8_t {
    DISCONNECTED = 0,
    CONNECTING,
    CONNECTED,
    ERROR
};

/**
 * HTTP Response Structure
 */
struct HttpResponse {
    int16_t statusCode;
    bool success;
};

/**
 * Network Module Class - Handles Ethernet and HTTP operations
 */
class NetworkModule {
public:
    NetworkModule(uint8_t csPin, uint8_t rstPin)
        : _csPin(csPin), _rstPin(rstPin), _status(NetworkStatus::DISCONNECTED) {}

    /**
     * Initialize Ethernet with hardware reset
     * @param mac MAC address array (6 bytes)
     * @param timeoutMs DHCP timeout
     * @return true if connected successfully
     */
    bool begin(const uint8_t* mac, uint32_t timeoutMs = 10000) {
        // Hardware reset W5500
        pinMode(_rstPin, OUTPUT);
        digitalWrite(_rstPin, LOW);
        delay(50);
        digitalWrite(_rstPin, HIGH);
        delay(500);

        // Initialize SPI
        Ethernet.init(_csPin);

        _status = NetworkStatus::CONNECTING;

        // DHCP with timeout
        if (Ethernet.begin(const_cast<uint8_t*>(mac)) == 0) {
            _status = NetworkStatus::ERROR;
            return false;
        }

        // Verify we got a valid IP
        if (Ethernet.localIP() == IPAddress(0, 0, 0, 0)) {
            _status = NetworkStatus::ERROR;
            return false;
        }

        _status = NetworkStatus::CONNECTED;
        return true;
    }

    /**
     * Maintain Ethernet connection (call periodically)
     */
    void maintain() {
        Ethernet.maintain();
    }

    /**
     * Check if connected
     */
    bool isConnected() const {
        return _status == NetworkStatus::CONNECTED &&
               Ethernet.localIP() != IPAddress(0, 0, 0, 0);
    }

    /**
     * Get local IP as string
     */
    void getLocalIP(char* buffer, size_t bufferSize) const {
        IPAddress ip = Ethernet.localIP();
        snprintf(buffer, bufferSize, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    }

    /**
     * Get network status
     */
    NetworkStatus getStatus() const {
        return _status;
    }

    /**
     * Send GPS data via HTTP POST
     * @param host Server hostname
     * @param path URL path
     * @param port Server port
     * @param deviceId Device identifier
     * @param gpsData GPS data to send
     * @return HttpResponse with status
     */
    HttpResponse sendGPSData(const char* host, const char* path, uint16_t port,
                             const char* deviceId, const GPSData& gpsData) {
        HttpResponse response = {0, false};

        if (!isConnected()) {
            return response;
        }

        // Connect to server
        if (!_client.connect(host, port)) {
            _client.stop();
            return response;
        }

        // Build JSON payload using stack buffer
        char jsonBuffer[384];
        buildJsonPayload(jsonBuffer, sizeof(jsonBuffer), deviceId, gpsData);

        // Send HTTP POST request
        sendHttpPost(host, path, jsonBuffer);

        // Read response
        response = readHttpResponse();

        // Cleanup
        _client.stop();

        return response;
    }

private:
    const uint8_t _csPin;
    const uint8_t _rstPin;
    NetworkStatus _status;
    EthernetClient _client;

    /**
     * Build JSON payload into buffer (no heap allocation)
     */
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

        // Add system info
        char ipBuffer[16];
        getLocalIP(ipBuffer, sizeof(ipBuffer));
        doc["ip"] = ipBuffer;
        doc["uptime_sec"] = millis() / 1000;
        doc["free_heap"] = ESP.getFreeHeap();

        serializeJson(doc, buffer, bufferSize);
    }

    /**
     * Send HTTP POST request
     */
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

    /**
     * Read HTTP response with timeout
     */
    HttpResponse readHttpResponse() {
        HttpResponse response = {0, false};
        const uint32_t timeout = 5000;
        const uint32_t startTime = millis();

        // Wait for response
        while (_client.available() == 0) {
            if (millis() - startTime > timeout) {
                return response;
            }
            yield();
        }

        // Read status line
        if (_client.available()) {
            char statusLine[64];
            size_t len = _client.readBytesUntil('\n', statusLine, sizeof(statusLine) - 1);
            statusLine[len] = '\0';

            // Parse status code from "HTTP/1.1 200 OK"
            char* codeStart = strchr(statusLine, ' ');
            if (codeStart) {
                response.statusCode = atoi(codeStart + 1);
                response.success = (response.statusCode >= 200 && response.statusCode < 300);
            }
        }

        // Drain remaining response
        while (_client.available()) {
            _client.read();
        }

        return response;
    }
};

#endif // NETWORK_MODULE_H
