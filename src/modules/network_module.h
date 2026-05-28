#ifndef NETWORK_MODULE_H
#define NETWORK_MODULE_H

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoJson.h>
#include "gps_module.h"
#include "../config.h"

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
            Serial.println("[HTTP] Ethernet not connected");
            return response;
        }

        Serial.printf("[HTTP] Connecting to %s:%d...\n", host, port);

        // Connect to server
        if (!_client.connect(host, port)) {
            Serial.println("[HTTP] Connection failed!");
            _client.stop();
            return response;
        }

        Serial.println("[HTTP] Connected, sending POST...");

        // Build JSON payload using stack buffer
        char jsonBuffer[384];
        buildJsonPayload(jsonBuffer, sizeof(jsonBuffer), deviceId, gpsData);

        Serial.printf("[HTTP] Payload: %s\n", jsonBuffer);

        // Send HTTP POST request
        sendHttpPost(host, path, jsonBuffer);

        // Read response
        response = readHttpResponse();

        Serial.printf("[HTTP] Response: %d (success=%d)\n", response.statusCode, response.success);

        // Cleanup
        _client.stop();

        return response;
    }

    /**
     * Test whether a server is reachable by issuing an HTTP GET to `path`.
     * Reachable means: TCP connected AND server returned an HTTP status line.
     * Uses a separate probe socket so it won't disturb _client.
     */
    bool checkReachable(const char* host, uint16_t port, const char* path) {
        if (!isConnected()) {
            return false;
        }

        EthernetClient probe;
        probe.setConnectionTimeout(3000);
        if (!probe.connect(host, port)) {
            probe.stop();
            return false;
        }

        // Minimal HTTP GET
        probe.print(F("GET "));
        probe.print(path);
        probe.println(F(" HTTP/1.1"));
        probe.print(F("Host: "));
        probe.println(host);
        probe.println(F("Connection: close"));
        probe.println();

        // Wait for response
        const uint32_t timeout = 3000;
        const uint32_t startTime = millis();
        while (probe.available() == 0) {
            if (millis() - startTime > timeout) {
                probe.stop();
                return false;
            }
            yield();
        }

        // Parse HTTP status code from "HTTP/1.1 200 OK"
        char statusLine[64];
        size_t len = probe.readBytesUntil('\n', statusLine, sizeof(statusLine) - 1);
        statusLine[len] = '\0';
        probe.stop();

        char* codeStart = strchr(statusLine, ' ');
        return (codeStart != nullptr && atoi(codeStart + 1) > 0);
    }

    /**
     * Send device identity info (IP, MAC, device ID, firmware version) to server.
     * Used for device registration / sync, separate from GPS telemetry.
     */
    HttpResponse syncDeviceInfo(const char* host, const char* path, uint16_t port,
                                const char* deviceId, const char* otaToken) {
        HttpResponse response = {0, false};

        if (!isConnected()) {
            Serial.println("[SYNC] Ethernet not connected");
            return response;
        }

        Serial.printf("[SYNC] Connecting to %s:%d...\n", host, port);

        if (!_client.connect(host, port)) {
            Serial.println("[SYNC] Connection failed!");
            _client.stop();
            return response;
        }

        char jsonBuffer[512];
        buildDeviceInfoPayload(jsonBuffer, sizeof(jsonBuffer), deviceId, otaToken);

        Serial.printf("[SYNC] Payload: %s\n", jsonBuffer);

        sendHttpPost(host, path, jsonBuffer);
        response = readHttpResponse();

        Serial.printf("[SYNC] Response: %d (success=%d)\n", response.statusCode, response.success);

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
     * Build device identity payload (no GPS data)
     */
    void buildDeviceInfoPayload(char* buffer, size_t bufferSize,
                                const char* deviceId, const char* otaToken) {
        StaticJsonDocument<512> doc;

        doc["type"] = "device_sync";
        doc["device_id"] = deviceId;
        doc["firmware_version"] = FIRMWARE_VERSION;
        doc["build_date"] = FIRMWARE_BUILD;
        doc["ota_token"] = otaToken;

        char ipBuffer[16];
        getLocalIP(ipBuffer, sizeof(ipBuffer));
        doc["ip"] = ipBuffer;

        uint8_t mac[6];
        Ethernet.MACAddress(mac);
        char macBuf[18];
        snprintf(macBuf, sizeof(macBuf), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        doc["mac"] = macBuf;

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

        Serial.println("[HTTP] Waiting for response...");

        // Wait for response
        while (_client.available() == 0) {
            if (millis() - startTime > timeout) {
                Serial.println("[HTTP] Response timeout!");
                return response;
            }
            yield();
        }

        // Read status line
        if (_client.available()) {
            char statusLine[64];
            size_t len = _client.readBytesUntil('\n', statusLine, sizeof(statusLine) - 1);
            statusLine[len] = '\0';

            Serial.printf("[HTTP] Status line: %s\n", statusLine);

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
