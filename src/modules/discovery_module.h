#ifndef DISCOVERY_MODULE_H
#define DISCOVERY_MODULE_H

/**
 * @file discovery_module.h
 * @brief UDP discovery — find the GPS server on the LAN via broadcast.
 *
 * Protocol (JSON):
 *   ESP32 → broadcast 255.255.255.255:DISCOVERY_PORT
 *     {"q":"gps-server","dev":"...","mac":"...","fw":"...","nonce":"<32-hex>","ts":<ms>}
 *
 *   server → unicast back to ESP32 source port
 *     {"r":"gps-server","host":"...","port":N,"path":"...","nonce":"<echo>","sig":"<64-hex>"}
 *
 *   sig = HMAC-SHA256(DISCOVERY_SHARED_SECRET, "host|port|path|nonce")  hex-encoded
 *
 * State machine is non-blocking and advanced via tick() from loop().
 * On-demand socket: udp.begin() at start, udp.stop() when done — Don't keep-alive.
 */

#include <Arduino.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <ArduinoJson.h>
#include <esp_random.h>
#include "mbedtls/md.h"
#include "../config.h"

enum class DiscoveryState : uint8_t {
    IDLE = 0,
    WAITING_REPLY,
    DONE_SUCCESS,
    DONE_FAIL
};

struct DiscoveryResult {
    char host[64];
    uint16_t port;
    char path[128];
};

class DiscoveryModule {
public:
    /**
     * Begin a discovery attempt. Sends a burst of probes and waits up to
     * DISCOVERY_PROBE_TIMEOUT_MS for a valid (HMAC-verified) reply per probe.
     *
     * Call tick() from loop() (or in a tight loop with WDT reset at boot) to
     * advance the state machine.
     */
    bool start(const char* deviceId, const uint8_t* mac, const char* firmwareVersion) {
        if (_state == DiscoveryState::WAITING_REPLY) {
            return false;  // already in flight
        }

        if (!_udp.begin(DISCOVERY_PORT)) {
            Serial.println("[Discovery] udp.begin() failed (out of sockets?)");
            _state = DiscoveryState::DONE_FAIL;
            return false;
        }
        _udpOpen = true;

        _probesSent = 0;
        _state = DiscoveryState::WAITING_REPLY;
        _probeStartMs = 0;  // forces immediate first send in tick()

        strncpy(_deviceId, deviceId, sizeof(_deviceId) - 1);
        _deviceId[sizeof(_deviceId) - 1] = '\0';
        memcpy(_mac, mac, 6);
        strncpy(_fw, firmwareVersion, sizeof(_fw) - 1);
        _fw[sizeof(_fw) - 1] = '\0';

        return true;
    }

    /**
     * Advance state machine. Returns true when state transitions to DONE_*.
     * Caller should check isSuccess() / result() afterwards and then reset().
     */
    bool tick() {
        if (_state != DiscoveryState::WAITING_REPLY) {
            return false;
        }

        const uint32_t now = millis();

        // Send a probe if we haven't yet or the previous probe timed out.
        if (_probeStartMs == 0 || (now - _probeStartMs) >= DISCOVERY_PROBE_TIMEOUT_MS) {
            if (_probesSent >= DISCOVERY_PROBE_BURST) {
                // Exhausted retries
                _state = DiscoveryState::DONE_FAIL;
                Serial.println("[Discovery] all probes exhausted, no reply");
                closeSocket();
                return true;
            }
            sendProbe();
            _probesSent++;
            _probeStartMs = now;
        }

        // Drain any incoming packets
        int packetSize = _udp.parsePacket();
        if (packetSize > 0 && packetSize < (int)sizeof(_rxBuf)) {
            int n = _udp.read((unsigned char*)_rxBuf, sizeof(_rxBuf) - 1);
            if (n > 0) {
                _rxBuf[n] = '\0';
                if (processReply(_rxBuf)) {
                    _state = DiscoveryState::DONE_SUCCESS;
                    closeSocket();
                    return true;
                }
                // Invalid reply (bad sig, wrong nonce, etc.) — keep listening
            }
        }

        return false;
    }

