# ESP32 GPS Tracker with W5500 Ethernet

Project untuk tracking GPS menggunakan ESP32 + W5500 Ethernet Module + NEO-M8N GPS Module.

## Hardware yang Dibutuhkan

| Komponen | Keterangan |
|----------|------------|
| ESP32 DevKit | Main controller |
| W5500 Ethernet Module | Koneksi LAN/Internet |
| NEO-M8N GPS Module | GPS receiver |
| Kabel LAN | Koneksi ke router |
| Kabel jumper | Untuk wiring |

---

## Wiring Diagram

### W5500 Ethernet Module -> ESP32

W5500 menggunakan komunikasi SPI:

| W5500 Pin | ESP32 Pin | Keterangan |
|-----------|-----------|------------|
| VCC | 3.3V | Power supply |
| GND | GND | Ground |
| MISO | GPIO 19 | SPI Master In Slave Out |
| MOSI | GPIO 23 | SPI Master Out Slave In |
| SCK | GPIO 18 | SPI Clock |
| CS | GPIO 5 | Chip Select |
| RST | GPIO 4 | Reset |

```
W5500          ESP32
┌─────┐       ┌─────┐
│ VCC │───────│3.3V │
│ GND │───────│ GND │
│MISO │───────│GP19 │
│MOSI │───────│GP23 │
│ SCK │───────│GP18 │
│ CS  │───────│GP5  │
│ RST │───────│GP4  │
└─────┘       └─────┘
```

### NEO-M8N GPS Module -> ESP32

GPS menggunakan komunikasi Serial (UART):

| GPS Pin | ESP32 Pin | Keterangan |
|---------|-----------|------------|
| VCC | 3.3V atau 5V | Power supply |
| GND | GND | Ground |
| TX | GPIO 16 (RX2) | GPS mengirim data ke ESP32 |
| RX | GPIO 17 (TX2) | ESP32 mengirim command ke GPS |

```
NEO-M8N        ESP32
┌─────┐       ┌─────┐
│ VCC │───────│3.3V │ (atau 5V)
│ GND │───────│ GND │
│ TX  │───────│GP16 │ (RX2)
│ RX  │───────│GP17 │ (TX2)
└─────┘       └─────┘
```

### Wiring Lengkap

```
                    ┌──────────────┐
                    │    ESP32     │
                    │   DevKit     │
    ┌───────────────┤              ├───────────────┐
    │               │              │               │
    │    W5500      │              │    NEO-M8N    │
    │   ┌─────┐     │              │     ┌─────┐   │
    │   │ VCC │─────┤3.3V      3.3V├─────│ VCC │   │
    │   │ GND │─────┤GND        GND├─────│ GND │   │
    │   │MISO │─────┤GPIO19        │     │     │   │
    │   │MOSI │─────┤GPIO23        │     │     │   │
    │   │ SCK │─────┤GPIO18        │     │     │   │
    │   │ CS  │─────┤GPIO5         │     │     │   │
    │   │ RST │─────┤GPIO4         │     │     │   │
    │   └─────┘     │        GPIO16├─────│ TX  │   │
    │               │        GPIO17├─────│ RX  │   │
    │               │              │     └─────┘   │
    │               └──────────────┘               │
    │                                              │
    └──────────────────────────────────────────────┘
```

---

## Konfigurasi Pin (config.h)

```cpp
// W5500 Ethernet
#define W5500_CS    5
#define W5500_RST   4

// GPS NEO-M8N
#define GPS_RX_PIN  16    // ESP32 RX <- GPS TX
#define GPS_TX_PIN  17    // ESP32 TX -> GPS RX
#define GPS_BAUD    9600
```

---

## Cara Menjalankan Project

### 1. Install PlatformIO

**Via VSCode:**
1. Buka VSCode
2. Pergi ke Extensions (Ctrl+Shift+X)
3. Cari "PlatformIO IDE"
4. Klik Install

### 2. Clone/Download Project

```bash
cd ~/Documents/development/project/microcontroller
git clone <repository-url> ESP32_MAP_TRACKING
```

### 3. Konfigurasi

File `src/config.h` tidak disertakan di repository karena berisi konfigurasi spesifik (server host, koordinat default, dll). Buat dari template:

```bash
cp src/config.example.h src/config.h
```

Kemudian edit `src/config.h` sesuai kebutuhan:

```cpp
// Server webhook tujuan
#define SERVER_HOST         "your-server.example.com"

// Device ID
#define DEVICE_ID           "ESP32_GPS_001"

// Interval pengiriman data (ms)
#define SEND_INTERVAL_NORMAL    30000    // 30 detik

// Default location (fallback saat GPS belum fix)
#define DEFAULT_LAT         -6.200000
#define DEFAULT_LNG         106.800000
```

> **Catatan:** Jangan commit `src/config.h` ke git. File ini sudah ada di `.gitignore`.

### 4. Hubungkan Hardware

1. Wiring W5500 dan GPS sesuai diagram di atas
2. Hubungkan kabel LAN dari W5500 ke router
3. Hubungkan ESP32 ke komputer via USB

### 5. Build & Upload

**Via Terminal:**
```bash
cd ESP32_MAP_TRACKING

# Build project
pio run

# Upload ke ESP32
pio run -t upload

# Monitor serial output
pio device monitor
```

**Via VSCode:**
1. Buka folder project di VSCode
2. Klik icon PlatformIO di sidebar
3. Pilih `esp32dev` > `Upload`
4. Untuk monitor: pilih `Monitor`

### 6. Verifikasi

Buka Serial Monitor (115200 baud). Output yang diharapkan:

```
Mendapatkan IP via DHCP...
Ethernet Ready! IP: 192.168.1.xxx
[GPS] Initializing GPS on Serial2...
[GPS] RX=16, TX=17, Baud=9600
[GPS] GPS initialized!
[GPS] Waiting for satellite fix...
```

---

## Troubleshooting

### W5500 tidak mendapat IP (0.0.0.0)
- Cek wiring SPI (MISO, MOSI, SCK, CS)
- Pastikan kabel LAN terhubung ke router
- Cek koneksi RST pin

### GPS tidak mendeteksi satelit
- Pastikan GPS module berada di area terbuka (outdoor)
- Tunggu 1-5 menit untuk cold start
- Cek wiring TX/RX (perhatikan cross connection)

### Serial Monitor tidak menampilkan output
- Pastikan baud rate = 115200
- Tekan tombol EN/Reset pada ESP32

---

## Library Dependencies

```ini
lib_deps =
    mikalhart/TinyGPSPlus@^1.0.3
    bblanchon/ArduinoJson@^6.21.3
    arduino-libraries/Ethernet @ ^2.0.2
```

---

## Struktur Project

```
ESP32_MAP_TRACKING/
├── src/
│   ├── modules/
│   │   ├── gps_module.h        # GPS (NEO-M8N) module
│   │   ├── network_module.h    # Ethernet (W5500) & HTTP module
│   │   └── webserver_module.h  # Built-in web server module
│   ├── main.cpp                # Main program
│   ├── config.h                # Konfigurasi (tidak di-commit, buat dari template)
│   └── config.example.h        # Template konfigurasi
├── platformio.ini              # PlatformIO configuration
└── README.md                   # Dokumentasi
```
