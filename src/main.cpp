#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoJson.h>
#include <TinyGPSPlus.h>
#include "config.h"

 

// GPS object
TinyGPSPlus gps;

// Hardware Serial for GPS
HardwareSerial gpsSerial(2);

// Tracking data structure
struct TrackingData {
    double latitude;
    double longitude;
    double speed;       // km/h
    double altitude;    // meters
    double course;      // degrees
    int satellites;
    bool valid;
    String datetime;
};

// --- KONFIGURASI WEBHOOK (PORT 80) ---
const char* server_host = "webhook.site"; 
const char* server_path = "/07047a2d-539d-46e4-98f7-0669190882a0"; // GANTI DENGAN ID WEBHOOK KAMU
const int   server_port = 80;               // Menggunakan HTTP biasa

// MAC Address (bebas, asal unik di jaringanmu)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

EthernetClient client;

// Forward declarations
void initGPS();
TrackingData readGPS();

bool sendHttpWebhook(TrackingData& data) {
  Serial.println("\n--- Menghubungkan ke Webhook (HTTP Port 80) ---");
  
  if (client.connect(server_host, server_port)) {
    Serial.println("Terhubung ke Server!");

    // 1. Siapkan Data JSON
    StaticJsonDocument<512> doc; 
    doc["status"] = "online";
    doc["mode"] = "HTTP_PORT_80";
    doc["IP"] = Ethernet.localIP();
    doc["uptime_ms"] = millis();

    // Create JSON payload
    doc["device_id"] = DEVICE_ID;
    doc["latitude"] = data.latitude;
    doc["longitude"] = data.longitude;
    doc["speed"] = data.speed;
    doc["altitude"] = data.altitude;
    doc["course"] = data.course;
    doc["satellites"] = data.satellites;
    doc["datetime"] = data.datetime;
    doc["timestamp"] = millis();
    
    String jsonString;
    serializeJson(doc, jsonString);

    // 2. Kirim HTTP POST Request
    client.print("POST "); client.print(server_path); client.println(" HTTP/1.1");
    client.print("Host: "); client.println(server_host);
    client.println("Content-Type: application/json");
    client.print("Content-Length: "); client.println(jsonString.length());
    client.println("Connection: close"); // Menutup koneksi setelah selesai
    client.println();
    client.println(jsonString);

    Serial.println("Data Berhasil Dikirim!");
    
    // 3. Baca respon singkat dari server (opsional)
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println(">>> Server No Response!");
        break;
      }
    }
    
    if (client.available()) {
      String response = client.readStringUntil('\r');
      Serial.print("Respon Server: "); Serial.println(response);
    }

    client.stop();
    
    return true;


  } else {
    Serial.println("Gagal koneksi ke server. Cek kabel LAN atau koneksi Internet.");
    client.stop();
    return false;
  }
}

void setup() {
  Serial.begin(115200);

  // --- Reset Fisik W5500 ---
  pinMode(W5500_RST, OUTPUT);
  digitalWrite(W5500_RST, LOW);
  delay(100);
  digitalWrite(W5500_RST, HIGH);
  delay(1000);

  // --- Inisialisasi Ethernet ---
  Ethernet.init(W5500_CS);
  
  Serial.println("Mendapatkan IP via DHCP...");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Gagal mendapatkan IP dari Router. Cek Kabel LAN.");
  } else {
    Serial.print("Ethernet Ready! IP: ");
    Serial.println(Ethernet.localIP());
    
    initGPS();
  }
}

void loop() {
    // Kirim data setiap 30 detik
    // Read GPS data
    TrackingData data = readGPS();

    // Send to webhook if GPS data is valid
    if (data.valid) {
        if (sendHttpWebhook(data)) {
            Serial.println("[WEBHOOK] Data sent successfully!\n");
        } else {
            Serial.println("[WEBHOOK] Failed to send data\n");
        }
    } else {
        Serial.println("[GPS] Waiting for valid GPS fix...\n");
    }

    // Interval pengiriman data
    delay(SEND_INTERVAL);
}

/**
 * Initialize GPS module
 */
void initGPS() {
    Serial.println("[GPS] Initializing GPS on Serial2...");
    Serial.printf("[GPS] RX=%d, TX=%d, Baud=%d\n", GPS_RX_PIN, GPS_TX_PIN, GPS_BAUD);

    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    delay(100);

    Serial.println("[GPS] GPS initialized!");
    Serial.println("[GPS] Waiting for satellite fix...\n");
}

/**
 * Read GPS data from module
 */
TrackingData readGPS() {
    TrackingData data;
    data.valid = false;

    // Read GPS data for 1 second
    unsigned long start = millis();
    while (millis() - start < 1000) {
        while (gpsSerial.available() > 0) {
            if (gps.encode(gpsSerial.read())) {
                // Check if we have valid location
                if (gps.location.isValid()) {
                    data.valid = true;
                    data.latitude = gps.location.lat();
                    data.longitude = gps.location.lng();
                }

                // Speed in km/h
                if (gps.speed.isValid()) {
                    data.speed = gps.speed.kmph();
                } else {
                    data.speed = 0;
                }

                // Altitude in meters
                if (gps.altitude.isValid()) {
                    data.altitude = gps.altitude.meters();
                } else {
                    data.altitude = 0;
                }

                // Course/heading in degrees
                if (gps.course.isValid()) {
                    data.course = gps.course.deg();
                } else {
                    data.course = 0;
                }

                // Number of satellites
                if (gps.satellites.isValid()) {
                    data.satellites = gps.satellites.value();
                } else {
                    data.satellites = 0;
                }

                // Date and time
                if (gps.date.isValid() && gps.time.isValid()) {
                    char datetime[25];
                    sprintf(datetime, "%04d-%02d-%02d %02d:%02d:%02d",
                            gps.date.year(), gps.date.month(), gps.date.day(),
                            gps.time.hour(), gps.time.minute(), gps.time.second());
                    data.datetime = String(datetime);
                } else {
                    data.datetime = "N/A";
                }
            }
        }
    }

    return data;
}