    bool isDone() const {
        return _state == DiscoveryState::DONE_SUCCESS || _state == DiscoveryState::DONE_FAIL;
    }
    bool isSuccess() const { return _state == DiscoveryState::DONE_SUCCESS; }
    bool isIdle() const { return _state == DiscoveryState::IDLE; }
    const DiscoveryResult& result() const { return _result; }

    /**
     * Compute SHA-256 fingerprint over "host|port|path" — used for TOFU pinning.
     * outHex must be at least 65 bytes (64 hex + null).
     */
    static void computeFingerprint(const char* host, uint16_t port, const char* path,
                                   char* outHex, size_t outSize) {
        if (outSize < 65) {
            if (outSize > 0) outHex[0] = '\0';
            return;
        }
        char msg[256];
        int n = snprintf(msg, sizeof(msg), "%s|%u|%s", host, port, path);
        if (n <= 0 || n >= (int)sizeof(msg)) {
            outHex[0] = '\0';
            return;
        }
        uint8_t hash[32];
        const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
        if (!info || mbedtls_md(info, (const uint8_t*)msg, (size_t)n, hash) != 0) {
            outHex[0] = '\0';
            return;
        }
        for (int i = 0; i < 32; i++) {
            sprintf(outHex + i * 2, "%02x", hash[i]);
        }
        outHex[64] = '\0';
    }

    /**
     * Reset state to IDLE. Closes UDP socket if still open.
     * Always safe to call.
     */
    void reset() {
        closeSocket();
        _state = DiscoveryState::IDLE;
        _probesSent = 0;
        _probeStartMs = 0;
        _nonce[0] = '\0';
    }

private:
    EthernetUDP _udp;
    bool _udpOpen = false;
    DiscoveryState _state = DiscoveryState::IDLE;

    char _deviceId[24] = {0};
    uint8_t _mac[6] = {0};
    char _fw[16] = {0};
    char _nonce[33] = {0};      // 32 hex + null
    uint8_t _probesSent = 0;
    uint32_t _probeStartMs = 0;

    char _rxBuf[512] = {0};
    DiscoveryResult _result = {{0}, 0, {0}};

    void closeSocket() {
        if (_udpOpen) {
            _udp.stop();
            _udpOpen = false;
        }
    }

    void sendProbe() {
        // Fresh nonce for this probe (anti-replay)
        generateNonce(_nonce, sizeof(_nonce));

        char macBuf[18];
        snprintf(macBuf, sizeof(macBuf), "%02X:%02X:%02X:%02X:%02X:%02X",
                 _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5]);

        StaticJsonDocument<256> doc;
        doc["q"] = "gps-server";
        doc["dev"] = _deviceId;
        doc["mac"] = macBuf;
        doc["fw"] = _fw;
        doc["nonce"] = _nonce;
        doc["ts"] = (uint32_t)millis();

        char buffer[256];
        size_t len = serializeJson(doc, buffer, sizeof(buffer));
        if (len == 0) return;

