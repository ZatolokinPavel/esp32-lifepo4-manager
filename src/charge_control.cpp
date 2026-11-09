#include "charge_control.h"
#include "inverter.h"

// ── Пороги ───────────────────────────────────────────────────────

// Гистерезис по SOC: удерживаем при SOC > 90, отпускаем при SOC < 90.
// Ровно 90% — ничего не меняем.
static constexpr uint8_t SOC_HOLD = 90;

// Лимиты тока инвертора (0.1 A)
static constexpr uint16_t CURRENT_LIMITING = 10;   // 1 A — минимум инвертора
static constexpr uint16_t CURRENT_HOLDING  = 50;   // 5 A — с запасом на собственное потребление инвертора
static constexpr uint16_t CURRENT_SOLAR_MAX = 600; // 60 A — ток ограничен мощностью самих панелей

// Закрытие мосфетов в фазе Limiting:
//   первые 2 минуты после записи лимитов ждём ток батареи <= 0;
//   после таймаута соглашаемся на ток <= 2 A.
// Ток с 60 A до 0 спадает примерно за 60 с.
static constexpr uint32_t ZERO_CURRENT_TIMEOUT_MS = 2 * 60 * 1000;
static constexpr int32_t  CLOSE_CURRENT_BEFORE_TIMEOUT = 0;     // мА
static constexpr int32_t  CLOSE_CURRENT_AFTER_TIMEOUT  = 2000;  // мА

// Сколько чтений подряд ток должен держаться ниже порога (защита от случайного значения)
static constexpr uint8_t LOW_CURRENT_READS = 3;

// ── Helpers ──────────────────────────────────────────────────────

const char* chargePhaseName(ChargePhase phase) {
    switch (phase) {
        case ChargePhase::Charging: return "charging";
        case ChargePhase::Limiting: return "limiting";
        case ChargePhase::Holding:  return "holding";
        default:                    return "unknown";
    }
}

static void enterPhase(ChargeControlState& state, ChargePhase phase) {
    Serial.printf("[CHG] Phase %s -> %s\n", chargePhaseName(state.phase), chargePhaseName(phase));
    state.phase           = phase;
    state.limitingSince   = millis();
    state.lowCurrentReads = 0;
    state.timeoutWarned   = false;
}

/// Целевой лимит зарядного тока от сети для фазы (0.1 A). 0 = не трогать регистр.
static uint16_t targetGridCurrent(const Config& cfg, ChargePhase phase) {
    switch (phase) {
        case ChargePhase::Charging: return (uint16_t)cfg.gridCurrent * 10;
        case ChargePhase::Limiting: return CURRENT_LIMITING;
        case ChargePhase::Holding:  return CURRENT_HOLDING;
        default:                    return 0;
    }
}

/// Целевой лимит зарядного тока от солнца для фазы (0.1 A). 0 = не трогать регистр.
static uint16_t targetSolarCurrent(ChargePhase phase) {
    switch (phase) {
        case ChargePhase::Charging: return CURRENT_SOLAR_MAX;
        case ChargePhase::Limiting: return CURRENT_LIMITING;
        case ChargePhase::Holding:  return CURRENT_HOLDING;
        default:                    return 0;
    }
}

/// Считает чтения подряд, в которых ток батареи не выше порога.
/// Порог зависит от того, истёк ли таймаут ожидания нулевого тока.
static void countLowCurrentReads(const BMSData& bms, ChargeControlState& state) {
    bool    timeoutPassed = millis() - state.limitingSince >= ZERO_CURRENT_TIMEOUT_MS;
    int32_t maxCurrent    = timeoutPassed ? CLOSE_CURRENT_AFTER_TIMEOUT : CLOSE_CURRENT_BEFORE_TIMEOUT;

    if (bms.current > maxCurrent) {
        state.lowCurrentReads = 0;
    } else if (state.lowCurrentReads < LOW_CURRENT_READS) {
        state.lowCurrentReads++;
    }

    if (timeoutPassed && bms.current > maxCurrent && !state.timeoutWarned) {
        Serial.printf("[CHG] Warning: current still %dmA after timeout, MOSFETs stay open\n", bms.current);
        state.timeoutWarned = true;
    }
}

