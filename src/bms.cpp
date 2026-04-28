#include "bms.h"
#include "rs485.h"

// Forward declarations
static bool parseDataPacket(const uint8_t* pkt, BMSData& out);

// JK-BMS RS485 address
static constexpr uint8_t BMS_ADDRESS  = 4;
static constexpr uint8_t BMS_FUNCTION = 0x10;

// Expected confirmation in Modbus response
static const uint8_t EXPECTED_CONF[] = {0x16, 0x20, 0x00, 0x01};

// Data packet header: 55 AA EB 90
static constexpr uint8_t JK_HEADER[] = {0x55, 0xAA, 0xEB, 0x90};

// ── Public API ───────────────────────────────────────────────────

BMSData readBmsStatus(HardwareSerial& port) {
    BMSData data = {};
    data.online = false;

    // Request: write register 0x1620, 1 register, 2 bytes, value 0x0000
    uint8_t request[] = {
        0x16, 0x20,       // starting address
        0x00, 0x01,       // quantity of registers
        0x02,                 // byte count
        0x00, 0x00        // register value
    };

    RS485Result res = jk_bms_message(port, BMS_ADDRESS, BMS_FUNCTION,
                                     request, sizeof(request));

    if (!res.ok()) {
        Serial.printf("[BMS] Error: %d\n", (int)res.error);
        return data;
    }

    // Verify Modbus confirmation
    if (res.dataLen < sizeof(EXPECTED_CONF) ||
        memcmp(res.data, EXPECTED_CONF, sizeof(EXPECTED_CONF)) != 0) {
        Serial.println("[BMS] Unexpected confirmation");
        return data;
    }

    // Parse 300-byte data packet
    if (!res.hasDataPacket) {
        Serial.println("[BMS] No data packet in response");
        return data;
    }

    if (!parseDataPacket(res.dataPacket, data)) {
        Serial.println("[BMS] Failed to parse data packet");
        return data;
    }

    data.online = true;
    data.timestamp = millis();
    return data;
}

// ═════════════════════════════════════════════════════════════════
// Internal helpers
// ═════════════════════════════════════════════════════════════════

/// Read little-endian uint16 from buffer
static inline uint16_t readU16LE(const uint8_t* p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

/// Read little-endian uint32 from buffer
static inline uint32_t readU32LE(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/// Read little-endian int32 from buffer
static inline int32_t readI32LE(const uint8_t* p) {
    return (int32_t)readU32LE(p);
}

static bool parseDataPacket(const uint8_t* pkt, BMSData& out) {
    // Verify header: 55 AA EB 90
    if (memcmp(pkt, JK_HEADER, sizeof(JK_HEADER)) != 0) {
        return false;
    }

    // Cell voltages start at offset 6, each 2 bytes LE (mV)
    // 8 cells for JK-B2A8S20P
    out.cellCount = 8;
    for (uint8_t i = 0; i < out.cellCount; i++) {
        out.cellVoltage[i] = readU16LE(&pkt[6 + i * 2]);
    }

    // Offsets from JK-BMS register map documentation.
    // Doc offsets start from data area; packet has 6-byte header (55 AA EB 90 XX XX),
    // so actual packet offset = doc_offset + 6.
    out.power        = readU32LE(&pkt[148 + 6]);  // doc[148] mW
    out.current      = readI32LE(&pkt[152 + 6]);  // doc[152] mA, signed
    out.soc          = pkt[167 + 6];                // doc[167] %
    out.capRemain    = readU32LE(&pkt[168 + 6]);  // doc[168] mAh
    out.capNominal   = readU32LE(&pkt[172 + 6]);  // doc[172] mAh
    out.chargeMOS    = pkt[192 + 6] == 1;           // doc[192]
    out.dischargeMOS = pkt[193 + 6] == 1;           // doc[193]

    return true;
}
