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

// ── Зарядные токи ────────────────────────────────────────────────
// Оба регистра: единица 0.1A, диапазон 10–600 (1–60 A).

/// Записать максимальный ток заряда от сети (регистр 0x4E9D).
/// @param currentTenths — ток в единицах 0.1A (например, 200 = 20.0A)
/// @return true если запись успешна
bool writeGridChargeCurrent(HardwareSerial& port, uint16_t currentTenths);

/// Записать максимальный ток заряда от солнца (регистр 0x4EA4).
/// В протоколе описан как "Max Combine charger current", но фактически
/// (по бумажной инструкции и проверке) это "Max solar charging current".
/// @param currentTenths — ток в единицах 0.1A
/// @return true если запись успешна
bool writeSolarChargeCurrent(HardwareSerial& port, uint16_t currentTenths);

// ── Выход инвертора ──────────────────────────────────────────────

/// Включить/выключить выход инвертора при отсутствии сети
/// (регистр 0x4E85 "Inverter offgrid work enable": 0 — OFF, 1 — ON).
/// @return true если запись успешна
bool writeInverterOutput(HardwareSerial& port, bool enabled);