// ── Переходы между фазами ────────────────────────────────────────

static void updatePhase(const BMSData& bms, const Config& cfg,
                        ChargeControlState& state, HardwareSerial& bmsPort) {
    uint8_t soc     = bms.soc;
    bool    isDaily = cfg.strategy == ChargeStrategy::Daily;
    bool    isFull  = cfg.strategy == ChargeStrategy::Full;

    // Старт: мосфеты уже закрыты, Daily и SOC не ниже 90 — продолжаем удерживать
    if (state.phase == ChargePhase::Unknown && isDaily && soc >= SOC_HOLD && !bms.chargeMOS) {
        enterPhase(state, ChargePhase::Holding);
        return;
    }

    // Старт: в остальных случаях — обычная зарядка, мосфеты открыть
    if (state.phase == ChargePhase::Unknown) {
        if (toggleBmsControl(bmsPort, BMSControl::Charge, true)) {
            enterPhase(state, ChargePhase::Charging);
        }
        return;
    }

    // Daily и SOC > 90 — начинаем снижать ток
    if (state.phase == ChargePhase::Charging && isDaily && soc > SOC_HOLD) {
        enterPhase(state, ChargePhase::Limiting);
        return;
    }

    // Отмена ограничения: Full или SOC < 90 (мосфеты ещё открыты)
    if (state.phase == ChargePhase::Limiting && (isFull || soc < SOC_HOLD)) {
        enterPhase(state, ChargePhase::Charging);
        return;
    }

    // Лимиты 1 A ещё не записаны в инвертор — таймаут не запускаем
    if (state.phase == ChargePhase::Limiting
        && (state.gridWritten != CURRENT_LIMITING || state.solarWritten != CURRENT_LIMITING)) {
        state.limitingSince   = millis();
        state.lowCurrentReads = 0;
        return;
    }

    if (state.phase == ChargePhase::Limiting) {
        countLowCurrentReads(bms, state);
    }

    // Ток упал — закрыть зарядные мосфеты
    if (state.phase == ChargePhase::Limiting && state.lowCurrentReads >= LOW_CURRENT_READS) {
        if (toggleBmsControl(bmsPort, BMSControl::Charge, false)) {
            enterPhase(state, ChargePhase::Holding);
        }
        return;
    }

    // Full или SOC < 90 — открыть мосфеты, вернуть зарядку
    if (state.phase == ChargePhase::Holding && (isFull || soc < SOC_HOLD)) {
        if (toggleBmsControl(bmsPort, BMSControl::Charge, true)) {
            enterPhase(state, ChargePhase::Charging);
        }
        return;
    }
}

// ── Синхронизация лимитов инвертора ──────────────────────────────

/// Порядок записи важен: сначала солнце (0x4EA4), потом сеть (0x4E9D).
///
/// При обратном порядке инвертор подтверждал обе записи, но лимит сети иногда
/// молча оставался прежним. Наблюдалось в Limiting: сеть осталась 25 A, солнце
/// стало 1 A, заряд продолжился до 97%. Пауза ~1 с между записями не помогла,
/// перестановка порядка помогла. Причина неизвестна.
static void syncCurrents(const Config& cfg, ChargeControlState& state, HardwareSerial& invPort) {
    uint16_t grid  = targetGridCurrent(cfg, state.phase);
    uint16_t solar = targetSolarCurrent(state.phase);

    if (solar != 0 && solar != state.solarWritten && writeSolarChargeCurrent(invPort, solar)) {
        state.solarWritten = solar;
    }

    if (grid != 0 && grid != state.gridWritten && writeGridChargeCurrent(invPort, grid)) {
        state.gridWritten = grid;
    }
}

// ── Public API ───────────────────────────────────────────────────

void forgetWrittenCurrents(ChargeControlState& state) {
    state.gridWritten  = 0;
    state.solarWritten = 0;
}

void chargeControl(const BMSData& bms, const Config& cfg, ChargeControlState& state,
                   HardwareSerial& bmsPort, HardwareSerial& invPort) {
    updatePhase(bms, cfg, state, bmsPort);
    syncCurrents(cfg, state, invPort);
}
