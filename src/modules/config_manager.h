#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

/**
 * @file config_manager.h
 * @brief Persistent configuration manager using ESP32 Preferences (NVS)
 *
 * Stores webhook configuration in non-volatile storage
 * Allows runtime configuration via web interface
 */

#include <Arduino.h>
#include <Preferences.h>
#include <esp_random.h>
#include "../config.h"

// Configuration limits
#define CONFIG_HOST_MAX_LEN     64
#define CONFIG_PATH_MAX_LEN     128
#define CONFIG_TOKEN_MAX_LEN    33      // 32 hex chars + null
#define CONFIG_FINGERPRINT_LEN  65      // 64 hex chars + null (SHA-256)

/**
 * Source of currently-active server config — drives the dashboard badge
 * and the resolve priority order.
 */
enum class ServerSource : uint8_t {
    NONE = 0,          // host kosong, belum pernah resolve
    DISCOVERED = 1,    // dari UDP discovery (cached / fresh)
    MANUAL = 2         // di-set user via /api/config
};

/**
 * Webhook Configuration Structure
 */
struct WebhookConfig {
    char host[CONFIG_HOST_MAX_LEN];
    uint16_t port;
    char path[CONFIG_PATH_MAX_LEN];
    char pingPath[CONFIG_PATH_MAX_LEN];
    char syncPath[CONFIG_PATH_MAX_LEN];
    char otaToken[CONFIG_TOKEN_MAX_LEN];
    bool enabled;
    ServerSource source;
    char trustFingerprint[CONFIG_FINGERPRINT_LEN];  // TOFU lock; empty = unlocked
};

/**
 * Timing Configuration Structure
 *
 * Runtime-tunable counterparts to SEND_INTERVAL_NORMAL, SEND_INTERVAL_NO_FIX,
 * and HTTP_TIMEOUT in config.h. The #defines remain as boot defaults.
 */
struct TimingConfig {
    uint32_t sendIntervalNormal;   // ms between sends when GPS fix is valid
    uint32_t sendIntervalNoFix;    // ms between sends when no GPS fix
    uint32_t httpTimeout;          // ms to wait for HTTP response
};

/**
 * Configuration Manager Class
 */
class ConfigManager {
public:
    /**
     * Initialize and load configuration from NVS
     */
    void begin() {
        _prefs.begin("gps-tracker", false);  // namespace, read-write mode
        load();
        Serial.println("[Config] Configuration loaded from NVS");
    }

    /**
     * Load configuration from NVS, fallback to defaults
     */
    void load() {
        // Load webhook config with defaults from config.h
        strncpy(_webhookConfig.host, _prefs.getString("wh_host", SERVER_HOST).c_str(), CONFIG_HOST_MAX_LEN - 1);
        _webhookConfig.host[CONFIG_HOST_MAX_LEN - 1] = '\0';

        _webhookConfig.port = _prefs.getUShort("wh_port", SERVER_PORT);

        strncpy(_webhookConfig.path, _prefs.getString("wh_path", SERVER_PATH).c_str(), CONFIG_PATH_MAX_LEN - 1);
        _webhookConfig.path[CONFIG_PATH_MAX_LEN - 1] = '\0';

        strncpy(_webhookConfig.pingPath, _prefs.getString("wh_ping_path", SERVER_PING_PATH).c_str(), CONFIG_PATH_MAX_LEN - 1);
        _webhookConfig.pingPath[CONFIG_PATH_MAX_LEN - 1] = '\0';

        strncpy(_webhookConfig.syncPath, _prefs.getString("wh_sync_path", SERVER_SYNC_PATH).c_str(), CONFIG_PATH_MAX_LEN - 1);
        _webhookConfig.syncPath[CONFIG_PATH_MAX_LEN - 1] = '\0';

        _webhookConfig.enabled = _prefs.getBool("wh_enabled", false);

        _webhookConfig.source = (ServerSource)_prefs.getUChar("wh_source", (uint8_t)ServerSource::NONE);

        String storedFp = _prefs.getString("wh_trust_fp", "");
        strncpy(_webhookConfig.trustFingerprint, storedFp.c_str(), CONFIG_FINGERPRINT_LEN - 1);
        _webhookConfig.trustFingerprint[CONFIG_FINGERPRINT_LEN - 1] = '\0';

        // OTA token: auto-generate once on first boot, then persist.
        String storedToken = _prefs.getString("wh_ota_token", "");
        if (storedToken.length() == 0) {
            generateOtaToken(_webhookConfig.otaToken, CONFIG_TOKEN_MAX_LEN);
            _prefs.putString("wh_ota_token", _webhookConfig.otaToken);
            Serial.println("[Config] Generated new OTA token");
        } else {
            strncpy(_webhookConfig.otaToken, storedToken.c_str(), CONFIG_TOKEN_MAX_LEN - 1);
            _webhookConfig.otaToken[CONFIG_TOKEN_MAX_LEN - 1] = '\0';
        }

        // Timing config — defaults come from compile-time #defines in config.h
        _timingConfig.sendIntervalNormal = _prefs.getUInt("tm_send_n",  SEND_INTERVAL_NORMAL);
        _timingConfig.sendIntervalNoFix  = _prefs.getUInt("tm_send_nf", SEND_INTERVAL_NO_FIX);
        _timingConfig.httpTimeout        = _prefs.getUInt("tm_http_to", HTTP_TIMEOUT);

        printConfig();
    }

