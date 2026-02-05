#ifndef WEBSERVER_MODULE_H
#define WEBSERVER_MODULE_H

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include "../config.h"
#include "gps_module.h"

// Workaround: ESP32 Server class requires begin(uint16_t) override
class ESP32EthernetServer : public EthernetServer {
public:
    ESP32EthernetServer(uint16_t port) : EthernetServer(port) {}
    void begin(uint16_t port = 0) { EthernetServer::begin(); }
};

class WebServerModule {
public:
    WebServerModule(uint16_t port) : _server(port) {}

    void begin() {
        _server.begin();
    }

    void handle(const GPSData& gpsData, bool gpsValid) {
        EthernetClient client = _server.available();
        if (!client) return;

        boolean blankLine = false;

        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                if (c == '\n' && blankLine) {
                    sendWebPage(client, gpsData, gpsValid);
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
    ESP32EthernetServer _server;

    void sendWebPage(EthernetClient& client, const GPSData& gpsData, bool gpsValid) {
        double lat = gpsValid ? gpsData.latitude : DEFAULT_LAT;
        double lng = gpsValid ? gpsData.longitude : DEFAULT_LNG;
        double spd = gpsData.speed;
        double alt = gpsData.altitude;
        double crs = gpsData.course;
        uint8_t sat = gpsData.satellites;
        unsigned long upt = millis() / 1000;

        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");
        client.println();

        client.println("<!DOCTYPE HTML><html><head>");
        client.println("<meta charset='UTF-8'>");
        client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
        client.println("<title>Starlink GPS OS</title>");

        client.println("<link rel='stylesheet' href='https://unpkg.com/leaflet@1.9.4/dist/leaflet.css' />");
        client.println("<script src='https://unpkg.com/leaflet@1.9.4/dist/leaflet.js'></script>");
        client.println("<link href='https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700&family=Roboto+Mono:wght@400;700&display=swap' rel='stylesheet'>");

        client.println("<style>");
        client.println("body { margin: 0; padding: 0; background: #000; font-family: 'Roboto Mono', monospace; overflow: hidden; }");
        client.println("#map { height: 100vh; width: 100%; z-index: 1; filter: contrast(1.1) saturate(1.2); }");

        client.println(".panel { position: absolute; top: 20px; right: 20px; width: 300px; background: rgba(10, 15, 25, 0.9); ");
        client.println("  backdrop-filter: blur(10px); border: 1px solid #334455; border-radius: 12px; padding: 20px; z-index: 1000; box-shadow: 0 0 20px rgba(0,0,0,0.8); color: #fff; }");

        client.println(".header { display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid #334455; padding-bottom: 10px; margin-bottom: 15px; }");
        client.println(".title { font-family: 'Orbitron', sans-serif; font-weight: 700; color: #00ddff; font-size: 18px; letter-spacing: 1px; }");
        client.println(".live-dot { height: 10px; width: 10px; background-color: #00ff00; border-radius: 50%; display: inline-block; box-shadow: 0 0 5px #00ff00; margin-right: 5px;}");
        client.println(".live-text { color: #00ff00; font-size: 12px; font-weight: bold; }");

        client.println(".row { display: flex; justify-content: space-between; margin-bottom: 8px; font-size: 13px; }");
        client.println(".label { color: #8899aa; }");
        client.println(".value { color: #fff; font-weight: bold; text-align: right; }");
        client.println(".green { color: #00ff00; }");
        client.println(".cyan { color: #00ddff; }");
        client.println(".white { color: #ffffff; }");

        client.println(".time-box { background: #000; border: 1px solid #334455; padding: 8px; text-align: center; border-radius: 4px; margin: 15px 0; font-size: 14px; color: #fff; }");
        client.println(".time-icon { display: inline-block; width: 10px; height: 10px; border-radius: 50%; background: #fff; margin-right: 5px; }");

        client.println(".signal-section { margin: 15px 0; }");
        client.println(".bar-container { height: 6px; background: #333; border-radius: 3px; margin-top: 5px; overflow: hidden; }");
        client.println(".bar-fill { height: 100%; background: #00ff00; width: 0%; transition: width 0.5s; box-shadow: 0 0 10px #00ff00; }");

        client.println(".custom-marker svg { filter: drop-shadow(0 0 5px #00ff00); }");
        client.println("@keyframes spin { 100% { transform: rotate(360deg); } }");
        client.println(".spin-ring { animation: spin 4s linear infinite; transform-origin: center; }");

        client.println("</style></head><body>");

        client.println("<div id='map'></div>");
        client.println("<div class='panel'>");

        client.println("<div class='header'>");
        client.println("  <div class='title'>STARLINK OS</div>");
        client.println("  <div><span class='live-dot'></span><span class='live-text'>LIVE</span></div>");
        client.println("</div>");

        client.println("<div class='row'><span class='label'>GPS STATUS</span><span class='value green' id='gps-status'>ONLINE</span></div>");
        client.println("<div class='row'><span class='label'>LOC</span><span class='value white' id='loc-txt'>...</span></div>");
        client.println("<div class='row'><span class='label'>UPTIME</span><span class='value white' id='uptime'>0m 0s</span></div>");

        client.println("<div class='time-box'><span class='time-icon'></span><span id='clock'>--:--:-- WIB</span></div>");

        client.println("<div class='signal-section'>");
        client.println("  <div class='row'><span class='label'>SATELLITES</span><span class='value white' id='sig-txt'>0 SAT</span></div>");
        client.println("  <div class='bar-container'><div class='bar-fill' id='sig-bar'></div></div>");
        client.println("</div>");

        client.println("<div class='row'><span class='label'>⬆ ALTITUDE</span><span class='value cyan' id='alt-txt'>0 m</span></div>");
        client.println("<div class='row'><span class='label'>⬇ SPEED</span><span class='value green' id='spd-txt'>0 km/h</span></div>");
        client.println("<div class='row'><span class='label'>PING (HEAD)</span><span class='value white' id='crs-txt'>0°</span></div>");
        client.println("<div class='row'><span class='label'>TEMP</span><span class='value green'>NORMAL</span></div>");

        client.println("</div>");

        // JavaScript
        client.println("<script>");

        client.print("var map = L.map('map', {zoomControl: false}).setView([");
        client.print(lat, 6); client.print(","); client.print(lng, 6);
        client.println("], 18);");

        client.println("L.tileLayer('https://mt1.google.com/vt/lyrs=y&x={x}&y={y}&z={z}', { attribution: '' }).addTo(map);");

        client.println("var iconSvg = `<svg width='60' height='60' viewBox='0 0 100 100' fill='none' xmlns='http://www.w3.org/2000/svg'>");
        client.println("  <circle cx='50' cy='50' r='45' stroke='#00ff00' stroke-width='2' opacity='0.5' />");
        client.println("  <circle cx='50' cy='50' r='35' stroke='#00ff00' stroke-width='1' stroke-dasharray='5 5' class='spin-ring' />");
        client.println("  <path d='M50 20 L80 80 L50 70 L20 80 Z' fill='#00ff00' />");
        client.println("</svg>`;");

        client.println("var starlinkIcon = L.divIcon({ html: iconSvg, className: 'custom-marker', iconSize: [60,60], iconAnchor: [30,30] });");

        client.print("var marker = L.marker([");
        client.print(lat, 6); client.print(","); client.print(lng, 6);
        client.println("], {icon: starlinkIcon}).addTo(map);");

        // Set panel values from server-side variables
        client.print("document.getElementById('loc-txt').innerText = '");
        client.print(lat, 5); client.print(", "); client.print(lng, 5);
        client.println("';");

        client.print("document.getElementById('sig-bar').style.width = '");
        client.print(min((int)sat * 8.5, 100.0), 0);
        client.println("%';");

        client.print("document.getElementById('sig-txt').innerText = '");
        client.print(sat); client.println(" SAT';");

        client.print("document.getElementById('spd-txt').innerText = '");
        client.print(spd, 1); client.println(" km/h';");

        client.print("document.getElementById('alt-txt').innerText = '");
        client.print(alt, 1); client.println(" m';");

        client.print("document.getElementById('crs-txt').innerText = '");
        client.print(crs, 0); client.println("°';");

        client.print("var upt = "); client.print(upt); client.println(";");
        client.println("document.getElementById('uptime').innerText = Math.floor(upt/60) + 'm ' + (upt%60) + 's';");

        client.print("document.querySelector('.custom-marker svg path').setAttribute('transform', 'rotate(");
        client.print(crs, 0); client.println(" 50 50)');");

        client.println("setInterval(() => {");
        client.println("  var now = new Date();");
        client.println("  var timeString = now.toLocaleTimeString('id-ID', { hour12: false }) + ' WIB';");
        client.println("  document.getElementById('clock').innerText = timeString;");
        client.println("}, 1000);");
        client.println("</script></body></html>");
    }
};

#endif // WEBSERVER_MODULE_H
