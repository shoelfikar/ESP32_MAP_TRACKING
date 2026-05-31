#ifndef WEBSERVER_MODULE_H
#define WEBSERVER_MODULE_H

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <esp_task_wdt.h>
#include "../config.h"
#include "gps_module.h"
#include "webpage_renderer.h"
#include "webpage_settings.h"
#include "config_manager.h"
#include "network_module.h"

// Workaround: ESP32 Server class requires begin(uint16_t) override
class ESP32EthernetServer : public EthernetServer {
public:
    ESP32EthernetServer(uint16_t port) : EthernetServer(port) {}
    void begin(uint16_t port = 0) { EthernetServer::begin(); }
};

class WebServerModule {
    static constexpr uint32_t SOCKET_READ_TIMEOUT_MS = 2000;  // per readStringUntil/readBytes
    static constexpr uint32_t HEADER_TIMEOUT_MS = 3000;       // max wait per header line
    static constexpr uint32_t BODY_TIMEOUT_MS = 5000;         // idle timeout while reading body

public:
    WebServerModule(uint16_t port) : _server(port), _configMgr(nullptr), _network(nullptr) {}

    void begin() {
        _server.begin();
    }

    void setConfigManager(ConfigManager* configMgr) {
        _configMgr = configMgr;
    }

    void setNetwork(NetworkModule* network) {
        _network = network;
    }

    void handle(const GPSData& gpsData, bool gpsValid) {
        EthernetClient client = _server.available();
        if (!client) return;

        client.setTimeout(SOCKET_READ_TIMEOUT_MS);

        // Read the request
        String requestLine = "";
        String body = "";
        String updateToken = "";
        int contentLength = 0;
        bool isPost = false;

        // Read headers. Wait for each line with a timeout so a request split
        // across multiple TCP segments isn't truncated when available() is
        // momentarily 0 mid-request.
        bool headersComplete = false;
        while (true) {
            if (!waitForData(client, HEADER_TIMEOUT_MS)) break;

            String line = client.readStringUntil('\n');
            line.trim();

            if (requestLine.length() == 0) {
                requestLine = line;
                isPost = line.startsWith("POST");
            }

            if (line.startsWith("Content-Length:")) {
                contentLength = line.substring(15).toInt();
            }

            if (line.startsWith("X-Update-Token:")) {
                updateToken = line.substring(15);
                updateToken.trim();
            }

            if (line.length() == 0) {
                headersComplete = true;
                break;
            }
        }

        if (!headersComplete) {
            client.stop();  // malformed or timed-out request
            return;
        }

        // Firmware uploads must be streamed (too large to buffer in a String),
        // so branch before the generic body read.
        if (isPost && requestLine.startsWith("POST /api/firmware/update")) {
            handleFirmwareUpdate(client, contentLength, updateToken);
            return;  // response + reboot handled inside
        }

        // Read full body for other POSTs, waiting for all Content-Length bytes.
        if (isPost && contentLength > 0) {
            body = readBody(client, contentLength, BODY_TIMEOUT_MS);
        }

        // Parse route from request line
        String route = "/";
        int spaceIdx = requestLine.indexOf(' ');
        if (spaceIdx > 0) {
            int secondSpace = requestLine.indexOf(' ', spaceIdx + 1);
            if (secondSpace > spaceIdx) {
                route = requestLine.substring(spaceIdx + 1, secondSpace);
            }
        }

        // Route handling
        if (route == "/" || route == "/index.html") {
            WebPage::render(client, gpsData, gpsValid, _configMgr);
        }
        else if (route == "/settings") {
            if (_configMgr) {
                WebPageSettings::render(client, *_configMgr);
            } else {
                sendError(client, 500, "Config manager not initialized");
            }
        }
        else if (route == "/api/config" && !isPost) {
            sendConfigJson(client);
        }
        else if (route == "/api/config" && isPost) {
            handleSaveConfig(client, body);
        }
        else if (route == "/api/config/reset" && isPost) {
            handleResetConfig(client);
        }
        else if (route == "/api/server/ping" && isPost) {
            handlePing(client, body);
        }
        else if (route == "/api/server/sync" && isPost) {
            handleSync(client);
        }
        else if (route == "/api/device/status" && !isPost) {
            unsigned long up = millis() / 1000;
            char buffer[224];
            snprintf(buffer, sizeof(buffer),
                "{\"version\":\"%s\",\"build\":\"%s\","
                "\"uptime\":\"%02u:%02u:%02u\","
                "\"network\":%s,\"gps\":%s,\"webhook\":%s}",
                FIRMWARE_VERSION, FIRMWARE_BUILD,
                (unsigned)(up / 3600), (unsigned)((up % 3600) / 60), (unsigned)(up % 60),
                (_network && _network->isConnected()) ? "true" : "false",
                gpsValid ? "true" : "false",
                (_configMgr && _configMgr->isEnabled()) ? "true" : "false");
            sendJsonResponse(client, 200, buffer);
        }
        else {
            sendError(client, 404, "Not Found");
        }

        delay(1);
        client.stop();
    }

private:
    ESP32EthernetServer _server;
    ConfigManager* _configMgr;

