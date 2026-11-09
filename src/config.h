#pragma once
#include <Arduino.h>

/// Стратегия заряда аккумулятора:
///   Daily — ежедневная работа 20%–90%, щадящий режим для долговечности
///   Full  — полный заряд 100% с балансировкой
enum class ChargeStrategy : uint8_t {
    Daily = 0,
    Full  = 1
};

/// Конфигурация приложения (хранится в NVS).
struct Config {
    // ── Заряд ────────────────────────────────────────────────────
    ChargeStrategy strategy    = ChargeStrategy::Daily; // стратегия заряда
    uint8_t        gridCurrent = 26;                    // ток заряда от сети, амперы (3–60)

    // ── Инвертор ─────────────────────────────────────────────────
    bool           inverterOutput = true;               // выход инвертора без сети (регистр 0x4E85)
};

/// Загрузить конфигурацию из NVS (вызвать один раз в setup).
Config loadConfig();

/// Сохранить конфигурацию в NVS.
void saveConfig(const Config& cfg);
