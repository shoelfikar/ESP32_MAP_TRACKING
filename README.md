# ESP32 GPS Tracker (PELNI) — W5500 Ethernet + NEO-M8N

Firmware GPS tracker berbasis **ESP32 + W5500 Ethernet + NEO-M8N** untuk fleet PELNI. Device membaca posisi GPS dan mengirimkannya ke server via HTTP POST di jaringan LAN.

Fitur utama dibanding tracker biasa:

- **Zero-touch provisioning** — device otomatis menemukan IP server via **UDP discovery** saat boot. Tidak ada IP server yang di-hardcode; firmware sama persis untuk semua unit fleet.
- **Verifikasi HMAC-SHA256 + TOFU pinning** — reply discovery ditandatangani shared secret, lalu server yang pertama terverifikasi "di-pin" (Trust On First Use) agar device tidak bisa dibelokkan ke server palsu.
- **OTA firmware update** via HTTP (token per-device yang di-generate otomatis saat first boot).
- **Web UI bawaan** — status device, konfigurasi manual server (fallback saat discovery gagal), trigger discovery/OTA, semuanya dari browser.
- **Boot sync** — setiap reboot, device melapor ke server (identitas, versi firmware, OTA token).
- **Reliability** — watchdog timer, auto-reconnect, LED status, OTA rollback otomatis kalau firmware baru gagal online.

> Detail protokol discovery: [docs/udp-discovery-plan.md](docs/udp-discovery-plan.md). Daemon server: [server-discovery/README.md](server-discovery/README.md).

---

## Daftar Isi

