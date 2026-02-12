#ifndef WEBPAGE_RENDERER_H
#define WEBPAGE_RENDERER_H

/**
 * @file webpage_renderer.h
 * @brief Dashboard View - Resource monitoring webpage renderer
 *
 * Displays ESP32 system info: Memory, CPU, Network, GPS status
 * For map view, use webpage_renderer_map.h instead
 */

#include <Arduino.h>
#include "../config.h"
#include "gps_module.h"

#if WIFI_ENABLE
#include <WiFi.h>
#else
#include <Ethernet.h>
#endif

namespace WebPage {

inline void render(Print& out, const GPSData& gpsData, bool gpsValid) {
    // System info
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t usedPercent = 100 - (freeHeap * 100 / totalHeap);
    uint32_t cpuFreq = ESP.getCpuFreqMHz();
    uint8_t chipRev = ESP.getChipRevision();
    unsigned long uptimeSec = millis() / 1000;
    uint8_t hours = uptimeSec / 3600;
    uint8_t mins = (uptimeSec % 3600) / 60;
    uint8_t secs = uptimeSec % 60;

    // GPS data
    double lat = gpsValid ? gpsData.latitude : DEFAULT_LAT;
    double lng = gpsValid ? gpsData.longitude : DEFAULT_LNG;
    double spd = gpsData.speed;
    double alt = gpsData.altitude;
    double crs = gpsData.course;
    uint8_t sat = gpsData.satellites;

    // Network info
    #if WIFI_ENABLE
    int32_t rssi = WiFi.RSSI();
    const char* netType = "WiFi";
    char ssidBuf[33];
    strncpy(ssidBuf, WiFi.SSID().c_str(), sizeof(ssidBuf) - 1);
    ssidBuf[sizeof(ssidBuf) - 1] = '\0';
    #else
    int32_t rssi = 0;
    const char* netType = "Ethernet";
    const char* ssidBuf = "-";
    #endif

    // Signal quality
    const char* signalQuality = "No Signal";
    #if WIFI_ENABLE
    if (rssi >= -50) { signalQuality = "Excellent"; }
    else if (rssi >= -60) { signalQuality = "Good"; }
    else if (rssi >= -70) { signalQuality = "Fair"; }
    else if (rssi >= -80) { signalQuality = "Weak"; }
    else { signalQuality = "Very Weak"; }
    #else
    signalQuality = "Wired";
    #endif

    // Device ID
    char deviceId[24];
    uint64_t chipId = ESP.getEfuseMac();
    snprintf(deviceId, sizeof(deviceId), "%s%06X", DEVICE_ID_PREFIX, (uint32_t)(chipId & 0xFFFFFF));

    // IP Address
    char ipBuf[16] = "0.0.0.0";
    #if WIFI_ENABLE
    IPAddress ip = WiFi.localIP();
    uint8_t mac[6];
    WiFi.macAddress(mac);
    #else
    IPAddress ip = Ethernet.localIP();
    uint8_t mac[6];
    Ethernet.MACAddress(mac);
    #endif
    snprintf(ipBuf, sizeof(ipBuf), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

    // MAC Address
    char macBuf[18];
    snprintf(macBuf, sizeof(macBuf), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // HTTP Response
    out.println("HTTP/1.1 200 OK");
    out.println("Content-Type: text/html");
    out.println("Connection: close");
    out.println();

    // HTML
    out.println("<!DOCTYPE html><html lang='en'><head>");
    out.println("<meta charset='UTF-8'>");
    out.println("<meta name='viewport' content='width=device-width,initial-scale=1'>");
    out.println("<title>PELNI GPS Tracker</title>");
    out.println("<style>");

    // CSS - Minified for smaller payload
    out.println("*{margin:0;padding:0;box-sizing:border-box}");
    out.println("body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:#0f172a;color:#e2e8f0;min-height:100vh;padding:20px}");
    out.println(".header{text-align:center;margin-bottom:30px}");
    out.println(".header h1{font-size:1.5rem;font-weight:600;color:#38bdf8}");
    out.println(".header .device-id{font-size:.875rem;color:#64748b;margin-top:4px}");
    out.println(".status-bar{display:flex;justify-content:center;gap:20px;margin-bottom:30px;flex-wrap:wrap}");
    out.println(".status-item{display:flex;align-items:center;gap:6px;font-size:.75rem;color:#94a3b8}");
    out.println(".status-dot{width:8px;height:8px;border-radius:50%;background:#22c55e}");
    out.println(".status-dot.warning{background:#eab308}.status-dot.error{background:#ef4444}");
    out.println(".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:16px;max-width:900px;margin:0 auto}");
    out.println(".card{background:#1e293b;border-radius:12px;padding:20px;border:1px solid #334155}");
    out.println(".card-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:16px}");
    out.println(".card-title{font-size:.875rem;color:#94a3b8;font-weight:500}");
    out.println(".card-icon{width:32px;height:32px;display:flex;align-items:center;justify-content:center;border-radius:8px;background:#334155}");
    out.println(".card-icon svg{width:18px;height:18px;stroke:#38bdf8}");
    out.println(".card-value{font-size:2rem;font-weight:700;color:#f1f5f9;line-height:1}");
    out.println(".card-unit{font-size:.875rem;color:#64748b;font-weight:400;margin-left:4px}");
    out.println(".card-detail{margin-top:12px;font-size:.75rem;color:#64748b}");
    out.println(".progress-bar{height:6px;background:#334155;border-radius:3px;margin-top:12px;overflow:hidden}");
    out.println(".progress-fill{height:100%;background:linear-gradient(90deg,#22c55e,#38bdf8);border-radius:3px}");
    out.println(".progress-fill.warning{background:linear-gradient(90deg,#eab308,#f97316)}");
    out.println(".progress-fill.danger{background:linear-gradient(90deg,#ef4444,#f97316)}");
    out.println(".network-badge{display:inline-flex;align-items:center;gap:6px;padding:6px 12px;border-radius:20px;font-size:.75rem;font-weight:600;text-transform:uppercase;letter-spacing:.5px}");
    out.println(".network-badge.wifi{background:rgba(139,92,246,.2);color:#a78bfa;border:1px solid rgba(139,92,246,.3)}");
    out.println(".network-badge.ethernet{background:rgba(34,197,94,.2);color:#4ade80;border:1px solid rgba(34,197,94,.3)}");
    out.println(".network-badge svg{width:14px;height:14px}");
    out.println(".network-info{display:flex;flex-direction:column;gap:8px;margin-top:12px}");
    out.println(".network-row{display:flex;justify-content:space-between;align-items:center;padding:8px 12px;background:#0f172a;border-radius:6px}");
    out.println(".network-label{font-size:.75rem;color:#64748b}");
    out.println(".network-value{font-size:.875rem;color:#e2e8f0;font-weight:500}");
    out.println(".footer{text-align:center;margin-top:30px;font-size:.75rem;color:#475569}");
    out.println("@media(max-width:640px){body{padding:12px}.card{padding:16px}.card-value{font-size:1.5rem}}");

    out.println("</style></head><body>");

    // Header
    out.println("<div class='header'>");
    out.println("<h1>PELNI GPS Tracker</h1>");
    out.print("<div class='device-id'>"); out.print(deviceId); out.print(" | MAC: "); out.print(macBuf); out.println("</div>");
    out.println("</div>");

    // Status Bar
    out.println("<div class='status-bar'>");
    out.print("<div class='status-item'><div class='status-dot'></div><span>Network: "); out.print(netType); out.println("</span></div>");
    out.print("<div class='status-item'><div class='status-dot");
    if (!gpsValid) out.print(" warning");
    out.print("'></div><span>GPS "); out.print(gpsValid ? "Fix" : "No Fix"); out.println("</span></div>");
    out.println("<div class='status-item'><div class='status-dot'></div><span>Online</span></div>");
    out.println("</div>");

    // Grid Start
    out.println("<div class='grid'>");

    // Memory Card
    out.println("<div class='card'>");
    out.println("<div class='card-header'><span class='card-title'>Memory Usage</span>");
    out.println("<div class='card-icon'><svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M9 3v2m6-2v2M9 19v2m6-2v2M5 9H3m2 6H3m18-6h-2m2 6h-2M7 19h10a2 2 0 002-2V7a2 2 0 00-2-2H7a2 2 0 00-2 2v10a2 2 0 002 2zM9 9h6v6H9V9z'/></svg></div></div>");
    out.print("<div class='card-value'>"); out.print(freeHeap / 1024); out.println("<span class='card-unit'>KB</span></div>");
    out.print("<div class='card-detail'>Free Heap: "); out.print(freeHeap); out.println(" bytes</div>");
    out.print("<div class='progress-bar'><div class='progress-fill");
    if (usedPercent > 80) out.print(" danger");
    else if (usedPercent > 60) out.print(" warning");
    out.print("' style='width:"); out.print(usedPercent); out.println("%'></div></div>");
    out.print("<div class='card-detail'>Total: "); out.print(totalHeap / 1024); out.print(" KB | Used: "); out.print(usedPercent); out.println("%</div>");
    out.println("</div>");

    // Uptime Card
    out.println("<div class='card'>");
    out.println("<div class='card-header'><span class='card-title'>Uptime</span>");
    out.println("<div class='card-icon'><svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M12 8v4l3 3m6-3a9 9 0 11-18 0 9 9 0 0118 0z'/></svg></div></div>");
    out.print("<div class='card-value'>");
    if (hours < 10) out.print("0"); out.print(hours); out.print(":");
    if (mins < 10) out.print("0"); out.print(mins); out.print(":");
    if (secs < 10) out.print("0"); out.print(secs);
    out.println("</div>");
    out.println("<div class='card-detail'>Running since boot</div>");
    out.println("</div>");

    // CPU Card
    out.println("<div class='card'>");
    out.println("<div class='card-header'><span class='card-title'>CPU Frequency</span>");
    out.println("<div class='card-icon'><svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M13 10V3L4 14h7v7l9-11h-7z'/></svg></div></div>");
    out.print("<div class='card-value'>"); out.print(cpuFreq); out.println("<span class='card-unit'>MHz</span></div>");
    out.print("<div class='card-detail'>Chip Rev: "); out.print(chipRev); out.println(" | Cores: 2</div>");
    out.println("</div>");

    // Network Status Card
    out.println("<div class='card'>");
    out.println("<div class='card-header'><span class='card-title'>Network Status</span>");
    out.println("<div class='card-icon'><svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M21 12a9 9 0 01-9 9m9-9a9 9 0 00-9-9m9 9H3m9 9a9 9 0 01-9-9m9 9c1.657 0 3-4.03 3-9s-1.343-9-3-9m0 18c-1.657 0-3-4.03-3-9s1.343-9 3-9m-9 9a9 9 0 019-9'/></svg></div></div>");

    #if WIFI_ENABLE
    out.println("<div class='network-badge wifi'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M8.111 16.404a5.5 5.5 0 017.778 0M12 20h.01m-7.08-7.071c3.904-3.905 10.236-3.905 14.141 0M1.394 9.393c5.857-5.857 15.355-5.857 21.213 0'/></svg>");
    out.println("WiFi</div>");
    #else
    out.println("<div class='network-badge ethernet'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M5 12h14M5 12a2 2 0 01-2-2V6a2 2 0 012-2h14a2 2 0 012 2v4a2 2 0 01-2 2M5 12a2 2 0 00-2 2v4a2 2 0 002 2h14a2 2 0 002-2v-4a2 2 0 00-2-2m-2-4h.01M17 16h.01'/></svg>");
    out.println("Ethernet</div>");
    #endif

    out.println("<div class='network-info'>");
    out.print("<div class='network-row'><span class='network-label'>IP Address</span><span class='network-value'>"); out.print(ipBuf); out.println("</span></div>");
    #if WIFI_ENABLE
    out.print("<div class='network-row'><span class='network-label'>SSID</span><span class='network-value'>"); out.print(ssidBuf); out.println("</span></div>");
    out.print("<div class='network-row'><span class='network-label'>Signal</span><span class='network-value'>"); out.print(rssi); out.print(" dBm ("); out.print(signalQuality); out.println(")</span></div>");
    #else
    out.println("<div class='network-row'><span class='network-label'>Connection</span><span class='network-value'>Wired</span></div>");
    #endif
    out.print("<div class='network-row'><span class='network-label'>MAC Address</span><span class='network-value'>"); out.print(macBuf); out.println("</span></div>");
    out.println("</div></div>");

    // Webhook Status Card
    out.println("<div class='card'>");
    out.println("<div class='card-header'><span class='card-title'>Webhook Status</span>");
    out.println("<div class='card-icon'><svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M7 16a4 4 0 01-.88-7.903A5 5 0 1115.9 6L16 6a5 5 0 011 9.9M15 13l-3-3m0 0l-3 3m3-3v12'/></svg></div></div>");
    out.println("<div class='network-badge' style='background:rgba(34,197,94,.2);color:#4ade80;border:1px solid rgba(34,197,94,.3)'>");
    out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor' style='width:14px;height:14px'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M5 13l4 4L19 7'/></svg>");
    out.println("Configured</div>");
    out.println("<div class='network-info'>");
    out.print("<div class='network-row'><span class='network-label'>Host</span><span class='network-value'>"); out.print(SERVER_HOST); out.println("</span></div>");
    out.print("<div class='network-row'><span class='network-label'>Port</span><span class='network-value'>"); out.print(SERVER_PORT); out.println("</span></div>");
    out.print("<div class='network-row'><span class='network-label'>Path</span><span class='network-value'>"); out.print(SERVER_PATH); out.println("</span></div>");
    out.println("</div></div>");

    // GPS Status Card
    out.println("<div class='card'>");
    out.println("<div class='card-header'><span class='card-title'>GPS Status</span>");
    out.println("<div class='card-icon'><svg fill='none' viewBox='0 0 24 24' stroke='currentColor'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M17.657 16.657L13.414 20.9a1.998 1.998 0 01-2.827 0l-4.244-4.243a8 8 0 1111.314 0z'/><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M15 11a3 3 0 11-6 0 3 3 0 016 0z'/></svg></div></div>");

    // GPS Status Badge
    if (gpsValid) {
        out.println("<div class='network-badge' style='background:rgba(34,197,94,.2);color:#4ade80;border:1px solid rgba(34,197,94,.3)'>");
        out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor' style='width:14px;height:14px'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M5 13l4 4L19 7'/></svg>");
        out.println("Fix OK</div>");
    } else {
        out.println("<div class='network-badge' style='background:rgba(234,179,8,.2);color:#facc15;border:1px solid rgba(234,179,8,.3)'>");
        out.println("<svg fill='none' viewBox='0 0 24 24' stroke='currentColor' style='width:14px;height:14px'><path stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z'/></svg>");
        out.println("No Fix</div>");
    }

    out.println("<div class='network-info'>");
    out.print("<div class='network-row'><span class='network-label'>Latitude</span><span class='network-value'>"); out.print(lat, 6); out.println("</span></div>");
    out.print("<div class='network-row'><span class='network-label'>Longitude</span><span class='network-value'>"); out.print(lng, 6); out.println("</span></div>");
    out.print("<div class='network-row'><span class='network-label'>Satellites</span><span class='network-value'>"); out.print(sat); out.println("</span></div>");
    out.print("<div class='network-row'><span class='network-label'>Speed</span><span class='network-value'>"); out.print(spd, 1); out.println(" km/h</span></div>");
    out.print("<div class='network-row'><span class='network-label'>Altitude</span><span class='network-value'>"); out.print(alt, 1); out.println(" m</span></div>");
    out.print("<div class='network-row'><span class='network-label'>Heading</span><span class='network-value'>"); out.print(crs, 1); out.println("&deg;</span></div>");
    out.println("</div></div>");

    // Grid End
    out.println("</div>");

    // Footer
    out.println("<div class='footer'>");
    out.print("PELNI GPS Tracker v"); out.print(FIRMWARE_VERSION); out.print(" | Build: "); out.println(FIRMWARE_BUILD);
    out.println("</div>");

    out.println("</body></html>");
}

} // namespace WebPage

#endif // WEBPAGE_RENDERER_H
