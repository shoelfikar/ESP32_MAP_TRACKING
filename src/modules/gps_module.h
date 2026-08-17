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
    double speed;  // knots (SOG)
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
        : _rxPin(rxPin), _txPin(txPin), _baudRate(baudRate), _serial(2) {
        _data.clear();
    }

    bool begin() {
        // Larger RX buffer so NMEA isn't dropped if update() is briefly delayed
        // (e.g. during a blocking webhook send).
        _serial.setRxBufferSize(1024);
        _serial.begin(_baudRate, SERIAL_8N1, _rxPin, _txPin);
        delay(100);
        return true;
    }

    /**
     * Feed all currently-available serial bytes into the parser. Non-blocking —
     * processes only what's buffered, so it must be called every loop iteration.
     */
    void update() {
        while (_serial.available() > 0) {
            if (_gps.encode(_serial.read())) {
                refresh();  // a complete sentence parsed
            }
        }
    }

    /**
     * Copy the latest GPS snapshot. Returns whether the fix is currently valid
     * (a recent, valid location — see FIX_MAX_AGE_MS).
     */
    bool getFix(GPSData& out) {
        refreshValidity();
        out = _data;
        return _data.valid;
    }

    bool hasValidFix() {
        refreshValidity();
        return _data.valid;
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
    static constexpr uint32_t FIX_MAX_AGE_MS = 5000;  // fix considered stale after this

    const uint8_t _rxPin;
    const uint8_t _txPin;
    const uint32_t _baudRate;
    HardwareSerial _serial;
    TinyGPSPlus _gps;
    GPSData _data;  // latest parsed snapshot

    // Refresh snapshot fields after a complete sentence parses.
    void refresh() {
        if (_gps.location.isValid()) {
            _data.latitude = _gps.location.lat();
            _data.longitude = _gps.location.lng();
        }

        _data.speed = _gps.speed.isValid() ? _gps.speed.knots() : 0.0;
        _data.altitude = _gps.altitude.isValid() ? _gps.altitude.meters() : 0.0;
        _data.course = _gps.course.isValid() ? _gps.course.deg() : 0.0;
        _data.satellites = _gps.satellites.isValid() ? _gps.satellites.value() : 0;

        if (_gps.date.isValid() && _gps.time.isValid()) {
            snprintf(_data.datetime, sizeof(_data.datetime),
                     "%04d-%02d-%02dT%02d:%02d:%02dZ",
                     _gps.date.year(), _gps.date.month(), _gps.date.day(),
                     _gps.time.hour(), _gps.time.minute(), _gps.time.second());
        } else {
            strncpy(_data.datetime, "N/A", sizeof(_data.datetime));
        }

        refreshValidity();
    }

    // A fix is valid only if the location is valid AND recent (not stale).
    void refreshValidity() {
        _data.valid = _gps.location.isValid() && _gps.location.age() < FIX_MAX_AGE_MS;
    }
};

#endif // GPS_MODULE_H