    // Block until the client has data, the connection drains/closes, or timeout.
    bool waitForData(EthernetClient& client, uint32_t timeoutMs) {
        uint32_t start = millis();
        while (client.available() == 0) {
            if (!client.connected() && client.available() == 0) return false;
            if (millis() - start > timeoutMs) return false;
            esp_task_wdt_reset();
            yield();
        }
        return true;
    }

    // Read exactly contentLength bytes, tolerating data arriving in segments.
    // Aborts on idle timeout or premature disconnect.
    String readBody(EthernetClient& client, int contentLength, uint32_t timeoutMs) {
        String body;
        body.reserve(contentLength + 1);
        uint32_t lastData = millis();
        while ((int)body.length() < contentLength) {
            if (client.available() > 0) {
                body += (char)client.read();
                lastData = millis();
            } else {
                if (!client.connected() && client.available() == 0) break;
                if (millis() - lastData > timeoutMs) break;
                esp_task_wdt_reset();
                yield();
            }
        }
        return body;
    }

    void sendJsonResponse(EthernetClient& client, int code, const char* json) {
        client.print("HTTP/1.1 ");
        client.print(code);
        client.println(" OK");
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.println();
        client.print(json);
    }

    void sendError(EthernetClient& client, int code, const char* message) {
        client.print("HTTP/1.1 ");
        client.print(code);
        client.println(" Error");
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.println();
        client.print("{\"success\":false,\"error\":\"");
        client.print(message);
        client.println("\"}");
    }

    void sendConfigJson(EthernetClient& client) {
        if (!_configMgr) {
            sendError(client, 500, "Config not available");
            return;
        }

        StaticJsonDocument<384> doc;
        doc["host"] = _configMgr->getHost();
        doc["port"] = _configMgr->getPort();
        doc["path"] = _configMgr->getPath();
        doc["pingPath"] = _configMgr->getPingPath();
        doc["syncPath"] = _configMgr->getSyncPath();
        doc["enabled"] = _configMgr->isEnabled();

        char buffer[384];
        serializeJson(doc, buffer);
        sendJsonResponse(client, 200, buffer);
    }

    void handleSaveConfig(EthernetClient& client, const String& body) {
        if (!_configMgr) {
            sendError(client, 500, "Config not available");
            return;
        }

        StaticJsonDocument<384> doc;
        DeserializationError error = deserializeJson(doc, body);

        if (error) {
            sendError(client, 400, "Invalid JSON");
            return;
        }

        if (doc.containsKey("host")) {
            _configMgr->setHost(doc["host"].as<const char*>());
        }
        if (doc.containsKey("port")) {
            _configMgr->setPort(doc["port"].as<uint16_t>());
        }
        if (doc.containsKey("path")) {
            _configMgr->setPath(doc["path"].as<const char*>());
        }
        if (doc.containsKey("pingPath")) {
            _configMgr->setPingPath(doc["pingPath"].as<const char*>());
        }
        if (doc.containsKey("syncPath")) {
            _configMgr->setSyncPath(doc["syncPath"].as<const char*>());
        }
        if (doc.containsKey("enabled")) {
            _configMgr->setEnabled(doc["enabled"].as<bool>());
        }

        if (_configMgr->save()) {
            sendJsonResponse(client, 200, "{\"success\":true}");
        } else {
            sendError(client, 500, "Failed to save");
        }
    }

    void handleResetConfig(EthernetClient& client) {
        if (!_configMgr) {
            sendError(client, 500, "Config not available");
            return;
        }

        _configMgr->resetToDefaults();
        sendJsonResponse(client, 200, "{\"success\":true}");
    }

