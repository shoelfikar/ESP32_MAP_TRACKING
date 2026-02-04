/**
 * Configuration file for ESP32 + GPS + WiFi Map Tracking
 *
 * Ubah nilai-nilai di bawah sesuai kebutuhan Anda
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// Device Configuration
// ============================================
#define DEVICE_ID       "ESP32_GPS_001"

// ============================================
// WiFi Configuration
// ============================================
#define WIFI_SSID       "SHOEL-HOME-NETWORK"       // Ganti dengan nama WiFi Anda
#define WIFI_PASSWORD   "@kadalgurun25"    // Ganti dengan password WiFi

// ============================================
// Webhook Configuration
// ============================================
// Ganti dengan URL webhook Anda (contoh: webhook.site, n8n, Make, dll)
#define WEBHOOK_URL     "https://webhook.site/07047a2d-539d-46e4-98f7-0669190882a0"

// Contoh webhook URLs:
// - webhook.site:  https://webhook.site/xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
// - n8n:           https://your-n8n.com/webhook/your-path
// - Make:          https://hook.make.com/xxxxxxxxx
// - Pipedream:     https://xxxxxxxx.m.pipedream.net

// ============================================
// Timing Configuration
// ============================================
// Interval pengiriman data (dalam milliseconds)
#define SEND_INTERVAL   30000    // 5 detik
#define GPS_FAILED_INTERVAL   300000

// ============================================
// GPS Configuration
// ============================================
// Pin untuk GPS module (NEO-M8N)
#define GPS_RX_PIN      16      // RX2 - connect to GPS TX
#define GPS_TX_PIN      17      // TX2 - connect to GPS RX
#define GPS_BAUD        9600

// --- KONFIGURASI PIN W5500 ---
#define W5500_CS    5
#define W5500_RST   4 

#endif // CONFIG_H
