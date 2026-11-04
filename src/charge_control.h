#pragma once
#include <Arduino.h>
#include "bms.h"
#include "config.h"

// ── Совместное управление зарядом: инвертор + BMS ───────────────
//
// В инверторе нет команды «стоп заряд», есть только лимиты тока (минимум 1 A).
// Резко закрывать зарядные мосфеты BMS на большом токе нельзя: импульсное
// перенапряжение может повредить инвертор. Поэтому в Daily при SOC > 90%:
//   1. снижаем оба лимита (сеть и солнце) до 1 A;
//   2. ждём, пока ток батареи упадёт (инвертор снижает ток плавно, до ~60 с);
//   3. закрываем зарядные мосфеты BMS;
//   4. поднимаем оба лимита до 5 A, чтобы инвертор питал себя от сети/солнца,
//      а не от батареи (ток в батарею не пойдёт — мосфеты закрыты).
// В Full или при SOC < 90% мосфеты открываются и лимиты возвращаются к номиналу.
//
// Фазы и целевые лимиты (0.1 A):
//   Phase      | 0x4E9D (сеть)       | 0x4EA4 (солнце) | зарядные мосфеты
//   Charging   | cfg.gridCurrent*10  | 600             | вкл
//   Limiting   | 10                  | 10              | вкл, ждём падения тока
//   Holding    | 50                  | 50              | выкл
//
// Переходы меняют только фазу и мосфеты. Регистры инвертора приводятся
// к целевым значениям фазы отдельным шагом на каждой итерации.

enum class ChargePhase : uint8_t {
    Unknown,   // после старта: состояние инвертора и BMS ещё не согласовано
    Charging,  // обычная зарядка
    Limiting,  // лимиты 1 A, ждём падения тока перед закрытием мосфетов
    Holding    // мосфеты закрыты, SOC удерживается
};

/// Runtime-состояние (не сохраняется).
struct ChargeControlState {
    ChargePhase phase           = ChargePhase::Unknown;
    uint32_t    limitingSince   = 0;  // millis(), когда лимиты 1 A были записаны в инвертор
    uint8_t     lowCurrentReads = 0;  // сколько чтений подряд ток ниже порога
    bool        timeoutWarned   = false;
    uint16_t    gridWritten     = 0;  // последний записанный лимит сети (0.1A), 0 = неизвестно
    uint16_t    solarWritten    = 0;  // последний записанный лимит солнца (0.1A), 0 = неизвестно
};

/// Имя фазы для логов и JSON: "unknown", "charging", "limiting", "holding".
const char* chargePhaseName(ChargePhase phase);

/// Вызывается раз за цикл опроса, когда BMS и инвертор онлайн.
/// @param bms     — свежие данные BMS
/// @param cfg     — конфигурация
/// @param state   — runtime-состояние (обновляется на месте)
/// @param bmsPort — UART порт BMS
/// @param invPort — UART порт инвертора
void chargeControl(const BMSData& bms, const Config& cfg, ChargeControlState& state,
                   HardwareSerial& bmsPort, HardwareSerial& invPort);
