#include "bms.h"
#include "rs485.h"

// Forward declarations
static bool parseRegisters(const uint8_t* payload, size_t len, BMSData& out);

// JK-BMS RS485 address
static constexpr uint8_t BMS_ADDRESS = 4;

// Read Holding Registers from block 0x1200.
// Modbus register address = 0x1200 + doc_hex_offset.
//
// Doc offset 0x0090 (144) BatVolt          → reg 0x1290
// Doc offset 0x0094 (148) BatWatt          → reg 0x1294
// Doc offset 0x0098 (152) BatCurrent       → reg 0x1298
// Doc offset 0x00A6 (166) BalanSta + SOC   → reg 0x12A6
// Doc offset 0x00A8 (168) SOCCapRemain     → reg 0x12A8
// Doc offset 0x00AC (172) SOCFullChargeCap → reg 0x12AC
// Doc offset 0x00C0 (192) Charge+Discharge → reg 0x12C0
//
// We read from 0x1290 to 0x12C2 (not inclusive) = 0x32 (50) registers = 100 bytes.
static constexpr uint16_t REG_START = 0x1290;
static constexpr uint16_t REG_COUNT = 0x12C2 - 0x1290;  // 50 registers
static constexpr size_t   REG_PAYLOAD_BYTES = REG_COUNT * 2;  // 100 bytes

// ── Public API ───────────────────────────────────────────────────

BMSData readBmsStatus(HardwareSerial& port) {
    BMSData data = {};
    data.online = false;

    // Modbus function 0x03 — Read Holding Registers
    // Data: [start_reg_hi, start_reg_lo, count_hi, count_lo]
    uint8_t request[] = {
        (uint8_t)(REG_START >> 8), (uint8_t)(REG_START & 0xFF),  // starting address
        (uint8_t)(REG_COUNT >> 8), (uint8_t)(REG_COUNT & 0xFF),  // quantity of registers
    };

    RS485Result res =
        modbus_message(port, BMS_ADDRESS, 0x03, request, sizeof(request));

    if (!res.ok()) {
        Serial.printf("[BMS] Error: %d\n", (int)res.error);
        return data;
    }

    // Response payload from modbus_message: [byte_count, data...]
    if (res.dataLen < 1) {
        Serial.println("[BMS] Empty response");
        return data;
    }

    uint8_t byteCount = res.data[0];
    if (res.dataLen < 1 + byteCount) {
        Serial.printf("[BMS] Short response: %d bytes, expected %d\n",
                      (int)res.dataLen, byteCount + 1);
        return data;
    }

    if (!parseRegisters(&res.data[1], byteCount, data)) {
        Serial.println("[BMS] Failed to parse registers");
        return data;
    }

    data.online = true;
    data.timestamp = millis();
    return data;
}

bool toggleBmsControl(HardwareSerial& port, BMSControl control, bool enable) {
    uint16_t reg;
    switch (control) {
        case BMSControl::Charge:    reg = 0x1070; break;
        case BMSControl::Discharge: reg = 0x1074; break;
        case BMSControl::Balance:   reg = 0x1078; break;
        default: return false;
    }

    uint8_t val = enable ? 1 : 0;

    // Function 0x10 — Write Multiple Registers
    // Data: [reg_hi, reg_lo, qty_hi, qty_lo, byte_count, data...]
    uint8_t request[] = {
        (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF),  // starting address
        0x00, 0x02,                                   // quantity of registers (2)
        0x04,                                         // byte count (4)
        0x00, 0x00, 0x00, val                         // register values
    };

    RS485Result res = modbus_message(port, BMS_ADDRESS, 0x10, request, sizeof(request));

    if (!res.ok()) {
        Serial.printf("[BMS] Toggle 0x%04X %s failed: error %d\n",
                      reg, enable ? "ON" : "OFF", (int)res.error);
        return false;
    }

    Serial.printf("[BMS] 0x%04X %s — OK\n", reg, enable ? "ON" : "OFF");
    return true;
}

// ── Пороги балансировки (мА) ─────────────────────────────────────
// Гистерезис: включаем при < 2000, выключаем при > 2500
static constexpr int32_t BALANCE_ON_CURRENT_MAX  = 2000;   // мА — включить если ток ниже
static constexpr int32_t BALANCE_OFF_CURRENT_MIN = 2500;   // мА — выключить если ток выше

// ── Управление балансировкой ─────────────────────────────────────

