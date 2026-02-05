/**
 * @file main.cpp
 * @brief ESP32 GPS Tracker with W5500 Ethernet - Production Version
 * @version 2.0.0
 *
 * Features:
 * - Modular architecture
 * - Minimal RAM usage (stack-based allocations)
 * - Watchdog timer for reliability
 * - Status LED indication
 * - Automatic reconnection
 * - Clean error handling
 */

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <esp_task_wdt.h>
#include "config.h"
#include "modules/gps_module.h"
#include "modules/network_module.h"

// Workaround: ESP32 Server class requires begin(uint16_t) override
#if WEBSERVER_ENABLE
class ESP32EthernetServer : public EthernetServer {
public:
    ESP32EthernetServer(uint16_t port) : EthernetServer(port) {}
    void begin(uint16_t port = 0) { EthernetServer::begin(); }
};
#endif

// ============================================
// Application State
// ============================================
enum class AppState : uint8_t {
    INIT = 0,
    NETWORK_CONNECTING,
    RUNNING,
    ERROR_NETWORK,
    ERROR_FATAL
};

// ============================================
// Application Class - Single Instance
// ============================================
class GPSTrackerApp {
public:
    void setup() {
        initSerial();
        printBanner();
        initWatchdog();
        initStatusLED();

        _state = AppState::NETWORK_CONNECTING;

        if (!initNetwork()) {
            _state = AppState::ERROR_NETWORK;
            log("ERROR: Network initialization failed!");
            return;
        }

        if (!initGPS()) {
            log("WARNING: GPS initialization issue");
        }

        _state = AppState::RUNNING;
        _lastSendTime = 0;
        _currentInterval = SEND_INTERVAL_NORMAL;

        log("System initialized successfully");
        logMemoryStatus();
    }

    void loop() {
        // Feed watchdog
        esp_task_wdt_reset();

        // Maintain network connection
        _network.maintain();

        // Check network status
        if (!_network.isConnected()) {
            handleNetworkError();
            return;
        }

        // Handle web server clients
        #if WEBSERVER_ENABLE
        handleWebServer();
        #endif

        // Check if it's time to send data
        const uint32_t now = millis();
        if (now - _lastSendTime >= _currentInterval) {
            processAndSend();
            _lastSendTime = now;
        }

        // Small delay to prevent tight loop
        delay(100);
    }

private:
    // Modules (stack allocated)
    GPSModule _gps{GPS_RX_PIN, GPS_TX_PIN, GPS_BAUD_RATE};
    NetworkModule _network{W5500_CS_PIN, W5500_RST_PIN};

    #if WEBSERVER_ENABLE
    ESP32EthernetServer _webServer{WEBSERVER_PORT};
    GPSData _lastGPSData;
    bool _lastGPSValid = false;
    #endif

    // State variables
    AppState _state = AppState::INIT;
    uint32_t _lastSendTime = 0;
    uint32_t _currentInterval = SEND_INTERVAL_NORMAL;
    uint8_t _networkRetryCount = 0;

    // MAC address (const, stored in flash with PROGMEM on AVR, but ESP32 handles this)
    const uint8_t _mac[6] = MAC_ADDR;

    // ========================================
    // Initialization Methods
    // ========================================

    void initSerial() {
        #if DEBUG_SERIAL
        Serial.begin(DEBUG_BAUD_RATE);
        while (!Serial && millis() < 3000) {
            delay(10);
        }
        #endif
    }

    void printBanner() {
        log("\n========================================");
        log("  ESP32 GPS Tracker v" FIRMWARE_VERSION);
        log("  Build: " FIRMWARE_BUILD);
        log("  Device: " DEVICE_ID);
        log("========================================\n");
    }

    void initWatchdog() {
        esp_task_wdt_init(WATCHDOG_TIMEOUT, true);
        esp_task_wdt_add(NULL);
        log("Watchdog initialized (" + String(WATCHDOG_TIMEOUT) + "s timeout)");
    }

    void initStatusLED() {
        #if LED_ENABLE
        pinMode(LED_BUILTIN_PIN, OUTPUT);
        setLED(false);
        #endif
    }

    bool initNetwork() {
        log("Initializing Ethernet...");

        for (uint8_t retry = 0; retry < MAX_NETWORK_RETRIES; retry++) {
            if (retry > 0) {
                log("Retry " + String(retry) + "/" + String(MAX_NETWORK_RETRIES));
                delay(RETRY_DELAY_MS);
            }

            if (_network.begin(_mac)) {
                char ipBuffer[16];
                _network.getLocalIP(ipBuffer, sizeof(ipBuffer));
                log("Network connected! IP: " + String(ipBuffer));

                #if WEBSERVER_ENABLE
                _webServer.begin();
                log("Web server started on port " + String(WEBSERVER_PORT));
                #endif

                blinkLED(3, 100);
                return true;
            }
        }

        return false;
    }