    /**
     * Save configuration to NVS
     */
    bool save() {
        // putString/putUShort/putBool return the number of bytes written, not a
        // bool. Compare with > 0 so we build a real success flag (bitwise &= on
        // the raw byte counts would clear the flag, e.g. 1 & 2 == 0).
        bool success = true;

        success &= (_prefs.putString("wh_host", _webhookConfig.host) > 0);
        success &= (_prefs.putUShort("wh_port", _webhookConfig.port) > 0);
        success &= (_prefs.putString("wh_path", _webhookConfig.path) > 0);
        success &= (_prefs.putString("wh_ping_path", _webhookConfig.pingPath) > 0);
        success &= (_prefs.putString("wh_sync_path", _webhookConfig.syncPath) > 0);
        success &= (_prefs.putString("wh_ota_token", _webhookConfig.otaToken) > 0);
        success &= (_prefs.putBool("wh_enabled", _webhookConfig.enabled) > 0);
        success &= (_prefs.putUChar("wh_source", (uint8_t)_webhookConfig.source) > 0);
        // Trust fingerprint may legitimately be empty (TOFU unlocked); allow 0 bytes.
        _prefs.putString("wh_trust_fp", _webhookConfig.trustFingerprint);

        success &= (_prefs.putUInt("tm_send_n",  _timingConfig.sendIntervalNormal) > 0);
        success &= (_prefs.putUInt("tm_send_nf", _timingConfig.sendIntervalNoFix)  > 0);
        success &= (_prefs.putUInt("tm_http_to", _timingConfig.httpTimeout)        > 0);

        if (success) {
            Serial.println("[Config] Configuration saved to NVS");
        } else {
            Serial.println("[Config] Error saving configuration!");
        }

        return success;
    }

    /**
     * Reset configuration to defaults (from config.h)
     */
    void resetToDefaults() {
        strncpy(_webhookConfig.host, SERVER_HOST, CONFIG_HOST_MAX_LEN - 1);
        _webhookConfig.host[CONFIG_HOST_MAX_LEN - 1] = '\0';

        _webhookConfig.port = SERVER_PORT;

        strncpy(_webhookConfig.path, SERVER_PATH, CONFIG_PATH_MAX_LEN - 1);
        _webhookConfig.path[CONFIG_PATH_MAX_LEN - 1] = '\0';

        strncpy(_webhookConfig.pingPath, SERVER_PING_PATH, CONFIG_PATH_MAX_LEN - 1);
        _webhookConfig.pingPath[CONFIG_PATH_MAX_LEN - 1] = '\0';

        strncpy(_webhookConfig.syncPath, SERVER_SYNC_PATH, CONFIG_PATH_MAX_LEN - 1);
        _webhookConfig.syncPath[CONFIG_PATH_MAX_LEN - 1] = '\0';

        _webhookConfig.enabled = false;
        _webhookConfig.source = ServerSource::NONE;
        _webhookConfig.trustFingerprint[0] = '\0';

        _timingConfig.sendIntervalNormal = SEND_INTERVAL_NORMAL;
        _timingConfig.sendIntervalNoFix  = SEND_INTERVAL_NO_FIX;
        _timingConfig.httpTimeout        = HTTP_TIMEOUT;

        save();
        Serial.println("[Config] Reset to defaults");
    }

    /**
     * Clear TOFU trust lock. After this, next successful discovery will
     * accept (and re-pin) whatever fingerprint comes back. Use when server
     * legitimately moved (replaced hardware, IP rotation, etc.).
     */
    void resetTrust() {
        _webhookConfig.trustFingerprint[0] = '\0';
        _prefs.putString("wh_trust_fp", "");
        Serial.println("[Config] TOFU trust fingerprint cleared");
    }

    /**
     * Clear all stored configuration (factory reset)
     */
    void clear() {
        _prefs.clear();
        Serial.println("[Config] NVS cleared");
        load();  // Reload defaults
    }

    // ========================================
    // Setters
    // ========================================
    void setHost(const char* host) {
        strncpy(_webhookConfig.host, host, CONFIG_HOST_MAX_LEN - 1);
        _webhookConfig.host[CONFIG_HOST_MAX_LEN - 1] = '\0';
    }

    void setPort(uint16_t port) {
        _webhookConfig.port = port;
    }

    void setPath(const char* path) {
        strncpy(_webhookConfig.path, path, CONFIG_PATH_MAX_LEN - 1);
        _webhookConfig.path[CONFIG_PATH_MAX_LEN - 1] = '\0';
    }

    void setPingPath(const char* path) {
        strncpy(_webhookConfig.pingPath, path, CONFIG_PATH_MAX_LEN - 1);
        _webhookConfig.pingPath[CONFIG_PATH_MAX_LEN - 1] = '\0';
    }