void balancingManagement(const BMSData& bms, BalancingState& state,
                         HardwareSerial& port) {
    if (!bms.online) return;

    int32_t current = bms.current;  // мА, отрицательный = разряд
    uint8_t soc     = bms.soc;

    // allowed != false  →  (allowed == Yes || allowed == Unknown)
    // Ток разрядный — выключить балансировку
    if (state.allowed != BalanceAllowed::No && current < 0) {
        if (toggleBmsControl(port, BMSControl::Balance, false)) {
            state.allowed = BalanceAllowed::No;
        }
        return;
    }

    // Зарядный ток слишком большой — выключить балансировку
    if (state.allowed != BalanceAllowed::No && current > BALANCE_OFF_CURRENT_MIN) {
        if (toggleBmsControl(port, BMSControl::Balance, false)) {
            state.allowed = BalanceAllowed::No;
        }
        return;
    }

    // allowed /= true  →  (allowed == No || allowed == Unknown)
    // SOC == 100% и ток маленький — включить балансировку
    if (state.allowed != BalanceAllowed::Yes && current >= 0 && current < BALANCE_ON_CURRENT_MAX && soc == 100) {
        if (toggleBmsControl(port, BMSControl::Balance, true)) {
            state.allowed = BalanceAllowed::Yes;
        }
        return;
    }
}

// ═════════════════════════════════════════════════════════════════
// Internal helpers
// ═════════════════════════════════════════════════════════════════

/// Read big-endian uint16 (standard Modbus register order)
static inline uint16_t readU16BE(const uint8_t* p) {
    return ((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

/// Read big-endian uint32 (two consecutive Modbus registers, high word first)
static inline uint32_t readU32BE(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

/// Read big-endian int32
static inline int32_t readI32BE(const uint8_t* p) {
    return (int32_t)readU32BE(p);
}

/// Parse Modbus register data starting from register 0x1290 (doc offset 144).
///
/// JK-BMS uses byte-level addressing: register address = 0x1200 +
/// doc_byte_offset. But Modbus returns 2 bytes per register, so 32 registers =
/// 64 bytes. Byte offset in response = doc_offset - 144 (start doc offset).
///
///   doc 144 (byte  0) — BatVol, UINT32, mV                     [reg 0x1290]
///   doc 148 (byte  4) — BatWatt, UINT32, mW                    [reg 0x1294]
///   doc 152 (byte  8) — BatCurrent, INT32, mA                  [reg 0x1298]
///   doc 156 (byte 12) — TempBat1, INT16, 0.1℃                  [reg 0x129C]
///   doc 158 (byte 14) — TempBat2, INT16, 0.1℃                  [reg 0x129E]
///   doc 160 (byte 16) — Alarm, UINT32                          [reg 0x12A0]
///   doc 164 (byte 20) — BalanCurrent, INT16, mA                [reg 0x12A4]
///   doc 166 (byte 22) — BalanSta(U8) + SOC(U8) %               [reg 0x12A6]
///   doc 167 → SOC at byte 23
///   doc 168 (byte 24) — SOCCapRemain, INT32, mAH               [reg 0x12A8]
///   doc 172 (byte 28) — SOCFullChargeCap, UINT32, mAH          [reg 0x12AC]
///   doc 192 (byte 48) — ChargeMOS (U8)                         [reg 0x12C0, byte 0]
///   doc 193 (byte 49) — DischargeMOS (U8)                      [reg 0x12C0, byte 1]
static bool parseRegisters(const uint8_t *d, size_t len, BMSData &out) {
    if (len < REG_PAYLOAD_BYTES) {
        Serial.printf("[BMS] Register data too short: %d bytes, expected %d\n",
                      (int)len, (int)REG_PAYLOAD_BYTES);
        return false;
    }

    out.voltage = readU32BE(&d[0]);     // doc 144: BatVol (mV)
    out.power = readU32BE(&d[4]);       // doc 148: BatWatt (mW)
    out.current = readI32BE(&d[8]);     // doc 152: BatCurrent (mA)
    out.soc = d[23];                    // doc 167: SOC (%)
    out.capRemain = readU32BE(&d[24]);  // doc 168: SOCCapRemain (mAH)
    out.capNominal = readU32BE(&d[28]); // doc 172: SOCFullChargeCap (mAH)

    out.chargeMOS = d[48] != 0;         // doc 192: ChargeMOS status
    out.dischargeMOS = d[49] != 0;      // doc 193: DischargeMOS status

    return true;
}
