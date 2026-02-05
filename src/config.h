/**
 * @file config.h
 * @brief Configuration for ESP32 GPS Tracker with W5500 Ethernet
 * @version 2.0.0
 *
 * Production Configuration File
 * Edit values below according to your setup
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// Firmware Version
// ============================================
#define FIRMWARE_VERSION    "2.0.0"
#define FIRMWARE_BUILD      __DATE__ " " __TIME__

// ============================================
// Device Configuration
// ============================================
#define DEVICE_ID           "ESP32_GPS_001"

// ============================================
// Server Configuration (HTTP)
// ============================================
#define SERVER_HOST         "pelni-webhook-send.shoel-dev.workers.dev"
#define SERVER_PATH         "/"
#define SERVER_PORT         80

// ============================================
// Timing Configuration (milliseconds)
// ============================================
#define SEND_INTERVAL_NORMAL    30000       // 30 seconds when GPS valid
#define SEND_INTERVAL_NO_FIX    300000      // 5 minutes when no GPS fix
#define GPS_READ_TIMEOUT        2000        // GPS read timeout
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
// MAC Address (must be unique on your network)
#define MAC_ADDR            {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}

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

// Default Location (used when GPS has no fix)
#define DEFAULT_LAT         -6.37396
#define DEFAULT_LNG         106.84527

#endif // CONFIG_H