1. [Arsitektur Singkat](#arsitektur-singkat)
2. [Hardware yang Dibutuhkan](#hardware-yang-dibutuhkan)
3. [Wiring Diagram](#wiring-diagram)
4. [Quick Start](#quick-start)
5. [Konfigurasi (`config.h`)](#konfigurasi-configh)
6. [Build & Upload](#build--upload)
7. [Server Discovery Daemon](#server-discovery-daemon)
8. [Web UI & Endpoint](#web-ui--endpoint)
9. [OTA Firmware Update](#ota-firmware-update)
10. [Alur State Device](#alur-state-device)
11. [Troubleshooting](#troubleshooting)
12. [Struktur Project](#struktur-project)

---

## Arsitektur Singkat

```
   ┌──────────────┐   UDP "DISCOVER?"   ┌──────────────────────┐
   │              │ ──────────────────► │  discovery-responder │  (Go daemon di server)
   │  ESP32 GPS   │ ◄────────────────── │  reply + HMAC sig    │
   │   Tracker    │   host:port:path    └──────────────────────┘
   │  (W5500 LAN) │
   │              │   HTTP POST GPS      ┌──────────────────────┐
   │              │ ──────────────────► │   HTTP / API Server   │  (Laravel / webhook)
   └──────────────┘   + boot sync + OTA  └──────────────────────┘
```

Saat boot device meresolusi alamat server dengan urutan: **cache NVS → config manual → UDP discovery**. Setelah terverifikasi, alamat di-pin (TOFU) dan disimpan ke NVS untuk boot berikutnya.

---

## Hardware yang Dibutuhkan

| Komponen | Keterangan |
|----------|------------|
| ESP32 DevKit (`esp32dev`) atau NodeMCU-32S (`nodemcu-32s`) | Main controller |
| W5500 Ethernet Module | Koneksi LAN via SPI |
| NEO-M8N GPS Module | GPS receiver via UART |
| Kabel LAN | Ke switch/router (subnet sama dengan server) |
| Kabel jumper | Untuk wiring |

---

## Wiring Diagram

### W5500 Ethernet Module → ESP32 (SPI)

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

### NEO-M8N GPS Module → ESP32 (UART / Serial2)

| GPS Pin | ESP32 Pin | Keterangan |
|---------|-----------|------------|
| VCC | 3.3V atau 5V | Power supply |
| GND | GND | Ground |
| TX | GPIO 16 (RX2) | GPS → ESP32 |
| RX | GPIO 17 (TX2) | ESP32 → GPS |

```
NEO-M8N        ESP32
┌─────┐       ┌─────┐
│ VCC │───────│3.3V │ (atau 5V)
│ GND │───────│ GND │
│ TX  │───────│GP16 │ (RX2)
│ RX  │───────│GP17 │ (TX2)
└─────┘       └─────┘
```

> Pin di atas dapat diubah lewat [src/config.example.h](src/config.example.h) (`W5500_CS_PIN`, `W5500_RST_PIN`, `GPS_RX_PIN`, `GPS_TX_PIN`).

---

## Quick Start

```bash
# 1. Clone project
git clone <repository-url> ESP32_MAP_TRACKING
cd ESP32_MAP_TRACKING

# 2. Buat config dari template (config.h di-gitignore)
cp src/config.example.h src/config.h

# 3. Set shared secret discovery (WAJIB sama dengan daemon server)
#    Edit DISCOVERY_SHARED_SECRET di src/config.h — pakai random hex 32-byte:
openssl rand -hex 32

# 4. Wiring hardware + colok LAN + USB, lalu build & upload
pio run -e esp32dev -t upload

# 5. Monitor serial
pio device monitor
```

Setelah boot, device mengambil IP via DHCP, lalu:
- Kalau daemon discovery hidup di subnet yang sama → server ketemu otomatis, mulai kirim GPS.
- Kalau discovery gagal → device masuk state `NO_SERVER`, buka Web UI-nya (`http://<ip-device>/`) untuk input server manual.

---

## Konfigurasi (`config.h`)

`src/config.h` **tidak di-commit** (ada di `.gitignore`) — berisi secret & setelan per-deployment. Buat dari [src/config.example.h](src/config.example.h).

Field yang paling penting:

```cpp
// Device ID = prefix + Chip ID (auto). Contoh: "GPS_" + "A1B2C3" = "GPS_A1B2C3"
#define DEVICE_ID_PREFIX    "GPS_"

// Server: DIKOSONGKAN by design — device auto-discover via UDP.
// Isi hanya kalau mau hardcode fallback (tidak disarankan).
#define SERVER_HOST         ""
#define SERVER_PATH         ""
#define SERVER_PORT         0
#define SERVER_SYNC_PATH    "/api/gps/sync"   // endpoint sync identitas device

// UDP Discovery — harus match daemon server
#define DISCOVERY_PORT              4210
#define DISCOVERY_SHARED_SECRET     "ganti-dengan-random-hex-32-byte"

// Interval kirim GPS
#define SEND_INTERVAL_NORMAL    30000    // 30 dtk saat GPS valid
#define SEND_INTERVAL_NO_FIX    300000   // 5 mnt saat belum ada fix
```

> ⚠️ `DISCOVERY_SHARED_SECRET` **harus identik** dengan env `SHARED_SECRET` di daemon [server-discovery](server-discovery/README.md). Kalau beda, semua reply discovery ditolak (HMAC mismatch).

> OTA token **tidak** di-set manual — di-generate otomatis saat first boot dan disimpan di NVS (lihat serial log / Web UI).

---

## Build & Upload

Ada dua environment PlatformIO di [platformio.ini](platformio.ini):

| Env | Board |
|-----|-------|
| `esp32dev` | ESP32 DevKit (default) |
| `nodemcu-32s` | NodeMCU-32S |

```bash
# Build
pio run -e esp32dev

# Upload
pio run -e esp32dev -t upload

# Serial monitor (115200 baud)
pio device monitor
```

**Via VSCode:** install extension *PlatformIO IDE*, buka folder project, pilih environment di status bar, lalu klik **Upload** / **Monitor**.

### Alternatif: Web Flasher (tanpa PlatformIO)

[flash.html](flash.html) adalah flasher berbasis browser (esptool-js) untuk flashing firmware langsung dari Chrome/Edge lewat Web Serial — berguna untuk deploy di lapangan tanpa toolchain. Buka file-nya di browser, colok ESP32 via USB, lalu ikuti wizard.

Flashing memakai **satu file** `firmware-merged.bin` (di-flash di offset `0x0`) — cukup untuk ESP32 baru/kosong. Pilih file di card *Pilih Binary*, Connect, lalu Flash.

Card **Serial Monitor** memungkinkan melihat log serial langsung dari browser (default 115200 baud) tanpa perlu flashing — klik **Open Monitor**, dan **Reset** untuk melihat log boot dari awal.

#### Membuat `firmware-merged.bin` (satu file)

ESP32 baru **tidak bisa** di-flash hanya dengan `firmware.bin` — butuh bootloader + partition table. Untuk menghasilkan satu file gabungan yang bisa di-flash di offset `0x0`, jalankan script berikut (menggabungkan keempat binary di root):

```bash
./make_merged.sh
# → firmware-merged.bin (flash di offset 0x0)
```

Flash via CLI (chip baru):

```bash
esptool.py --chip esp32 write_flash 0x0 firmware-merged.bin
```

### Verifikasi Serial

Output yang diharapkan setelah upload:

```
Ethernet Ready! IP: 192.168.1.xxx
Web: http://192.168.1.xxx:80
[GPS] Initializing GPS on Serial2... RX=16, TX=17, Baud=9600
Starting UDP discovery (port 4210)...
Discovery: reply verified, server pinned → RUNNING
```

---

## Server Discovery Daemon

Agar device menemukan server otomatis, jalankan daemon Go [server-discovery](server-discovery/) di host yang **satu subnet** dengan device (UDP broadcast tidak menyeberangi router/VLAN).

```bash
cd server-discovery
go build -o discovery-responder ./...

SHARED_SECRET="<hex-32-byte-sama-dengan-config.h>" \
SERVER_PORT=8000 \
SERVER_PATH=/api/webhook/gps-tracking \
./discovery-responder
```

Setup lengkap (mode implicit/explicit host, systemd, cross-compile, verifikasi manual): **[server-discovery/README.md](server-discovery/README.md)**.

---

## Web UI & Endpoint

Setiap device menjalankan web server di `http://<ip-device>:80`. Selain dashboard status, tersedia REST endpoint (dipakai UI dan tooling):

| Method | Endpoint | Fungsi |
|--------|----------|--------|
| `GET`  | `/` | Dashboard status device |
| `GET`  | `/settings` | Halaman konfigurasi |
| `GET`  | `/api/config` | Baca konfigurasi server aktif |
| `POST` | `/api/config` | Set server manual (host/port/path) |
| `POST` | `/api/config/reset` | Reset config ke default (clear NVS) |
| `POST` | `/api/server/ping` | Cek reachability server |
| `POST` | `/api/server/sync` | Trigger sync identitas ke server |
| `GET`  | `/api/server/status` | State device (`running`, `no_server`, dll) |
| `POST` | `/api/server/discover` | Trigger UDP discovery sekarang |
| `POST` | `/api/server/reset-trust` | Hapus TOFU pin (pakai kalau server sah pindah IP) |
| `GET`  | `/api/device/status` | Info device (id, mac, firmware, uptime) |
| `POST` | `/api/firmware/update` | OTA upload firmware (butuh header token) |

### Payload Telemetri ke Server

Device POST JSON ke `SERVER_PATH` setiap `SEND_INTERVAL_*`. Field GPS hanya dikirim saat fix valid (`status: "online"`); saat `no_fix` hanya `satellites` yang ikut.

| Field | Satuan / Format | Keterangan |
|-------|-----------------|------------|
| `device_id` | string | ID device (`DEVICE_ID_PREFIX` + MAC) |
| `status` | `online` \| `no_fix` | Ada/tidaknya fix valid |
| `latitude` / `longitude` | derajat desimal (WGS84) | — |
| `speed` | **knots** (SOG) | Speed over ground, satuan maritim |
| `altitude` | meter | Di atas mean sea level |
| `course` | derajat (0–359) | Course over ground, true north |
| `satellites` | integer | Jumlah satelit terkunci |
| `timestamp` | ISO 8601 UTC | `YYYY-MM-DDTHH:MM:SSZ` dari GPS |
| `ip`, `uptime_sec`, `free_heap` | — | Info sistem device |

> **Catatan versi:** `speed` sebelumnya dikirim dalam **km/h**. Sejak perubahan ke knots, pastikan semua device sudah di-flash firmware baru sebelum backend mengasumsikan satuan knot — jangan sampai ada campuran satuan di database.

---

## OTA Firmware Update

Device menerima firmware baru via HTTP POST tanpa perlu kabel USB.

1. Ambil **OTA token** device (dari serial log first boot atau `GET /api/device/status`). Token unik per-device, tersimpan di NVS.
2. Upload binary hasil build (`.pio/build/<env>/firmware.bin`):

```bash
curl -X POST "http://<ip-device>/api/firmware/update" \
  -H "X-Update-Token: <ota-token-device>" \
  --data-binary @.pio/build/esp32dev/firmware.bin
```

3. Device flash image, reboot ke firmware baru, dan menjalankan **health gate**: jika firmware baru gagal online (network tidak up), bootloader otomatis **rollback** ke firmware sebelumnya. Kalau sehat, image baru dikonfirmasi permanen.

> Token salah → `401 Unauthorized`. Ganti token dengan `POST /api/config/reset` (regenerate saat boot berikutnya).

---

## Alur State Device

| State | LED | Arti |
|-------|-----|------|
| `INIT` / `NETWORK_CONNECTING` | — | Boot, ambil DHCP |
| `RUNNING` | solid saat kirim | Server terverifikasi, mengirim GPS |
| `NO_SERVER` | double-blink cepat | Network OK tapi server belum ketemu — Web UI aktif untuk override manual; retry discovery dengan backoff (5s→10s→30s→60s) |
| `ERROR_NETWORK` | — | Ethernet/DHCP gagal |
| `ERROR_FATAL` | — | Kegagalan tak terpulihkan |

Resolusi server saat boot: **cache NVS → config manual → UDP discovery** → sync + TOFU pin.

---

## Troubleshooting

### W5500 tidak dapat IP (0.0.0.0 / `ERROR_NETWORK`)
- Cek wiring SPI (MISO/MOSI/SCK/CS) dan pin RST.
- Pastikan kabel LAN terhubung ke switch/router aktif (DHCP jalan).

### GPS tidak mendeteksi satelit
- Modul harus di area terbuka (outdoor); cold start 1–5 menit.
- Cek TX/RX **ter-cross** (GPS TX → ESP32 RX, GPS RX → ESP32 TX).

### Device stuck di `NO_SERVER`
- Pastikan daemon discovery hidup **di subnet yang sama** dan port `4210/udp` tidak diblok firewall.
- Pastikan `DISCOVERY_SHARED_SECRET` (config.h) = `SHARED_SECRET` (daemon) — HMAC mismatch = reply ditolak.
- Fallback: buka `http://<ip-device>/settings`, isi host/port/path server manual.

### Server sah pindah IP tapi device menolak reply baru
- TOFU pin masih mengunci fingerprint lama. Jalankan `POST /api/server/reset-trust`, lalu trigger `POST /api/server/discover`.

### Serial Monitor kosong
- Baud rate = `115200`. Tekan tombol EN/Reset pada ESP32.

---

## Struktur Project

```
ESP32_MAP_TRACKING/
├── src/
│   ├── modules/
│   │   ├── gps_module.h            # Parsing NEO-M8N (TinyGPSPlus)
│   │   ├── network_module.h        # W5500 Ethernet + HTTP client + sync
│   │   ├── discovery_module.h      # UDP discovery + verifikasi HMAC
│   │   ├── config_manager.h        # Persistensi NVS (server, OTA token, TOFU)
│   │   ├── webserver_module.h      # Web server + REST endpoint + OTA handler
│   │   ├── webpage_renderer.h      # Render dashboard
│   │   └── webpage_settings.h      # Render halaman settings
│   ├── main.cpp                    # Aplikasi utama (state machine, boot flow)
│   ├── config.example.h            # Template konfigurasi
│   └── config.h                    # Konfigurasi aktual (gitignored)
├── server-discovery/               # Daemon Go UDP discovery responder
│   ├── discovery-responder.go
│   └── README.md
├── docs/
│   └── udp-discovery-plan.md       # Spec protokol discovery
├── flash.html                      # Web flasher (esptool-js) + serial monitor
├── make_merged.sh                  # Gabung 4 binary → firmware-merged.bin (flash 1 file)
├── platformio.ini                  # Environment esp32dev & nodemcu-32s
└── README.md
```

---

## Library Dependencies

```ini
lib_deps =
    mikalhart/TinyGPSPlus@^1.0.3
    bblanchon/ArduinoJson@^6.21.3
    arduino-libraries/Ethernet @ ^2.0.2
```

OTA & NVS memakai komponen bawaan ESP-IDF/Arduino (`Update`, `Preferences`, `esp_ota_ops`) — tidak perlu dependency tambahan.
