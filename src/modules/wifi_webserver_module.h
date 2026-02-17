#ifndef WIFI_WEBSERVER_MODULE_H
#define WIFI_WEBSERVER_MODULE_H

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "../config.h"
#include "gps_module.h"
#include "webpage_renderer.h"
#include "webpage_settings.h"
#include "webpage_network_settings.h"
#include "config_manager.h"

class WiFiWebServerModule {
public:
    WiFiWebServerModule(uint16_t port) : _server(port), _configMgr(nullptr) {}

    void begin() {
        _server.begin();
    }

    void setConfigManager(ConfigManager* configMgr) {
        _configMgr = configMgr;
    }

    void handle(const GPSData& gpsData, bool gpsValid) {
        WiFiClient client = _server.available();
        if (!client) return;

        // Read the request
        String requestLine = "";
        String request = "";
        String body = "";
        int contentLength = 0;
        bool isPost = false;

        // Read headers
        while (client.connected() && client.available()) {
            String line = client.readStringUntil('\n');
            line.trim();

            if (requestLine.length() == 0) {
                requestLine = line;
                isPost = line.startsWith("POST");
            }

            if (line.startsWith("Content-Length:")) {
                contentLength = line.substring(15).toInt();
            }

            if (line.length() == 0) {
                // End of headers, read body if POST
                if (isPost && contentLength > 0) {
                    body = "";
                    while (body.length() < contentLength && client.available()) {
                        body += (char)client.read();
                    }
                }
                break;
            }
        }

        // Parse route from request line (e.g., "GET /settings HTTP/1.1")
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
            // GET /api/config - return current config as JSON
            sendConfigJson(client);
        }
        else if (route == "/api/config" && isPost) {
            // POST /api/config - save config
            handleSaveConfig(client, body);
        }
        else if (route == "/api/config/reset" && isPost) {
            // POST /api/config/reset - reset to defaults
            handleResetConfig(client);
        }
        else if (route == "/network") {
            // GET /network - Network settings page
            if (_configMgr) {
                WebPageNetworkSettings::render(client, *_configMgr);
            } else {
                sendError(client, 500, "Config manager not initialized");
            }
        }
        else if (route == "/api/wifi/scan" && !isPost) {
            // GET /api/wifi/scan - Scan available WiFi networks
            handleWifiScan(client);
        }
        else if (route == "/api/network" && !isPost) {
            // GET /api/network - Get network config
            sendNetworkConfigJson(client);
        }
        else if (route == "/api/network" && isPost) {
            // POST /api/network - Save network config
            handleSaveNetworkConfig(client, body);
        }
        else if (route == "/api/network/reset" && isPost) {
            // POST /api/network/reset - Reset network config
            handleResetNetworkConfig(client);
        }
        else if (route == "/api/restart" && isPost) {
            // POST /api/restart - Restart device
            handleRestart(client);
        }
        else {
            sendError(client, 404, "Not Found");
        }

        delay(1);
        client.stop();
    }

private:
    WiFiServer _server;
    ConfigManager* _configMgr;

    void sendJsonResponse(WiFiClient& client, int code, const char* json) {
        client.print("HTTP/1.1 ");
        client.print(code);
        client.println(" OK");
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.println();
        client.print(json);
    }

    void sendError(WiFiClient& client, int code, const char* message) {
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

    void sendConfigJson(WiFiClient& client) {
        if (!_configMgr) {
            sendError(client, 500, "Config not available");
            return;
        }

        StaticJsonDocument<256> doc;
        doc["host"] = _configMgr->getHost();
        doc["port"] = _configMgr->getPort();
        doc["path"] = _configMgr->getPath();
        doc["enabled"] = _configMgr->isEnabled();

        char buffer[256];
        serializeJson(doc, buffer);
        sendJsonResponse(client, 200, buffer);
    }

    void handleSaveConfig(WiFiClient& client, const String& body) {
        if (!_configMgr) {
            sendError(client, 500, "Config not available");
            return;
        }

        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, body);

        if (error) {
            sendError(client, 400, "Invalid JSON");
            return;
        }

        // Update config
        if (doc.containsKey("host")) {
            _configMgr->setHost(doc["host"].as<const char*>());
        }
        if (doc.containsKey("port")) {
            _configMgr->setPort(doc["port"].as<uint16_t>());
        }
        if (doc.containsKey("path")) {
            _configMgr->setPath(doc["path"].as<const char*>());
        }
        if (doc.containsKey("enabled")) {
            _configMgr->setEnabled(doc["enabled"].as<bool>());
        }

        // Save to NVS
        if (_configMgr->save()) {
            sendJsonResponse(client, 200, "{\"success\":true}");
        } else {
            sendError(client, 500, "Failed to save");
        }
    }

    void handleResetConfig(WiFiClient& client) {
        if (!_configMgr) {
            sendError(client, 500, "Config not available");
            return;
        }

        _configMgr->resetToDefaults();
        sendJsonResponse(client, 200, "{\"success\":true}");
    }

    void handleWifiScan(WiFiClient& client) {
        // Scan WiFi networks
        int n = WiFi.scanNetworks();

        StaticJsonDocument<1024> doc;
        JsonArray networks = doc.createNestedArray("networks");

        for (int i = 0; i < n && i < 15; i++) {  // Limit to 15 networks
            JsonObject net = networks.createNestedObject();
            net["ssid"] = WiFi.SSID(i);
            net["rssi"] = WiFi.RSSI(i);
            net["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
        }

        WiFi.scanDelete();

        char buffer[1024];
        serializeJson(doc, buffer);
        sendJsonResponse(client, 200, buffer);
    }

    void sendNetworkConfigJson(WiFiClient& client) {
        if (!_configMgr) {
            sendError(client, 500, "Config not available");
            return;
        }

        StaticJsonDocument<256> doc;
        doc["useWifi"] = _configMgr->useWifi();
        doc["ssid"] = _configMgr->getWifiSsid();
        // Don't send password for security

        char buffer[256];
        serializeJson(doc, buffer);
        sendJsonResponse(client, 200, buffer);
    }

    void handleSaveNetworkConfig(WiFiClient& client, const String& body) {
        if (!_configMgr) {
            sendError(client, 500, "Config not available");
            return;
        }

        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, body);

        if (error) {
            sendError(client, 400, "Invalid JSON");
            return;
        }

        // Update config
        if (doc.containsKey("useWifi")) {
            _configMgr->setUseWifi(doc["useWifi"].as<bool>());
        }
        if (doc.containsKey("ssid")) {
            _configMgr->setWifiSsid(doc["ssid"].as<const char*>());
        }
        if (doc.containsKey("password")) {
            _configMgr->setWifiPassword(doc["password"].as<const char*>());
        }

        // Save to NVS
        if (_configMgr->saveNetwork()) {
            sendJsonResponse(client, 200, "{\"success\":true}");
            // Schedule restart
            delay(100);
            ESP.restart();
        } else {
            sendError(client, 500, "Failed to save");
        }
    }

    void handleResetNetworkConfig(WiFiClient& client) {
        if (!_configMgr) {
            sendError(client, 500, "Config not available");
            return;
        }

        _configMgr->resetNetworkToDefaults();
        sendJsonResponse(client, 200, "{\"success\":true}");
    }

    void handleRestart(WiFiClient& client) {
        sendJsonResponse(client, 200, "{\"success\":true}");
        delay(100);
        ESP.restart();
    }
};

#endif // WIFI_WEBSERVER_MODULE_H