    // Test reachability of host/port supplied in the request body (form values).
    void handlePing(EthernetClient& client, const String& body) {
        if (!_network) {
            sendError(client, 500, "Network not available");
            return;
        }

        StaticJsonDocument<128> doc;
        if (deserializeJson(doc, body)) {
            sendError(client, 400, "Invalid JSON");
            return;
        }

        const char* host = doc["host"] | "";
        uint16_t port = doc["port"] | 0;
        const char* path = doc["path"] | "/";
        if (strlen(host) == 0 || port == 0) {
            sendError(client, 400, "Missing host or port");
            return;
        }

        const bool reachable = _network->checkReachable(host, port, path);
        sendJsonResponse(client, 200, reachable ? "{\"reachable\":true}" : "{\"reachable\":false}");
    }

    // Push device identity to the saved server config.
    void handleSync(EthernetClient& client) {
        if (!_network || !_configMgr) {
            sendError(client, 500, "Network or config not available");
            return;
        }

        char deviceId[24];
        uint64_t chipId = ESP.getEfuseMac();
        snprintf(deviceId, sizeof(deviceId), "%s%06X", DEVICE_ID_PREFIX, (uint32_t)(chipId & 0xFFFFFF));

        const HttpResponse res = _network->syncDeviceInfo(
            _configMgr->getHost(), _configMgr->getSyncPath(), _configMgr->getPort(),
            deviceId, _configMgr->getOtaToken());

        StaticJsonDocument<96> doc;
        doc["success"] = res.success;
        doc["statusCode"] = res.statusCode;
        char buffer[96];
        serializeJson(doc, buffer);
        sendJsonResponse(client, res.success ? 200 : 502, buffer);
    }

    // Stream a raw firmware binary from the request body into flash via the
    // Update library, then reboot into the new image on success.
    void handleFirmwareUpdate(EthernetClient& client, int contentLength, const String& token) {
        if (!_configMgr) {
            sendError(client, 500, "Config not available");
            return;
        }

        if (token != _configMgr->getOtaToken()) {
            Serial.println("[OTA] Rejected: bad token");
            sendError(client, 401, "Unauthorized");
            return;
        }

        if (contentLength <= 0) {
            sendError(client, 400, "Missing or invalid Content-Length");
            return;
        }

        Serial.printf("[OTA] Starting update, size=%d bytes\n", contentLength);

        if (!Update.begin((size_t)contentLength)) {
            Serial.printf("[OTA] begin failed: %s\n", Update.errorString());
            sendError(client, 500, "Update begin failed");
            return;
        }

        uint8_t buf[1024];
        size_t written = 0;
        uint32_t lastData = millis();
        const uint32_t IDLE_TIMEOUT = 10000;  // abort if no data for 10s

        while (written < (size_t)contentLength) {
            esp_task_wdt_reset();

            int avail = client.available();
            if (avail > 0) {
                size_t toRead = avail < (int)sizeof(buf) ? (size_t)avail : sizeof(buf);
                if (written + toRead > (size_t)contentLength) {
                    toRead = (size_t)contentLength - written;
                }
                int n = client.read(buf, toRead);
                if (n > 0) {
                    if (Update.write(buf, n) != (size_t)n) {
                        Serial.printf("[OTA] write failed: %s\n", Update.errorString());
                        Update.abort();
                        sendError(client, 500, "Update write failed");
                        return;
                    }
                    written += n;
                    lastData = millis();
                }
            } else {
                if (!client.connected()) break;
                if (millis() - lastData > IDLE_TIMEOUT) {
                    Serial.println("[OTA] upload timeout");
                    break;
                }
                yield();
            }
        }

        if (written != (size_t)contentLength || !Update.end(true)) {
            Serial.printf("[OTA] failed: wrote %u/%d, err=%s\n",
                          (unsigned)written, contentLength, Update.errorString());
            Update.abort();
            sendError(client, 500, "Update incomplete");
            return;  // old firmware stays active
        }

        Serial.printf("[OTA] Success: %u bytes. Rebooting...\n", (unsigned)written);

        char buffer[64];
        snprintf(buffer, sizeof(buffer), "{\"success\":true,\"bytes\":%u}", (unsigned)written);
        sendJsonResponse(client, 200, buffer);
        client.flush();
        client.stop();

        delay(200);
        ESP.restart();
    }

    NetworkModule* _network;
};

#endif // WEBSERVER_MODULE_H
