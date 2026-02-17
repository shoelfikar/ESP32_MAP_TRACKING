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
#define CONFIG_SSID_MAX_LEN     33
#define CONFIG_PASS_MAX_LEN     65

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
 * Network Configuration Structure
 */
struct NetworkConfig {
    bool useWifi;                           // true = WiFi, false = Ethernet
    char wifiSsid[CONFIG_SSID_MAX_LEN];
    char wifiPassword[CONFIG_PASS_MAX_LEN];
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

        // Load network config
        _networkConfig.useWifi = _prefs.getBool("net_wifi", WIFI_ENABLE);

        strncpy(_networkConfig.wifiSsid, _prefs.getString("net_ssid", WIFI_SSID).c_str(), CONFIG_SSID_MAX_LEN - 1);
        _networkConfig.wifiSsid[CONFIG_SSID_MAX_LEN - 1] = '\0';

        strncpy(_networkConfig.wifiPassword, _prefs.getString("net_pass", WIFI_PASSWORD).c_str(), CONFIG_PASS_MAX_LEN - 1);
        _networkConfig.wifiPassword[CONFIG_PASS_MAX_LEN - 1] = '\0';

        printConfig();
    }

    /**
     * Save webhook configuration to NVS
     */
    bool saveWebhook() {
        bool success = true;

        success &= _prefs.putString("wh_host", _webhookConfig.host);
        success &= _prefs.putUShort("wh_port", _webhookConfig.port);
        success &= _prefs.putString("wh_path", _webhookConfig.path);
        success &= _prefs.putBool("wh_enabled", _webhookConfig.enabled);

        if (success) {
            Serial.println("[Config] Webhook config saved to NVS");
        } else {
            Serial.println("[Config] Error saving webhook config!");
        }

        return success;
    }

    /**
     * Save network configuration to NVS
     */
    bool saveNetwork() {
        bool success = true;

        success &= _prefs.putBool("net_wifi", _networkConfig.useWifi);
        success &= _prefs.putString("net_ssid", _networkConfig.wifiSsid);
        success &= _prefs.putString("net_pass", _networkConfig.wifiPassword);

        if (success) {
            Serial.println("[Config] Network config saved to NVS");
        } else {
            Serial.println("[Config] Error saving network config!");
        }

        return success;
    }

    /**
     * Save all configuration to NVS (legacy compatibility)
     */
    bool save() {
        return saveWebhook() && saveNetwork();
    }

    /**
     * Reset webhook configuration to defaults (from config.h)
     */
    void resetWebhookToDefaults() {
        strncpy(_webhookConfig.host, SERVER_HOST, CONFIG_HOST_MAX_LEN - 1);
        _webhookConfig.host[CONFIG_HOST_MAX_LEN - 1] = '\0';

        _webhookConfig.port = SERVER_PORT;

        strncpy(_webhookConfig.path, SERVER_PATH, CONFIG_PATH_MAX_LEN - 1);
        _webhookConfig.path[CONFIG_PATH_MAX_LEN - 1] = '\0';

        _webhookConfig.enabled = true;

        saveWebhook();
        Serial.println("[Config] Webhook reset to defaults");
    }

    /**
     * Reset network configuration to defaults (from config.h)
     */
    void resetNetworkToDefaults() {
        _networkConfig.useWifi = WIFI_ENABLE;

        strncpy(_networkConfig.wifiSsid, WIFI_SSID, CONFIG_SSID_MAX_LEN - 1);
        _networkConfig.wifiSsid[CONFIG_SSID_MAX_LEN - 1] = '\0';

        strncpy(_networkConfig.wifiPassword, WIFI_PASSWORD, CONFIG_PASS_MAX_LEN - 1);
        _networkConfig.wifiPassword[CONFIG_PASS_MAX_LEN - 1] = '\0';

        saveNetwork();
        Serial.println("[Config] Network reset to defaults");
    }

    /**
     * Reset all configuration to defaults (from config.h)
     */
    void resetToDefaults() {
        resetWebhookToDefaults();
        resetNetworkToDefaults();
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
    // Webhook Setters
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
    // Network Setters
    // ========================================
    void setUseWifi(bool useWifi) {
        _networkConfig.useWifi = useWifi;
    }

    void setWifiSsid(const char* ssid) {
        strncpy(_networkConfig.wifiSsid, ssid, CONFIG_SSID_MAX_LEN - 1);
        _networkConfig.wifiSsid[CONFIG_SSID_MAX_LEN - 1] = '\0';
    }

    void setWifiPassword(const char* password) {
        strncpy(_networkConfig.wifiPassword, password, CONFIG_PASS_MAX_LEN - 1);
        _networkConfig.wifiPassword[CONFIG_PASS_MAX_LEN - 1] = '\0';
    }

    // ========================================
    // Webhook Getters
    // ========================================
    const char* getHost() const { return _webhookConfig.host; }
    uint16_t getPort() const { return _webhookConfig.port; }
    const char* getPath() const { return _webhookConfig.path; }
    bool isEnabled() const { return _webhookConfig.enabled; }
    const WebhookConfig& getWebhookConfig() const { return _webhookConfig; }

    // Legacy alias
    const WebhookConfig& getConfig() const { return _webhookConfig; }

    // ========================================
    // Network Getters
    // ========================================
    bool useWifi() const { return _networkConfig.useWifi; }
    const char* getWifiSsid() const { return _networkConfig.wifiSsid; }
    const char* getWifiPassword() const { return _networkConfig.wifiPassword; }
    const NetworkConfig& getNetworkConfig() const { return _networkConfig; }

    /**
     * Print current configuration to Serial
     */
    void printConfig() const {
        Serial.println("[Config] Webhook configuration:");
        Serial.printf("  Host: %s\n", _webhookConfig.host);
        Serial.printf("  Port: %d\n", _webhookConfig.port);
        Serial.printf("  Path: %s\n", _webhookConfig.path);
        Serial.printf("  Enabled: %s\n", _webhookConfig.enabled ? "Yes" : "No");

        Serial.println("[Config] Network configuration:");
        Serial.printf("  Mode: %s\n", _networkConfig.useWifi ? "WiFi" : "Ethernet");
        Serial.printf("  SSID: %s\n", _networkConfig.wifiSsid);
        Serial.println("  Password: ********");
    }

private:
    Preferences _prefs;
    WebhookConfig _webhookConfig;
    NetworkConfig _networkConfig;
};

#endif // CONFIG_MANAGER_H
