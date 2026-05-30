#include "inverter.h"
#include "rs485.h"
#include <Preferences.h>

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


// ── Управление зарядом ───────────────────────────────────────────

// Регистр 0x4E9D — Grid max charger current set, единица 0.1A (DC)
static constexpr uint16_t REG_GRID_CHARGE_CURRENT = 0x4E9D;

// Значения по умолчанию берутся из ChargeConfig default member initializers

// NVS namespace и ключи
static constexpr const char* NVS_NAMESPACE   = "charge";
static constexpr const char* NVS_KEY_CURRENT = "maxA";
static constexpr const char* NVS_KEY_MODE    = "mode";

ChargeConfig loadChargeConfig() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);  // read-only

    ChargeConfig cfg;  // default values from struct definition
    cfg.maxCurrentA = prefs.getUChar(NVS_KEY_CURRENT, cfg.maxCurrentA);
    cfg.mode = static_cast<ChargeMode>(prefs.getUChar(NVS_KEY_MODE, (uint8_t)cfg.mode));

    prefs.end();

    // Валидация диапазона
    if (cfg.maxCurrentA < 3)  cfg.maxCurrentA = 3;
    if (cfg.maxCurrentA > 60) cfg.maxCurrentA = 60;
    if (cfg.mode != ChargeMode::Daily && cfg.mode != ChargeMode::Full) {
        cfg.mode = ChargeMode::Daily;
    }

    Serial.printf("[CHG] Config loaded: %uA, mode=%s\n",
                  cfg.maxCurrentA, cfg.mode == ChargeMode::Full ? "full" : "daily");
    return cfg;
}

void saveChargeConfig(const ChargeConfig& cfg) {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);  // read-write
    prefs.putUChar(NVS_KEY_CURRENT, cfg.maxCurrentA);
    prefs.putUChar(NVS_KEY_MODE, (uint8_t)cfg.mode);
    prefs.end();
    Serial.printf("[CHG] Config saved: %uA, mode=%s\n",
                  cfg.maxCurrentA, cfg.mode == ChargeMode::Full ? "full" : "daily");
}

/// Записать ток заряда в регистр 0x4E9D инвертора.
/// @param currentTenths — ток в единицах 0.1A (например, 200 = 20.0A)
/// @return true если запись успешна
static bool writeChargeCurrent(HardwareSerial& port, uint16_t currentTenths) {
    // Function 0x10 — Write Multiple Registers (single register)
    // Data: [reg_hi, reg_lo, qty_hi, qty_lo, byte_count, val_hi, val_lo]
    uint8_t request[] = {
        (uint8_t)(REG_GRID_CHARGE_CURRENT >> 8),
        (uint8_t)(REG_GRID_CHARGE_CURRENT & 0xFF),
        0x00, 0x01,                              // quantity: 1 register
        0x02,                                    // byte count: 2
        (uint8_t)(currentTenths >> 8),
        (uint8_t)(currentTenths & 0xFF),
    };

    RS485Result res = modbus_message(port, INV_ADDRESS, 0x10,
                                     request, sizeof(request));
    if (!res.ok()) {
        Serial.printf("[CHG] Write 0x4E9D failed: error %d\n", (int)res.error);
        return false;
    }
    Serial.printf("[CHG] Charge current set to %u.%uA\n",
                  currentTenths / 10, currentTenths % 10);
    return true;
}

/// Управление зарядным током инвертора (регистр 0x4E9D).
///
/// Режим Full: всегда пользовательский ток — для полного заряда с балансировкой.
///
/// Режим Daily (щадящий, 20%–90%):
///   SOC ≤ 90%  → пользовательский ток (активная зарядка)
///   SOC > 90%  → 2.0A (ток ожидания)
///   SOC > 92%  → 1.0A (минимальный допустимый прошивкой)
///
/// Примечание по токам ожидания:
/// Инвертор не имеет отдельного источника питания — он всегда питается от
/// батареи. Его собственное потребление без нагрузки ~1A и может меняться
/// (например, при включении/отключении кулеров). Поэтому подача минимального
/// зарядного тока от сети или солнца необходима для перекрытия собственного
/// потребления инвертора.
/// Ток ожидания 2.0A сводит к минимуму ток батареи, но он может колебаться
/// как в плюс, так и в минус. Минимальный ток 1.0A будет всегда меньше собственного
/// потребления инвертора, что гарантированно даёт медленный разряд батареи и
/// тем самым исключает нежелательный подзаряд.
void chargeManagement(uint8_t soc, const ChargeConfig& cfg,
                      ChargeState& state, HardwareSerial& port) {

    // Определяем целевой ток (в единицах 0.1A)
    uint16_t targetTenths;

    if (cfg.mode == ChargeMode::Full) {
        // Полная зарядка — всегда пользовательский ток
        targetTenths = (uint16_t)cfg.maxCurrentA * 10;
    } else {
        // Режим Daily: ограничиваем ток при высоком SOC
        if (soc > 92) {
            targetTenths = 10;   // 1.0A — минимальный ток
        } else if (soc > 90) {
            targetTenths = 20;   // 2.0A — ток ожидания
        } else {
            targetTenths = (uint16_t)cfg.maxCurrentA * 10;
        }
    }

    // Записываем только если значение изменилось
    if (targetTenths == state.lastWrittenCurrent) return;

    if (writeChargeCurrent(port, targetTenths)) {
        state.lastWrittenCurrent = targetTenths;
    }
}
