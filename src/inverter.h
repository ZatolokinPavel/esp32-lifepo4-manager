#pragma once
#include <Arduino.h>

/// Data read from MUST PV18-3224 inverter via Modbus RTU.
struct InverterData {
    bool     online;
    uint32_t timestamp;          // millis() of last successful read

    // ── Inverter Display Message fields ──────────────────────────
    uint16_t gridVoltage;        // 0x6277, unit 0.1V (e.g. 2300 = 230.0V)
    uint16_t pLoad;              // 0x627F, unit 1W
    uint16_t sLoad;              // 0x6283, unit 1VA
    uint16_t loadPercent;        // 0x6280, unit 1%
};

/// Poll inverter over RS485 and return parsed data.
/// Uses modbus_message internally (address=4, function=0x03 Read Holding Registers).
InverterData readInverterStatus(HardwareSerial& port);
