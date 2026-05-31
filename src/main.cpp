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

#include <SPI.h>
#include <esp_mac.h>
#include <esp_ota_ops.h>
#include <Ethernet.h>
#include "modules/network_module.h"
#include "modules/discovery_module.h"
#if WEBSERVER_ENABLE
#include "modules/webserver_module.h"
#endif

// ============================================
// Application State
// ============================================
enum class AppState : uint8_t {
    INIT = 0,
    NETWORK_CONNECTING,
    RUNNING,
    NO_SERVER,        // network ok, but server not yet discovered/configured
    ERROR_NETWORK,
    ERROR_FATAL
};

// Backoff schedule for re-discovery while in NO_SERVER state (ms).
// Caps at the last entry — keep retrying at 60s intervals indefinitely.
static const uint32_t DISCOVERY_BACKOFF_MS[] = { 5000, 10000, 30000, 60000 };
static constexpr size_t DISCOVERY_BACKOFF_COUNT =
    sizeof(DISCOVERY_BACKOFF_MS) / sizeof(DISCOVERY_BACKOFF_MS[0]);

// ============================================
// Application Class - Single Instance
// ============================================
class GPSTrackerApp {
public:
    void setup() {
        initSerial();
        initDeviceId();
        initMacAddress();
        initWatchdog();
        initStatusLED();

        // Initialize configuration manager (loads from NVS)
        _configMgr.begin();

        _state = AppState::NETWORK_CONNECTING;

        const bool netOk = initNetwork();

        // OTA health gate: if this boot is a freshly-uploaded firmware on trial,
        // confirm it (cancel rollback) when the network is up, or roll back to
        // the previous firmware if it failed to come online.
        verifyOtaHealth(netOk);

        if (!netOk) {
            _state = AppState::ERROR_NETWORK;
            printBanner();  // Show banner even if network fails
            log("ERROR: Network initialization failed!");
            return;
        }

        printBanner();

        if (!initGPS()) {
            log("WARNING: GPS initialization issue");
        }

        // Resolve which server to talk to: cached → manual → UDP discovery.
        // Sets _state to RUNNING (resolved) or NO_SERVER (need retry or manual config).
        resolveServer();

        _lastSendTime = 0;
        _currentInterval = SEND_INTERVAL_NORMAL;

        log(_state == AppState::RUNNING
                ? String("System initialized successfully (server resolved)")
                : String("System initialized in NO_SERVER state — retry backoff active"));
        logMemoryStatus();
    }

    void loop() {
        // Feed watchdog
        esp_task_wdt_reset();

        // Continuously feed the GPS parser (non-blocking)
        _gps.update();

        // Maintain network connection
        _network.maintain();

        // Check network status
        if (!_network.isConnected()) {
            handleNetworkError();
            return;
        }

        // Handle web server clients (works in BOTH RUNNING and NO_SERVER so
        // operator can input manual config when discovery fails).
        #if WEBSERVER_ENABLE
        _lastGPSValid = _gps.getFix(_lastGPSData);
        _webServer.setServerState(stateLabel());
        _webServer.handle(_lastGPSData, _lastGPSValid);

        // Drain webserver-triggered requests (loose coupling — webserver doesn't
        // own the discovery module).
        if (_webServer.takeTrustResetRequest()) {
            log("Web request: reset TOFU trust fingerprint");
            _configMgr.resetTrust();
            _configMgr.save();
        }
        if (_webServer.takeDiscoveryRequest()) {
            log("Web request: trigger discovery now");
            if (attemptDiscovery()) {
                _state = AppState::RUNNING;
                _lastSendTime = 0;
                _consecutiveSendFails = 0;
            } else if (!_configMgr.isResolved()) {
                _state = AppState::NO_SERVER;
                _discoveryBackoffIdx = 0;
                _nextDiscoveryAttemptMs = millis() + DISCOVERY_BACKOFF_MS[0];
            }
        }
        #endif

        // NO_SERVER: serve web UI, blink LED, retry discovery with backoff.
        // Do NOT send GPS or device sync — server identity not verified.
        if (_state == AppState::NO_SERVER) {
            updateNoServerLED();
            tickNoServerRetry();
            delay(10);
            return;
        }

        // RUNNING path: webhook send loop
        const uint32_t now = millis();
        if (now - _lastSendTime >= _currentInterval && !_network.isSending()) {
            startSend();
            _lastSendTime = now;
        }

        // Advance the non-blocking send and handle its result when ready
        _network.pollSend();
        HttpResponse sendResult;
        if (_network.takeResult(sendResult)) {
            setLED(false);
            handleSendResult(sendResult);
        }

        // Small delay to prevent a tight spin; kept short so the GPS parser is
        // fed often and the web server stays responsive.
        delay(10);
    }

private:
    // Modules (stack allocated)
    GPSModule _gps{GPS_RX_PIN, GPS_TX_PIN, GPS_BAUD_RATE};
    ConfigManager _configMgr;

