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

// ── Управление зарядом ───────────────────────────────────────────

/// Режим заряда аккумулятора:
///   Daily — ежедневная работа 20%–90%, щадящий режим для долговечности
///   Full  — полный заряд 100% с балансировкой
enum class ChargeMode : uint8_t {
    Daily = 0,
    Full  = 1
};

/// Конфигурация заряда (хранится в NVS).
struct ChargeConfig {
    uint8_t    maxCurrentA = 20;              // желаемый ток заряда, целые амперы (3–60)
    ChargeMode mode        = ChargeMode::Daily; // режим работы
};

/// Состояние управления зарядом (runtime, не сохраняется).
struct ChargeState {
    uint16_t lastWrittenCurrent = 0;  // последний записанный ток (0.1A), 0 = ещё не писали
};

/// Загрузить конфигурацию заряда из NVS (вызвать один раз в setup).
ChargeConfig loadChargeConfig();

/// Сохранить конфигурацию заряда в NVS.
void saveChargeConfig(const ChargeConfig& cfg);

/// Управление зарядным током инвертора.
/// Вызывается после каждого успешного чтения данных инвертора.
/// Записывает регистр 0x4E9D (Grid max charger current) при необходимости.
/// @param soc    — текущий SOC батареи (%)
/// @param cfg    — конфигурация заряда
/// @param state  — runtime-состояние (обновляется на месте)
/// @param port   — UART порт инвертора
void chargeManagement(uint8_t soc, const ChargeConfig& cfg,
                      ChargeState& state, HardwareSerial& port);