        Serial.printf("[Discovery] probe #%u (nonce=%s)\n", _probesSent + 1, _nonce);
        _udp.beginPacket(IPAddress(255, 255, 255, 255), DISCOVERY_PORT);
        _udp.write((const uint8_t*)buffer, len);
        _udp.endPacket();
    }

    bool processReply(const char* json) {
        StaticJsonDocument<384> doc;
        DeserializationError err = deserializeJson(doc, json);
        if (err) {
            Serial.printf("[Discovery] reply not JSON: %s\n", err.c_str());
            return false;
        }

        const char* r = doc["r"] | "";
        if (strcmp(r, "gps-server") != 0) {
            return false;  // wrong magic, ignore quietly
        }

        const char* hostFromJson = doc["host"] | "";
        uint16_t port = doc["port"] | 0;
        const char* path = doc["path"] | "";
        const char* nonce = doc["nonce"] | "";
        const char* sig = doc["sig"] | "";

        if (port == 0 || path[0] == '\0') {
            Serial.println("[Discovery] reply missing required fields (port/path)");
            return false;
        }

        if (strcmp(nonce, _nonce) != 0) {
            Serial.println("[Discovery] reply nonce mismatch (replay or stale)");
            return false;
        }

        // Host resolution (hybrid mode):
        //   - If reply explicitly carries "host": use it as canonical (supports
        //     hostnames, sidecar where responder ≠ HTTP server). HMAC must
        //     sign over that explicit value.
        //   - If "host" omitted: derive from UDP source IP. Responder must have
        //     signed over its own source-equivalent IP. Spoofing the source IP
        //     breaks HMAC verification — security held.
        char effectiveHost[64];
        bool hostFromSource = false;
        if (hostFromJson[0] != '\0') {
            strncpy(effectiveHost, hostFromJson, sizeof(effectiveHost) - 1);
            effectiveHost[sizeof(effectiveHost) - 1] = '\0';
        } else {
            IPAddress srcIP = _udp.remoteIP();
            snprintf(effectiveHost, sizeof(effectiveHost), "%u.%u.%u.%u",
                     srcIP[0], srcIP[1], srcIP[2], srcIP[3]);
            hostFromSource = true;
        }

        if (!verifyHmac(effectiveHost, port, path, nonce, sig)) {
            Serial.printf("[Discovery] HMAC INVALID for host=%s (source=%s) — rejecting\n",
                          effectiveHost, hostFromSource ? "udp_src" : "json");
            return false;
        }

        strncpy(_result.host, effectiveHost, sizeof(_result.host) - 1);
        _result.host[sizeof(_result.host) - 1] = '\0';
        _result.port = port;
        strncpy(_result.path, path, sizeof(_result.path) - 1);
        _result.path[sizeof(_result.path) - 1] = '\0';

        Serial.printf("[Discovery] verified reply (host %s): %s:%u%s\n",
                      hostFromSource ? "from UDP src" : "from JSON",
                      _result.host, _result.port, _result.path);
        return true;
    }

    static void generateNonce(char* out, size_t outSize) {
        static const char hex[] = "0123456789abcdef";
        size_t maxBytes = (outSize - 1) / 2;
        if (maxBytes > 16) maxBytes = 16;
        size_t pos = 0;
        for (size_t i = 0; i < maxBytes; i++) {
            uint8_t b = (uint8_t)(esp_random() & 0xFF);
            out[pos++] = hex[b >> 4];
            out[pos++] = hex[b & 0x0F];
        }
        out[pos] = '\0';
    }

    static bool verifyHmac(const char* host, uint16_t port, const char* path,
                           const char* nonce, const char* sigHex) {
        if (!sigHex || strlen(sigHex) != 64) return false;

        char msg[384];
        int n = snprintf(msg, sizeof(msg), "%s|%u|%s|%s", host, port, path, nonce);
        if (n <= 0 || n >= (int)sizeof(msg)) return false;

        const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
        if (!info) return false;

        uint8_t computed[32];
        const char* secret = DISCOVERY_SHARED_SECRET;
        if (mbedtls_md_hmac(info,
                            (const uint8_t*)secret, strlen(secret),
                            (const uint8_t*)msg, (size_t)n,
                            computed) != 0) {
            return false;
        }

        // Parse hex sig
        uint8_t got[32];
        for (int i = 0; i < 32; i++) {
            char hi = sigHex[i * 2];
            char lo = sigHex[i * 2 + 1];
            int h = hexVal(hi);
            int l = hexVal(lo);
            if (h < 0 || l < 0) return false;
            got[i] = (uint8_t)((h << 4) | l);
        }

        // Constant-time compare
        uint8_t diff = 0;
        for (int i = 0; i < 32; i++) diff |= computed[i] ^ got[i];
        return diff == 0;
    }

    static int hexVal(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        return -1;
    }
};

#endif // DISCOVERY_MODULE_H
