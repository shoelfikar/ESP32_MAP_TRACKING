#ifndef DEVICE_IDENTITY_H
#define DEVICE_IDENTITY_H

/**
 * @file device_identity.h
 * @brief Single source of truth for the device ID derived from the factory MAC.
 *
 * PENTING: jangan pakai `ESP.getEfuseMac() & 0xFFFFFF`.
 * getEfuseMac() mengisi uint64 dari byte array MAC (mac[0]..mac[5]) pada mesin
 * little-endian, jadi 3 byte TERENDAH = mac[0..2] = OUI vendor (mis. 24:6F:28)
 * yang SAMA di semua ESP32. Byte unik per chip ada di mac[3..5].
 */

#include <Arduino.h>
#include <esp_mac.h>
#include "../config.h"

// Menghasilkan "GPS_XXXXXX" dengan XXXXXX = 3 byte unik terakhir MAC pabrik.
inline void buildDeviceId(char* out, size_t len) {
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);  // base MAC pabrik, tidak diturunkan
    snprintf(out, len, "%s%02X%02X%02X",
             DEVICE_ID_PREFIX, mac[3], mac[4], mac[5]);
}

#endif  // DEVICE_IDENTITY_H