    NetworkModule _network{W5500_CS_PIN, W5500_RST_PIN};
    DiscoveryModule _discovery;

    #if WEBSERVER_ENABLE
    WebServerModule _webServer{WEBSERVER_PORT};
    GPSData _lastGPSData;
    bool _lastGPSValid = false;
    #endif

    // State variables
    AppState _state = AppState::INIT;
    uint32_t _lastSendTime = 0;
    uint32_t _currentInterval = SEND_INTERVAL_NORMAL;
    uint8_t _networkRetryCount = 0;
    uint8_t _consecutiveSendFails = 0;        // for re-discovery trigger
    bool _sendFixValid = false;  // fix validity captured for the in-flight send

    // NO_SERVER retry state
    uint8_t _discoveryBackoffIdx = 0;
    uint32_t _nextDiscoveryAttemptMs = 0;

    // Device ID (prefix + chip ID)
    char _deviceId[24];

    uint8_t _mac[6];

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

    void initMacAddress() {
        // Derive W5500 MAC from ESP32's factory efuse MAC (globally unique).
        // ESP_MAC_ETH is a dedicated, non-overlapping block for Ethernet.
        esp_read_mac(_mac, ESP_MAC_ETH);
        char macBuf[18];
        snprintf(macBuf, sizeof(macBuf), "%02X:%02X:%02X:%02X:%02X:%02X",
                 _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5]);
        log("Ethernet MAC: " + String(macBuf));
    }

    // Bootloader rollback is enabled in the framework. A firmware applied via OTA
    // boots in PENDING_VERIFY state: it must be marked valid, otherwise the next
    // reset reverts to the previous firmware. We use "network is up" as the health
    // bar because it guarantees the device stays remotely recoverable.
    void verifyOtaHealth(bool networkHealthy) {
        const esp_partition_t* running = esp_ota_get_running_partition();
        esp_ota_img_states_t state;
        if (esp_ota_get_state_partition(running, &state) != ESP_OK) {
            return;  // can't query state; nothing to do
        }
        if (state != ESP_OTA_IMG_PENDING_VERIFY) {
            return;  // not a trial boot (normal boot or USB-flashed firmware)
        }

        if (networkHealthy) {
            esp_ota_mark_app_valid_cancel_rollback();
            log("OTA: new firmware verified healthy, rollback cancelled");
        } else {
            log("OTA: trial firmware failed health check, rolling back...");
            delay(100);
            esp_ota_mark_app_invalid_rollback_and_reboot();  // does not return
        }
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
        log("Initializing Ethernet...");

        for (uint8_t retry = 0; retry < MAX_NETWORK_RETRIES; retry++) {
            if (retry > 0) {
                log("Retry " + String(retry) + "/" + String(MAX_NETWORK_RETRIES));
                delay(RETRY_DELAY_MS);
            }

            if (_network.begin(_mac)) {
                #if WEBSERVER_ENABLE
                _webServer.setConfigManager(&_configMgr);
                _webServer.setNetwork(&_network);
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
    // Server Resolution (UDP discovery + cached/manual)
    // ========================================

    // Decide which server to talk to and transition to RUNNING or NO_SERVER.
    // Priority:
    //   1) Cached host in NVS (source = DISCOVERED or MANUAL) — verify reachable
    //   2) Fresh UDP discovery — verify HMAC + reachable + sync, then TOFU-pin
    //   3) Fall through to NO_SERVER (web UI lets operator input manual config)
    void resolveServer() {
        if (_configMgr.isResolved()) {
            log(String("Trying cached server (") + ConfigManager::sourceLabel(_configMgr.getSource()) +
                "): " + _configMgr.getHost() + ":" + String(_configMgr.getPort()));
            if (_network.checkReachable(_configMgr.getHost(),
                                        _configMgr.getPort(),
                                        _configMgr.getPingPath())) {
                log("Cached server reachable — using it");
                _state = AppState::RUNNING;
                return;
            }
            log("Cached server NOT reachable — falling through to discovery");
        }

        if (attemptDiscovery()) {
            _state = AppState::RUNNING;
            return;
        }

        log("No server resolved — entering NO_SERVER state (web UI accessible for manual config)");
        _state = AppState::NO_SERVER;
        _discoveryBackoffIdx = 0;
        _nextDiscoveryAttemptMs = millis() + DISCOVERY_BACKOFF_MS[0];
    }

    // Run one discovery attempt synchronously by spinning tick() with WDT feeds.
    // Total bounded by DISCOVERY_PROBE_BURST * DISCOVERY_PROBE_TIMEOUT_MS (~4.5s).
    // On success: verify reachability + sync device, then persist + TOFU-pin.
    bool attemptDiscovery() {
        log("Starting UDP discovery (port " + String(DISCOVERY_PORT) + ")...");

        if (!_discovery.start(_deviceId, _mac, FIRMWARE_VERSION)) {
            log("Discovery start() failed");
            return false;
        }

        // Spin until done. Each iteration is fast (parsePacket + maybe send).
        // Cap at probes * timeout + slack.
        const uint32_t maxDurationMs =
            (uint32_t)DISCOVERY_PROBE_BURST * (uint32_t)DISCOVERY_PROBE_TIMEOUT_MS + 500;
        const uint32_t startMs = millis();
        while (!_discovery.isDone()) {
            esp_task_wdt_reset();
            _discovery.tick();
            if (millis() - startMs > maxDurationMs) {
                log("Discovery wall-clock timeout — aborting");
                _discovery.reset();
                return false;
            }
            delay(5);
        }

        const bool ok = _discovery.isSuccess();
        if (!ok) {
            log("Discovery: no valid reply");
            _discovery.reset();
            return false;
        }

        const DiscoveryResult& r = _discovery.result();

        // TOFU check: if we have a pinned fingerprint, reply must match.
        char incomingFp[CONFIG_FINGERPRINT_LEN];
        DiscoveryModule::computeFingerprint(r.host, r.port, r.path, incomingFp, sizeof(incomingFp));
        if (_configMgr.hasTrustLock()) {
            if (strcmp(incomingFp, _configMgr.getTrustFingerprint()) != 0) {
                log("Discovery: fingerprint mismatch vs TOFU lock — REJECTING");
                log("  pinned:   " + String(_configMgr.getTrustFingerprint()));
                log("  incoming: " + String(incomingFp));
                log("  Use POST /api/server/reset-trust if server legitimately moved.");
                _discovery.reset();
                return false;
            }
            log("Discovery: fingerprint matches TOFU lock");
        }

        // Sanity check: server actually answers HTTP at PING path.
        if (!_network.checkReachable(r.host, r.port, _configMgr.getPingPath())) {
            log("Discovery: HMAC-valid but server NOT reachable on HTTP — discarding");
            _discovery.reset();
            return false;
        }

        // Persist + push device sync (carries ota_token — only after all checks).
        _configMgr.setHost(r.host);
        _configMgr.setPort(r.port);
        _configMgr.setPath(r.path);
        _configMgr.setSource(ServerSource::DISCOVERED);
        if (!_configMgr.hasTrustLock()) {
            _configMgr.setTrustFingerprint(incomingFp);
            log("Discovery: TOFU lock established (" + String(incomingFp).substring(0, 16) + "...)");
        }
        _configMgr.setEnabled(true);  // auto-enable webhook on first successful resolve
        _configMgr.save();

        const HttpResponse syncRes = _network.syncDeviceInfo(
            r.host, _configMgr.getSyncPath(), r.port,
            _deviceId, _configMgr.getOtaToken());
        if (!syncRes.success) {
            log("Discovery: /api/device/sync failed (HTTP " + String(syncRes.statusCode) +
                ") — continuing anyway, GPS send will proceed");
        }

        log("Discovery: resolved " + String(r.host) + ":" + String(r.port) + r.path);
        _discovery.reset();
        return true;
    }

    // Called from loop() when in NO_SERVER state.
    void tickNoServerRetry() {
        if (millis() < _nextDiscoveryAttemptMs) return;

        log("NO_SERVER: retrying discovery (backoff idx " + String(_discoveryBackoffIdx) + ")");
        if (attemptDiscovery()) {
            log("NO_SERVER → RUNNING");
            _state = AppState::RUNNING;
            _lastSendTime = 0;
            _consecutiveSendFails = 0;
            setLED(false);
            return;
        }

        // Advance backoff (clamp to last entry)
        if (_discoveryBackoffIdx + 1 < DISCOVERY_BACKOFF_COUNT) {
            _discoveryBackoffIdx++;
        }
        _nextDiscoveryAttemptMs = millis() + DISCOVERY_BACKOFF_MS[_discoveryBackoffIdx];
        log("NO_SERVER: next retry in " + String(DISCOVERY_BACKOFF_MS[_discoveryBackoffIdx] / 1000) + "s");
    }

    // Map AppState to a stable string label for /api/server/status.
    const char* stateLabel() const {
        switch (_state) {
            case AppState::INIT:               return "init";
            case AppState::NETWORK_CONNECTING: return "network_connecting";
            case AppState::RUNNING:            return "running";
            case AppState::NO_SERVER:          return "no_server";
            case AppState::ERROR_NETWORK:      return "error_network";
            case AppState::ERROR_FATAL:        return "error_fatal";
        }
        return "unknown";
    }

    // Fast double-blink LED pattern: 1500ms cycle, on for two short pulses.
    // Distinguishes NO_SERVER visually from solid-on "sending" and slow blinks.
    void updateNoServerLED() {
        #if LED_ENABLE
        const uint32_t phase = millis() % 1500;
        const bool on = (phase < 120) || (phase >= 240 && phase < 360);
        digitalWrite(LED_BUILTIN_PIN, on ? HIGH : LOW);
        #endif
    }

    // ========================================
    // Main Processing
    // ========================================

    void startSend() {
        log("\n--- Processing Cycle ---");

        // Snapshot the latest GPS fix (parser is fed continuously in loop())
        GPSData gpsData;
        const bool hasValidFix = _gps.getFix(gpsData);

        // Log GPS status
        logGPSStatus(gpsData, hasValidFix);

        // Check if webhook is enabled
        if (!_configMgr.isEnabled()) {
            log("Webhook disabled, skipping send");
            _currentInterval = hasValidFix ? SEND_INTERVAL_NORMAL : SEND_INTERVAL_NO_FIX;
            return;
        }

        // Kick off a non-blocking send; result handled later via takeResult()
        _sendFixValid = hasValidFix;
        setLED(true);
        _network.beginSend(
            _configMgr.getHost(),
            _configMgr.getPath(),
            _configMgr.getPort(),
            _deviceId, gpsData
        );
    }

    void handleSendResult(const HttpResponse& response) {
        if (response.success) {
            log("Data sent successfully (HTTP " + String(response.statusCode) + ")");
            _currentInterval = _sendFixValid ? SEND_INTERVAL_NORMAL : SEND_INTERVAL_NO_FIX;
            _networkRetryCount = 0;
            _consecutiveSendFails = 0;
        } else {
            log("Failed to send data (HTTP " + String(response.statusCode) + ")");
            _networkRetryCount++;
            _consecutiveSendFails++;

            // Trigger re-discovery after threshold consecutive failures —
            // server might have moved IP. Stays in RUNNING; cached host stays
            // trusted until discovery returns a different (verified) fingerprint.
            if (_consecutiveSendFails >= DISCOVERY_FAIL_THRESHOLD) {
                log("Send failed " + String(_consecutiveSendFails) +
                    "x in a row — attempting re-discovery");
                if (attemptDiscovery()) {
                    log("Re-discovery succeeded");
                    _consecutiveSendFails = 0;
                } else {
                    log("Re-discovery failed — staying on cached server, will retry on next failure burst");
                    _consecutiveSendFails = 0;  // reset to avoid hot-loop on every send
                }
            }
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
            log("Reconnected successfully");
            _networkRetryCount = 0;
            // Re-resolve: cached host may still be reachable, otherwise NO_SERVER.
            // This also re-runs discovery if cache is stale (e.g., DHCP lease change
            // on the server side during the outage).
            resolveServer();
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
