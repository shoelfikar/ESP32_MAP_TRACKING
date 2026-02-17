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
#include <esp_task_wdt.h>
#include "config.h"
#include "modules/gps_module.h"
#include "modules/config_manager.h"

#if WIFI_ENABLE
#include <WiFi.h>
#include "modules/wifi_module.h"
#if WEBSERVER_ENABLE
#include "modules/wifi_webserver_module.h"
#endif
#else
#include <SPI.h>
#include <Ethernet.h>
#include "modules/network_module.h"
#if WEBSERVER_ENABLE
#include "modules/webserver_module.h"
#endif
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
        initDeviceId();
        initWatchdog();
        initStatusLED();

        // Initialize configuration manager (loads from NVS)
        _configMgr.begin();

        _state = AppState::NETWORK_CONNECTING;

        if (!initNetwork()) {
            _state = AppState::ERROR_NETWORK;
            printBanner();  // Show banner even if network fails
            log("ERROR: Network initialization failed!");
            return;
        }

        printBanner();

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
        _webServer.handle(_lastGPSData, _lastGPSValid);
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
    ConfigManager _configMgr;

    #if WIFI_ENABLE
    WiFiNetworkModule _network;
    #else
    NetworkModule _network{W5500_CS_PIN, W5500_RST_PIN};
    #endif

    #if WEBSERVER_ENABLE
    #if WIFI_ENABLE
    WiFiWebServerModule _webServer{WEBSERVER_PORT};
    #else
    WebServerModule _webServer{WEBSERVER_PORT};
    #endif
    GPSData _lastGPSData;
    bool _lastGPSValid = false;
    #endif

    // State variables
    AppState _state = AppState::INIT;
    uint32_t _lastSendTime = 0;
    uint32_t _currentInterval = SEND_INTERVAL_NORMAL;
    uint8_t _networkRetryCount = 0;

    // Device ID (prefix + chip ID)
    char _deviceId[24];

    #if !WIFI_ENABLE
    const uint8_t _mac[6] = MAC_ADDR;
    #endif

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

    void initDeviceId() {
        uint64_t chipId = ESP.getEfuseMac();
        snprintf(_deviceId, sizeof(_deviceId), "%s%06X",
                 DEVICE_ID_PREFIX,
                 (uint32_t)(chipId & 0xFFFFFF));
    }

    void printBanner() {
        char ipBuffer[16] = "Not connected";
        if (_network.isConnected()) {
            _network.getLocalIP(ipBuffer, sizeof(ipBuffer));
        }

        log("\n========================================");
        log("  ESP32 GPS Tracker v" FIRMWARE_VERSION);
        log("  Build: " FIRMWARE_BUILD);
        log("  Device: " + String(_deviceId));
        log("  IP: " + String(ipBuffer));
        #if WEBSERVER_ENABLE
        log("  Web: http://" + String(ipBuffer) + ":" + String(WEBSERVER_PORT));
        #endif
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
        #if WIFI_ENABLE
        log("Connecting to WiFi...");
        log("  SSID: " + String(_configMgr.getWifiSsid()));
        #else
        log("Initializing Ethernet...");
        #endif

        for (uint8_t retry = 0; retry < MAX_NETWORK_RETRIES; retry++) {
            if (retry > 0) {
                log("Retry " + String(retry) + "/" + String(MAX_NETWORK_RETRIES));
                delay(RETRY_DELAY_MS);
            }

            #if WIFI_ENABLE
            if (_network.begin(_configMgr.getWifiSsid(), _configMgr.getWifiPassword())) {
            #else
            if (_network.begin(_mac)) {
            #endif
                #if WEBSERVER_ENABLE
                _webServer.setConfigManager(&_configMgr);
                _webServer.begin();
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

        // Check if webhook is enabled
        if (!_configMgr.isEnabled()) {
            log("Webhook disabled, skipping send");
            _currentInterval = hasValidFix ? SEND_INTERVAL_NORMAL : SEND_INTERVAL_NO_FIX;
            return;
        }

        // Send data to server using config values
        setLED(true);
        const HttpResponse response = _network.sendGPSData(
            _configMgr.getHost(),
            _configMgr.getPath(),
            _configMgr.getPort(),
            _deviceId, gpsData
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

        #if WIFI_ENABLE
        if (_network.begin(_configMgr.getWifiSsid(), _configMgr.getWifiPassword())) {
        #else
        if (_network.begin(_mac)) {
        #endif
            _state = AppState::RUNNING;
            log("Reconnected successfully");
            _networkRetryCount = 0;
        } else {
            log("Reconnection failed");
            delay(RETRY_DELAY_MS);
        }
    }

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
