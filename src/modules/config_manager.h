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
#include "../config.h"

// Configuration limits
#define CONFIG_HOST_MAX_LEN     64
#define CONFIG_PATH_MAX_LEN     128

/**
 * Webhook Configuration Structure
 */
struct WebhookConfig {
    char host[CONFIG_HOST_MAX_LEN];
    uint16_t port;
    char path[CONFIG_PATH_MAX_LEN];
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

        _webhookConfig.enabled = _prefs.getBool("wh_enabled", true);

        printConfig();
    }

    /**
     * Save configuration to NVS
     */
    bool save() {
        bool success = true;

        success &= _prefs.putString("wh_host", _webhookConfig.host);
        success &= _prefs.putUShort("wh_port", _webhookConfig.port);
        success &= _prefs.putString("wh_path", _webhookConfig.path);
        success &= _prefs.putBool("wh_enabled", _webhookConfig.enabled);

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

        _webhookConfig.enabled = true;

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

    void setEnabled(bool enabled) {
        _webhookConfig.enabled = enabled;
    }

    // ========================================
    // Getters
    // ========================================
    const char* getHost() const { return _webhookConfig.host; }
    uint16_t getPort() const { return _webhookConfig.port; }
    const char* getPath() const { return _webhookConfig.path; }
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
        Serial.printf("  Enabled: %s\n", _webhookConfig.enabled ? "Yes" : "No");
    }

private:
    Preferences _prefs;
    WebhookConfig _webhookConfig;
};

#endif // CONFIG_MANAGER_H
