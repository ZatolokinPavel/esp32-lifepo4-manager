#include "inverter.h"
#include "rs485.h"

// MUST PV18-3224 Modbus address (per protocol doc: Inverter ModbusRTU ID == 4)
static constexpr uint8_t INV_ADDRESS = 4;

// "Inverter Display Message" block: start register and count
// Request frame: 04 03 62 71 00 4F  4B C8
static constexpr uint16_t REG_DISPLAY_START = 0x6271;
static constexpr uint16_t REG_DISPLAY_COUNT = 0x4F;  // 79 registers

// ── Public API ───────────────────────────────────────────────────

InverterData readInverterStatus(HardwareSerial& port) {
    InverterData data = {};
    data.online = false;

    // Modbus function 0x03 — Read Holding Registers
    // Data: [start_reg_hi, start_reg_lo, count_hi, count_lo]
    uint8_t request[] = {
        (uint8_t)(REG_DISPLAY_START >> 8),
        (uint8_t)(REG_DISPLAY_START & 0xFF),
        (uint8_t)(REG_DISPLAY_COUNT >> 8),
        (uint8_t)(REG_DISPLAY_COUNT & 0xFF),
    };

    RS485Result res = modbus_message(port, INV_ADDRESS, 0x03,
                                     request, sizeof(request));

    if (!res.ok()) {
        Serial.printf("[INV] Error: %d\n", (int)res.error);
        return data;
    }

    // Response payload: [byte_count, reg0_hi, reg0_lo, reg1_hi, reg1_lo, ...]
    // Expected byte_count = 79 * 2 = 158, total payload = 1 + 158 = 159 bytes
    uint8_t expectedBytes = REG_DISPLAY_COUNT * 2;
    if (res.dataLen < 1 + expectedBytes) {
        Serial.printf("[INV] Short response: %d bytes (expected %d)\n",
                      (int)res.dataLen, 1 + expectedBytes);
        return data;
    }

    // Helper: extract register value at given offset (0-based from block start)
    // Data layout: res.data[0] = byte_count, then pairs [hi, lo] for each register
    auto reg = [&](uint8_t offset) -> uint16_t {
        uint16_t idx = 1 + offset * 2;
        return ((uint16_t)res.data[idx] << 8) | res.data[idx + 1];
    };

    data.gridVoltage = reg(6);   // 0x6277 Grid voltage (0.1V)
    data.pLoad       = reg(14);  // 0x627F PLoad (1W)
    data.sLoad       = reg(18);  // 0x6283 Sload (1VA)
    data.loadPercent = reg(15);  // 0x6280 Load percent (1%)

    data.online = true;
    data.timestamp = millis();
    return data;
}

// ── Зарядные токи ────────────────────────────────────────────────

// Max utility (grid) charging current, единица 0.1A
static constexpr uint16_t REG_GRID_CHARGE_CURRENT  = 0x4E9D;
// Max solar charging current, единица 0.1A (в протоколе ошибочно "Max Combine charger current")
static constexpr uint16_t REG_SOLAR_CHARGE_CURRENT = 0x4EA4;

/// Записать один holding-регистр инвертора (функция 0x10).
static bool writeRegister(HardwareSerial& port, uint16_t reg, uint16_t value) {
    // Data: [reg_hi, reg_lo, qty_hi, qty_lo, byte_count, val_hi, val_lo]
    uint8_t request[] = {
        (uint8_t)(reg >> 8),
        (uint8_t)(reg & 0xFF),
        0x00, 0x01,                // quantity: 1 register
        0x02,                      // byte count: 2
        (uint8_t)(value >> 8),
        (uint8_t)(value & 0xFF),
    };

    RS485Result res = modbus_message(port, INV_ADDRESS, 0x10,
                                     request, sizeof(request));
    if (!res.ok()) {
        Serial.printf("[INV] Write 0x%04X=%u failed: error %d\n", reg, value, (int)res.error);
        return false;
    }
    Serial.printf("[INV] Write 0x%04X=%u OK\n", reg, value);
    return true;
}

bool writeGridChargeCurrent(HardwareSerial& port, uint16_t currentTenths) {
    return writeRegister(port, REG_GRID_CHARGE_CURRENT, currentTenths);
}

bool writeSolarChargeCurrent(HardwareSerial& port, uint16_t currentTenths) {
    return writeRegister(port, REG_SOLAR_CHARGE_CURRENT, currentTenths);
}
