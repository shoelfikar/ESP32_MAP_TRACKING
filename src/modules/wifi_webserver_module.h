#ifndef WIFI_WEBSERVER_MODULE_H
#define WIFI_WEBSERVER_MODULE_H

#include <Arduino.h>
#include <WiFi.h>
#include "../config.h"
#include "gps_module.h"
#include "webpage_renderer.h"

class WiFiWebServerModule {
public:
    WiFiWebServerModule(uint16_t port) : _server(port) {}

    void begin() {
        _server.begin();
    }

    void handle(const GPSData& gpsData, bool gpsValid) {
        WiFiClient client = _server.available();
        if (!client) return;

        boolean blankLine = false;

        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                if (c == '\n' && blankLine) {
                    WebPage::render(client, gpsData, gpsValid);
                    break;
                }
                if (c == '\n') blankLine = true;
                else if (c != '\r') blankLine = false;
            }
        }
        delay(1);
        client.stop();
    }

private:
    WiFiServer _server;
};

#endif // WIFI_WEBSERVER_MODULE_H
