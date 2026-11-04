#include "config.h"
#include <Preferences.h>

// NVS namespace и ключи.
// Namespace "charge" оставлен прежним, чтобы не потерять уже сохранённые значения.
static constexpr const char* NVS_NAMESPACE    = "charge";
static constexpr const char* NVS_KEY_CURRENT  = "gridCur";
static constexpr const char* NVS_KEY_STRATEGY = "strategy";

Config loadConfig() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);  // read-only

    Config cfg;  // default values from struct definition
    cfg.gridCurrent = prefs.getUChar(NVS_KEY_CURRENT, cfg.gridCurrent);
    cfg.strategy = static_cast<ChargeStrategy>(prefs.getUChar(NVS_KEY_STRATEGY, (uint8_t)cfg.strategy));

    prefs.end();

    // Валидация диапазона
    if (cfg.gridCurrent < 3)  cfg.gridCurrent = 3;
    if (cfg.gridCurrent > 60) cfg.gridCurrent = 60;
    if (cfg.strategy != ChargeStrategy::Daily && cfg.strategy != ChargeStrategy::Full) {
        cfg.strategy = ChargeStrategy::Daily;
    }

    Serial.printf("[CFG] Loaded: %uA, strategy=%s\n",
                  cfg.gridCurrent, cfg.strategy == ChargeStrategy::Full ? "full" : "daily");
    return cfg;
}

void saveConfig(const Config& cfg) {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);  // read-write
    prefs.putUChar(NVS_KEY_CURRENT, cfg.gridCurrent);
    prefs.putUChar(NVS_KEY_STRATEGY, (uint8_t)cfg.strategy);
    prefs.end();
    Serial.printf("[CFG] Saved: %uA, strategy=%s\n",
                  cfg.gridCurrent, cfg.strategy == ChargeStrategy::Full ? "full" : "daily");
}
