#ifndef WEBPAGE_RENDERER_H
#define WEBPAGE_RENDERER_H

#include <Arduino.h>
#include "../config.h"
#include "gps_module.h"

namespace WebPage {

inline void render(Print& out, const GPSData& gpsData, bool gpsValid) {
    double lat = gpsValid ? gpsData.latitude : DEFAULT_LAT;
    double lng = gpsValid ? gpsData.longitude : DEFAULT_LNG;
    double spd = gpsData.speed;
    double alt = gpsData.altitude;
    double crs = gpsData.course;
    uint8_t sat = gpsData.satellites;
    unsigned long upt = millis() / 1000;

    out.println("HTTP/1.1 200 OK");
    out.println("Content-Type: text/html");
    out.println("Connection: close");
    out.println();

    out.println("<!DOCTYPE HTML><html><head>");
    out.println("<meta charset='UTF-8'>");
    out.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
    out.println("<title>Starlink GPS OS</title>");

    out.println("<link rel='stylesheet' href='https://unpkg.com/leaflet@1.9.4/dist/leaflet.css' />");
    out.println("<script src='https://unpkg.com/leaflet@1.9.4/dist/leaflet.js'></script>");
    out.println("<link href='https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700&family=Roboto+Mono:wght@400;700&display=swap' rel='stylesheet'>");

    out.println("<style>");
    out.println("body { margin: 0; padding: 0; background: #000; font-family: 'Roboto Mono', monospace; overflow: hidden; }");
    out.println("#map { height: 100vh; width: 100%; z-index: 1; filter: contrast(1.1) saturate(1.2); }");

    out.println(".panel { position: absolute; top: 20px; right: 20px; width: 300px; background: rgba(10, 15, 25, 0.9); ");
    out.println("  backdrop-filter: blur(10px); border: 1px solid #334455; border-radius: 12px; padding: 20px; z-index: 1000; box-shadow: 0 0 20px rgba(0,0,0,0.8); color: #fff; }");

    out.println(".header { display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid #334455; padding-bottom: 10px; margin-bottom: 15px; }");
    out.println(".title { font-family: 'Orbitron', sans-serif; font-weight: 700; color: #00ddff; font-size: 18px; letter-spacing: 1px; }");
    out.println(".live-dot { height: 10px; width: 10px; background-color: #00ff00; border-radius: 50%; display: inline-block; box-shadow: 0 0 5px #00ff00; margin-right: 5px;}");
    out.println(".live-text { color: #00ff00; font-size: 12px; font-weight: bold; }");

    out.println(".row { display: flex; justify-content: space-between; margin-bottom: 8px; font-size: 13px; }");
    out.println(".label { color: #8899aa; }");
    out.println(".value { color: #fff; font-weight: bold; text-align: right; }");
    out.println(".green { color: #00ff00; }");
    out.println(".cyan { color: #00ddff; }");
    out.println(".white { color: #ffffff; }");

    out.println(".time-box { background: #000; border: 1px solid #334455; padding: 8px; text-align: center; border-radius: 4px; margin: 15px 0; font-size: 14px; color: #fff; }");
    out.println(".time-icon { display: inline-block; width: 10px; height: 10px; border-radius: 50%; background: #fff; margin-right: 5px; }");

    out.println(".signal-section { margin: 15px 0; }");
    out.println(".bar-container { height: 6px; background: #333; border-radius: 3px; margin-top: 5px; overflow: hidden; }");
    out.println(".bar-fill { height: 100%; background: #00ff00; width: 0%; transition: width 0.5s; box-shadow: 0 0 10px #00ff00; }");

    out.println(".custom-marker svg { filter: drop-shadow(0 0 5px #00ff00); }");
    out.println("@keyframes spin { 100% { transform: rotate(360deg); } }");
    out.println(".spin-ring { animation: spin 4s linear infinite; transform-origin: center; }");

    out.println("</style></head><body>");

    out.println("<div id='map'></div>");
    out.println("<div class='panel'>");

    out.println("<div class='header'>");
    out.println("  <div class='title'>STARLINK OS</div>");
    out.println("  <div><span class='live-dot'></span><span class='live-text'>LIVE</span></div>");
    out.println("</div>");

    out.println("<div class='row'><span class='label'>GPS STATUS</span><span class='value green' id='gps-status'>ONLINE</span></div>");
    out.println("<div class='row'><span class='label'>LOC</span><span class='value white' id='loc-txt'>...</span></div>");
    out.println("<div class='row'><span class='label'>UPTIME</span><span class='value white' id='uptime'>0m 0s</span></div>");

    out.println("<div class='time-box'><span class='time-icon'></span><span id='clock'>--:--:-- WIB</span></div>");

    out.println("<div class='signal-section'>");
    out.println("  <div class='row'><span class='label'>SATELLITES</span><span class='value white' id='sig-txt'>0 SAT</span></div>");
    out.println("  <div class='bar-container'><div class='bar-fill' id='sig-bar'></div></div>");
    out.println("</div>");

    out.println("<div class='row'><span class='label'>⬆ ALTITUDE</span><span class='value cyan' id='alt-txt'>0 m</span></div>");
    out.println("<div class='row'><span class='label'>⬇ SPEED</span><span class='value green' id='spd-txt'>0 km/h</span></div>");
    out.println("<div class='row'><span class='label'>PING (HEAD)</span><span class='value white' id='crs-txt'>0°</span></div>");
    out.println("<div class='row'><span class='label'>TEMP</span><span class='value green'>NORMAL</span></div>");

    out.println("</div>");

    // JavaScript
    out.println("<script>");

    out.print("var map = L.map('map', {zoomControl: false}).setView([");
    out.print(lat, 6); out.print(","); out.print(lng, 6);
    out.println("], 18);");

    out.println("L.tileLayer('https://mt1.google.com/vt/lyrs=y&x={x}&y={y}&z={z}', { attribution: '' }).addTo(map);");

    out.println("var iconSvg = `<svg width='60' height='60' viewBox='0 0 100 100' fill='none' xmlns='http://www.w3.org/2000/svg'>");
    out.println("  <circle cx='50' cy='50' r='45' stroke='#00ff00' stroke-width='2' opacity='0.5' />");
    out.println("  <circle cx='50' cy='50' r='35' stroke='#00ff00' stroke-width='1' stroke-dasharray='5 5' class='spin-ring' />");
    out.println("  <path d='M50 20 L80 80 L50 70 L20 80 Z' fill='#00ff00' />");
    out.println("</svg>`;");

    out.println("var starlinkIcon = L.divIcon({ html: iconSvg, className: 'custom-marker', iconSize: [60,60], iconAnchor: [30,30] });");

    out.print("var marker = L.marker([");
    out.print(lat, 6); out.print(","); out.print(lng, 6);
    out.println("], {icon: starlinkIcon}).addTo(map);");

    // Set panel values from server-side variables
    out.print("document.getElementById('loc-txt').innerText = '");
    out.print(lat, 5); out.print(", "); out.print(lng, 5);
    out.println("';");

    out.print("document.getElementById('sig-bar').style.width = '");
    out.print(min((int)sat * 8.5, 100.0), 0);
    out.println("%';");

    out.print("document.getElementById('sig-txt').innerText = '");
    out.print(sat); out.println(" SAT';");

    out.print("document.getElementById('spd-txt').innerText = '");
    out.print(spd, 1); out.println(" km/h';");

    out.print("document.getElementById('alt-txt').innerText = '");
    out.print(alt, 1); out.println(" m';");

    out.print("document.getElementById('crs-txt').innerText = '");
    out.print(crs, 0); out.println("°';");

    out.print("var upt = "); out.print(upt); out.println(";");
    out.println("document.getElementById('uptime').innerText = Math.floor(upt/60) + 'm ' + (upt%60) + 's';");

    out.print("document.querySelector('.custom-marker svg path').setAttribute('transform', 'rotate(");
    out.print(crs, 0); out.println(" 50 50)');");

    out.println("setInterval(() => {");
    out.println("  var now = new Date();");
    out.println("  var timeString = now.toLocaleTimeString('id-ID', { hour12: false }) + ' WIB';");
    out.println("  document.getElementById('clock').innerText = timeString;");
    out.println("}, 1000);");
    out.println("</script></body></html>");
}

} // namespace WebPage

#endif // WEBPAGE_RENDERER_H
