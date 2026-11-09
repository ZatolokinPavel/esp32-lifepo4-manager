#include "config.h"
#include <Preferences.h>

// NVS namespace и ключи.
// Namespace "charge" оставлен прежним, чтобы не потерять уже сохранённые значения.
static constexpr const char* NVS_NAMESPACE      = "charge";
static constexpr const char* NVS_KEY_CURRENT    = "gridCur";
static constexpr const char* NVS_KEY_STRATEGY   = "strategy";
static constexpr const char* NVS_KEY_INV_OUTPUT = "invOutput";

Config loadConfig() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);  // read-only

    Config cfg;  // default values from struct definition
    cfg.gridCurrent = prefs.getUChar(NVS_KEY_CURRENT, cfg.gridCurrent);
    cfg.strategy = static_cast<ChargeStrategy>(prefs.getUChar(NVS_KEY_STRATEGY, (uint8_t)cfg.strategy));
    cfg.inverterOutput = prefs.getBool(NVS_KEY_INV_OUTPUT, cfg.inverterOutput);

    prefs.end();

    // Валидация диапазона
    if (cfg.gridCurrent < 3)  cfg.gridCurrent = 3;
    if (cfg.gridCurrent > 60) cfg.gridCurrent = 60;
    if (cfg.strategy != ChargeStrategy::Daily && cfg.strategy != ChargeStrategy::Full) {
        cfg.strategy = ChargeStrategy::Daily;
    }

    Serial.printf("[CFG] Loaded: %uA, strategy=%s, inv_output=%s\n",
                  cfg.gridCurrent, cfg.strategy == ChargeStrategy::Full ? "full" : "daily",
                  cfg.inverterOutput ? "on" : "off");
    return cfg;
}

void saveConfig(const Config& cfg) {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);  // read-write
    prefs.putUChar(NVS_KEY_CURRENT, cfg.gridCurrent);
    prefs.putUChar(NVS_KEY_STRATEGY, (uint8_t)cfg.strategy);
    prefs.putBool(NVS_KEY_INV_OUTPUT, cfg.inverterOutput);
    prefs.end();
    Serial.printf("[CFG] Saved: %uA, strategy=%s, inv_output=%s\n",
                  cfg.gridCurrent, cfg.strategy == ChargeStrategy::Full ? "full" : "daily",
                  cfg.inverterOutput ? "on" : "off");
}