    bool initGPS() {
        log("Initializing GPS...");
        log("  RX=" + String(GPS_RX_PIN) + ", TX=" + String(GPS_TX_PIN) +
            ", Baud=" + String(GPS_BAUD_RATE));

        if (_gps.begin()) {
            log("GPS module initialized");
            return true;
        }
        return false;
    }

    // ========================================
    // Main Processing
    // ========================================

    void processAndSend() {
        log("\n--- Processing Cycle ---");

        // Read GPS data
        GPSData gpsData;
        const bool hasValidFix = _gps.read(gpsData, GPS_READ_TIMEOUT);

        // Log GPS status
        logGPSStatus(gpsData, hasValidFix);

        // Store last GPS data for web server
        #if WEBSERVER_ENABLE
        _lastGPSData = gpsData;
        _lastGPSValid = hasValidFix;
        #endif

        // Send data to server
        setLED(true);
        const HttpResponse response = _network.sendGPSData(
            SERVER_HOST, SERVER_PATH, SERVER_PORT,
            DEVICE_ID, gpsData
        );
        setLED(false);

        // Process response
        if (response.success) {
            log("Data sent successfully (HTTP " + String(response.statusCode) + ")");
            _currentInterval = hasValidFix ? SEND_INTERVAL_NORMAL : SEND_INTERVAL_NO_FIX;
            _networkRetryCount = 0;
        } else {
            log("Failed to send data (HTTP " + String(response.statusCode) + ")");
            _networkRetryCount++;
        }

        logMemoryStatus();
    }

    void logGPSStatus(const GPSData& data, bool hasValidFix) {
        if (hasValidFix) {
            log("GPS Fix: Valid");
            log("  Lat: " + String(data.latitude, 6));
            log("  Lng: " + String(data.longitude, 6));
            log("  Satellites: " + String(data.satellites));
            log("  Speed: " + String(data.speed, 1) + " km/h");
        } else {
            log("GPS Fix: No valid fix (satellites: " + String(data.satellites) + ")");
        }
    }

    // ========================================
    // Error Handling
    // ========================================

    void handleNetworkError() {
        log("Network disconnected! Attempting reconnection...");
        _state = AppState::ERROR_NETWORK;

        blinkLED(5, 200);

        if (_network.begin(_mac)) {
            _state = AppState::RUNNING;
            log("Reconnected successfully");
            _networkRetryCount = 0;
        } else {
            log("Reconnection failed");
            delay(RETRY_DELAY_MS);
        }
    }

    // ========================================
    // Web Server
    // ========================================

    #if WEBSERVER_ENABLE
    void handleWebServer() {
        EthernetClient client = _webServer.available();
        if (!client) return;

        boolean blankLine = false;

        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                if (c == '\n' && blankLine) {
                    sendWebPage(client);
                    break;
                }
                if (c == '\n') blankLine = true;
                else if (c != '\r') blankLine = false;
            }
        }
        delay(1);
        client.stop();
    }

    void sendWebPage(EthernetClient& client) {
        double lat = _lastGPSValid ? _lastGPSData.latitude : DEFAULT_LAT;
        double lng = _lastGPSValid ? _lastGPSData.longitude : DEFAULT_LNG;
        double spd = _lastGPSData.speed;
        double alt = _lastGPSData.altitude;
        double crs = _lastGPSData.course;
        uint8_t sat = _lastGPSData.satellites;
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
    #endif

    // ========================================
    // Utility Methods
    // ========================================

    void log(const String& message) {
        #if DEBUG_SERIAL
        Serial.println("[" + String(millis() / 1000) + "s] " + message);
        #endif
    }

    void logMemoryStatus() {
        #if DEBUG_SERIAL
        log("Free heap: " + String(ESP.getFreeHeap()) + " bytes");
        #endif
    }

    void setLED(bool state) {
        #if LED_ENABLE
        digitalWrite(LED_BUILTIN_PIN, state ? HIGH : LOW);
        #endif
    }

    void blinkLED(uint8_t times, uint16_t delayMs) {
        #if LED_ENABLE
        for (uint8_t i = 0; i < times; i++) {
            setLED(true);
            delay(delayMs);
            setLED(false);
            delay(delayMs);
        }
        #endif
    }
};

// ============================================
// Single Application Instance
// ============================================
static GPSTrackerApp app;

// ============================================
// Arduino Entry Points
// ============================================
void setup() {
    app.setup();
}

void loop() {
    app.loop();
}
