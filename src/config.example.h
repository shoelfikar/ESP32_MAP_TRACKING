/**
 * @file config.example.h
 * @brief Configuration for ESP32 GPS Tracker with W5500 Ethernet
 * @version 2.0.0
 *
 * Copy this file to config.h and edit values according to your setup:
 *   cp src/config.example.h src/config.h
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// Firmware Version
// ============================================
#define FIRMWARE_VERSION    "1.0.0"
#define FIRMWARE_BUILD      __DATE__ " " __TIME__

// ============================================
// Device Configuration
// ============================================
// Device ID = PREFIX + ESP32 Chip ID (auto-generated)
// Example: "GPS_" + "A1B2C3" = "GPS_A1B2C3"
#define DEVICE_ID_PREFIX    "GPS_"

// ============================================
// Server Configuration (HTTP)
// ============================================
// Pure UDP discovery (Opsi A): host/port/path WAJIB kosong supaya device
// auto-discover via UDP broadcast saat boot. Tidak ada compile-time fallback.
// Kalau discovery gagal, operator bisa input manual via web UI device.
#define SERVER_HOST         ""
#define SERVER_PATH         ""
#define SERVER_PORT         0
#define SERVER_PING_PATH    "/health"               // Path used for reachability check
#define SERVER_SYNC_PATH    "/api/gps/sync"      // Path used for device identity sync

// ============================================
// UDP Discovery Configuration
// ============================================
#define DISCOVERY_PORT              4210            // UDP port untuk discovery probe & reply
#define DISCOVERY_PROBE_TIMEOUT_MS  1500            // Tunggu reply per probe (ms)
#define DISCOVERY_PROBE_BURST       3               // Berapa probe per attempt
#define DISCOVERY_FAIL_THRESHOLD    3               // Send gagal berapa kali sebelum re-discovery (state RESOLVED)
// Shared secret untuk HMAC-SHA256 verifikasi reply.
// GANTI dengan random 32-byte hex string untuk deployment Anda.
// Sama dengan SHARED_SECRET di server-discovery responder.
// File config.h gitignored — aman dari leak ke commit publik.
#define DISCOVERY_SHARED_SECRET     "CHANGE_ME_TO_RANDOM_32_BYTE_HEX_STRING"

// ============================================
// Timing Configuration (milliseconds)
// ============================================
#define SEND_INTERVAL_NORMAL    30000       // 30 seconds when GPS valid
#define SEND_INTERVAL_NO_FIX    300000      // 5 minutes when no GPS fix
#define HTTP_TIMEOUT            10000       // HTTP request timeout
#define WATCHDOG_TIMEOUT        60          // Watchdog timeout in seconds

// ============================================
// GPS Module (NEO-M8N) Configuration
// ============================================
#define GPS_RX_PIN          16      // ESP32 RX <- GPS TX
#define GPS_TX_PIN          17      // ESP32 TX -> GPS RX
#define GPS_BAUD_RATE       9600

// ============================================
// W5500 Ethernet Module Configuration
// ============================================
#define W5500_CS_PIN        5       // Chip Select
#define W5500_RST_PIN       4       // Reset
// SPI Pins (using ESP32 default VSPI)
// MISO = GPIO 19
// MOSI = GPIO 23
// SCK  = GPIO 18

// ============================================
// Status LED Configuration (Optional)
// ============================================
#define LED_BUILTIN_PIN     2       // Built-in LED
#define LED_ENABLE          true    // Enable status LED

// ============================================
// Debug Configuration
// ============================================
#define DEBUG_SERIAL        true    // Enable serial debug output
#define DEBUG_BAUD_RATE     115200

// ============================================
// Network Configuration
// ============================================
// MAC address is derived automatically from the ESP32 efuse (ESP_MAC_ETH)

// ============================================
// Memory Optimization
// ============================================
#define JSON_BUFFER_SIZE    384     // JSON document size
#define HTTP_BUFFER_SIZE    512     // HTTP response buffer

// ============================================
// Retry Configuration
// ============================================
#define MAX_NETWORK_RETRIES 3       // Max network connection retries
#define RETRY_DELAY_MS      5000    // Delay between retries

// ============================================
// Web Server Configuration
// ============================================
#define WEBSERVER_ENABLE    true    // Enable built-in web server
#define WEBSERVER_PORT      80      // Web server port

// ============================================
// OTA Firmware Update
// ============================================
// The "X-Update-Token" secret is auto-generated on first boot and stored in NVS.

// Default Location (used when GPS has no fix)
#define DEFAULT_LAT         0.0
#define DEFAULT_LNG         0.0

#endif // CONFIG_H