    void setSyncPath(const char* path) {
        strncpy(_webhookConfig.syncPath, path, CONFIG_PATH_MAX_LEN - 1);
        _webhookConfig.syncPath[CONFIG_PATH_MAX_LEN - 1] = '\0';
    }

    void setEnabled(bool enabled) {
        _webhookConfig.enabled = enabled;
    }

    void setSource(ServerSource source) {
        _webhookConfig.source = source;
    }

    void setTrustFingerprint(const char* fp) {
        strncpy(_webhookConfig.trustFingerprint, fp, CONFIG_FINGERPRINT_LEN - 1);
        _webhookConfig.trustFingerprint[CONFIG_FINGERPRINT_LEN - 1] = '\0';
    }

    // Timing setters — clamp to safe minimums so a bad UI value can't stall the loop.
    void setSendIntervalNormal(uint32_t ms) {
        _timingConfig.sendIntervalNormal = ms < 1000 ? 1000 : ms;
    }
    void setSendIntervalNoFix(uint32_t ms) {
        _timingConfig.sendIntervalNoFix = ms < 1000 ? 1000 : ms;
    }
    void setHttpTimeout(uint32_t ms) {
        _timingConfig.httpTimeout = ms < 500 ? 500 : ms;
    }

    // ========================================
    // Getters
    // ========================================
    const char* getHost() const { return _webhookConfig.host; }
    uint16_t getPort() const { return _webhookConfig.port; }
    const char* getPath() const { return _webhookConfig.path; }
    const char* getPingPath() const { return _webhookConfig.pingPath; }
    const char* getSyncPath() const { return _webhookConfig.syncPath; }
    const char* getOtaToken() const { return _webhookConfig.otaToken; }
    bool isEnabled() const { return _webhookConfig.enabled; }
    ServerSource getSource() const { return _webhookConfig.source; }
    const char* getTrustFingerprint() const { return _webhookConfig.trustFingerprint; }
    bool hasTrustLock() const { return _webhookConfig.trustFingerprint[0] != '\0'; }

    // Resolved = punya host non-empty + port valid. Tidak peduli source-nya.
    bool isResolved() const {
        return _webhookConfig.host[0] != '\0' && _webhookConfig.port != 0;
    }

    const WebhookConfig& getConfig() const { return _webhookConfig; }

    // Timing getters
    uint32_t getSendIntervalNormal() const { return _timingConfig.sendIntervalNormal; }
    uint32_t getSendIntervalNoFix()  const { return _timingConfig.sendIntervalNoFix; }
    uint32_t getHttpTimeout()        const { return _timingConfig.httpTimeout; }
    const TimingConfig& getTimingConfig() const { return _timingConfig; }

    /**
     * Print current configuration to Serial
     */
    void printConfig() const {
        Serial.println("[Config] Webhook configuration:");
        Serial.printf("  Host: %s\n", _webhookConfig.host);
        Serial.printf("  Port: %d\n", _webhookConfig.port);
        Serial.printf("  Path: %s\n", _webhookConfig.path);
        Serial.printf("  Ping Path: %s\n", _webhookConfig.pingPath);
        Serial.printf("  Sync Path: %s\n", _webhookConfig.syncPath);
        Serial.printf("  OTA Token: %s\n", _webhookConfig.otaToken);
        Serial.printf("  Enabled: %s\n", _webhookConfig.enabled ? "Yes" : "No");
        Serial.printf("  Source: %s\n", sourceLabel(_webhookConfig.source));
        Serial.printf("  Trust FP: %s\n", _webhookConfig.trustFingerprint[0] ? _webhookConfig.trustFingerprint : "(unlocked)");
        Serial.println("[Config] Timing configuration:");
        Serial.printf("  Send Interval (fix):    %u ms\n", _timingConfig.sendIntervalNormal);
        Serial.printf("  Send Interval (no fix): %u ms\n", _timingConfig.sendIntervalNoFix);
        Serial.printf("  HTTP Timeout:           %u ms\n", _timingConfig.httpTimeout);
    }

    static const char* sourceLabel(ServerSource s) {
        switch (s) {
            case ServerSource::DISCOVERED: return "discovered";
            case ServerSource::MANUAL:     return "manual";
            default:                       return "none";
        }
    }

private:
    Preferences _prefs;
    WebhookConfig _webhookConfig;
    TimingConfig _timingConfig;

    // Generate a random hex token (16 bytes -> 32 hex chars) using the ESP32 HW RNG.
    void generateOtaToken(char* out, size_t outSize) {
        static const char hex[] = "0123456789abcdef";
        size_t pos = 0;
        for (int i = 0; i < 16 && pos + 2 < outSize; i++) {
            uint8_t b = (uint8_t)(esp_random() & 0xFF);
            out[pos++] = hex[b >> 4];
            out[pos++] = hex[b & 0x0F];
        }
        out[pos] = '\0';
    }
};

#endif // CONFIG_MANAGER_H
