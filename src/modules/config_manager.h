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

        save();
        Serial.println("[Config] Reset to defaults");
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
    const WebhookConfig& getConfig() const { return _webhookConfig; }

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
    }

private:
    Preferences _prefs;
    WebhookConfig _webhookConfig;

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
