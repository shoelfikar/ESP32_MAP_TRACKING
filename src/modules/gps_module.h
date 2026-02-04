#ifndef GPS_MODULE_H
#define GPS_MODULE_H

#include <Arduino.h>
#include <TinyGPSPlus.h>

/**
 * GPS Data Structure - Stack allocated, no heap usage
 */
struct GPSData {
    double latitude;
    double longitude;
    double speed;
    double altitude;
    double course;
    uint8_t satellites;
    bool valid;
    char datetime[24];  // Fixed buffer instead of String

    void clear() {
        latitude = 0.0;
        longitude = 0.0;
        speed = 0.0;
        altitude = 0.0;
        course = 0.0;
        satellites = 0;
        valid = false;
        datetime[0] = '\0';
    }
};

/**
 * GPS Module Class - Encapsulates all GPS functionality
 */
class GPSModule {
public:
    GPSModule(uint8_t rxPin, uint8_t txPin, uint32_t baudRate)
        : _rxPin(rxPin), _txPin(txPin), _baudRate(baudRate), _serial(2) {}

    bool begin() {
        _serial.begin(_baudRate, SERIAL_8N1, _rxPin, _txPin);
        delay(100);
        return true;
    }

    /**
     * Read GPS data with timeout
     * @param data Reference to GPSData struct to fill
     * @param timeoutMs Read timeout in milliseconds
     * @return true if valid fix obtained
     */
    bool read(GPSData& data, uint32_t timeoutMs = 1000) {
        data.clear();

        const uint32_t startTime = millis();
        while (millis() - startTime < timeoutMs) {
            while (_serial.available() > 0) {
                if (_gps.encode(_serial.read())) {
                    parseData(data);
                }
            }
            yield(); // Allow ESP32 background tasks
        }

        return data.valid;
    }

    /**
     * Get number of characters processed
     */
    uint32_t getCharsProcessed() const {
        return _gps.charsProcessed();
    }

    /**
     * Check if GPS is receiving data
     */
    bool isReceiving() const {
        return _gps.charsProcessed() > 0;
    }

private:
    const uint8_t _rxPin;
    const uint8_t _txPin;
    const uint32_t _baudRate;
    HardwareSerial _serial;
    TinyGPSPlus _gps;

    void parseData(GPSData& data) {
        // Location
        if (_gps.location.isValid()) {
            data.valid = true;
            data.latitude = _gps.location.lat();
            data.longitude = _gps.location.lng();
        }

        // Speed (km/h)
        data.speed = _gps.speed.isValid() ? _gps.speed.kmph() : 0.0;

        // Altitude (meters)
        data.altitude = _gps.altitude.isValid() ? _gps.altitude.meters() : 0.0;

        // Course (degrees)
        data.course = _gps.course.isValid() ? _gps.course.deg() : 0.0;

        // Satellites
        data.satellites = _gps.satellites.isValid() ? _gps.satellites.value() : 0;

        // DateTime - use fixed buffer
        if (_gps.date.isValid() && _gps.time.isValid()) {
            snprintf(data.datetime, sizeof(data.datetime),
                     "%04d-%02d-%02dT%02d:%02d:%02dZ",
                     _gps.date.year(), _gps.date.month(), _gps.date.day(),
                     _gps.time.hour(), _gps.time.minute(), _gps.time.second());
        } else {
            strncpy(data.datetime, "N/A", sizeof(data.datetime));
        }
    }
};

#endif // GPS_MODULE_H
