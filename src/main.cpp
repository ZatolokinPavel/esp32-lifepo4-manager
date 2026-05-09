#include <Arduino.h>
#include <ETH.h>
#include <Network.h>
#include <WebServer.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <esp_bt.h>
#include "bms.h"

// ── Pin Definitions ──────────────────────────────────────────────
// OTA protection switch on GPIO35 (input-only pin).
// External 10k pull-up resistor to 3.3V required.
// Switch connects GPIO35 to GND: closed (ON) = LOW = OTA allowed.
// Switch open (OFF) = HIGH = OTA blocked.
static constexpr uint8_t PIN_OTA_PROTECT = 35;

// RS485 UART — BMS (JK-B2A8S20P) on UART1
static constexpr uint8_t PIN_BMS_TX  = 17;  // TXD on board
static constexpr uint8_t PIN_BMS_RX  = 5;   // RXD on board

// RS485 UART — Inverter (MUST PV18-3224) on UART2
static constexpr uint8_t PIN_INV_TX  = 33;
static constexpr uint8_t PIN_INV_RX  = 32;

// ── Shared data (аналог ets) ─────────────────────────────────────
struct DeviceData {
    // BMS
    BMSData  bms;

    // Inverter
    float    invVoltageIn;
    float    invVoltageOut;
    float    invLoadPercent;
    bool     invOnline;
    uint32_t lastInvUpdate;
};

static DeviceData   deviceData = {};
static SemaphoreHandle_t dataMutex;

// ── UART ports for RS485 devices ─────────────────────────────────
HardwareSerial SerialBMS(1);   // UART1
HardwareSerial SerialINV(2);   // UART2

// ── Ethernet ─────────────────────────────────────────────────────
static bool ethConnected = false;

void onEthEvent(arduino_event_id_t event) {
    switch (event) {
        case ARDUINO_EVENT_ETH_START:
            Serial.println("[ETH] Started");
            ETH.setHostname("esp32-energy");
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            Serial.println("[ETH] Link up");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            Serial.printf("[ETH] IP: %s\n", ETH.localIP().toString().c_str());
            Serial.printf("[ETH] Speed: %d Mbps, Full-duplex: %s\n",
                          ETH.linkSpeed(), ETH.fullDuplex() ? "yes" : "no");
            ethConnected = true;
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            Serial.println("[ETH] Link down");
            ethConnected = false;
            break;
        case ARDUINO_EVENT_ETH_STOP:
            Serial.println("[ETH] Stopped");
            ethConnected = false;
            break;
        default:
            break;
    }
}

// ── HTTP Handlers ────────────────────────────────────────────────
WebServer server(80);

void handleStatus() {
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    DeviceData snapshot = deviceData;
    xSemaphoreGive(dataMutex);

    const BMSData& b = snapshot.bms;

    char json[512];
    snprintf(json, sizeof(json),
        "{\"bms\":{\"online\":%s,\"soc\":%u,\"voltage\":%u,\"current\":%d,\"power\":%u,"
        "\"cap_remain\":%u,\"cap_nominal\":%u,"
        "\"charge_mos\":%s,\"discharge_mos\":%s},"
        "\"inverter\":{\"online\":%s,\"voltage_in\":%.1f,\"voltage_out\":%.1f,\"load\":%.1f}}",
        b.online ? "true" : "false",
        b.soc, b.voltage, b.current, b.power,
        b.capRemain, b.capNominal,
        b.chargeMOS ? "true" : "false",
        b.dischargeMOS ? "true" : "false",
        snapshot.invOnline ? "true" : "false",
        snapshot.invVoltageIn, snapshot.invVoltageOut, snapshot.invLoadPercent);

    server.send(200, "application/json", json);
}

void handleMode() {
    // TODO: parse body and change inverter mode
    server.send(200, "application/json", "{\"status\":\"not_implemented\"}");
}

void handleSystem() {
    char json[128];
    snprintf(json, sizeof(json),
        "{\"uptime_sec\":%lu,\"free_heap\":%u}",
        millis() / 1000, ESP.getFreeHeap());
    server.send(200, "application/json", json);
}

void handleReboot() {
    // TODO: implement with rate-limiting / auth protection
    server.send(200, "application/json", "{\"status\":\"not_implemented\"}");
}

void handleNotFound() {
    server.send(404, "application/json", "{\"error\":\"not found\"}");
}

// ── OTA ──────────────────────────────────────────────────────────
static bool otaRunning = false;

bool isOtaAllowed() {
    return digitalRead(PIN_OTA_PROTECT) == LOW;  // Switch ON (closed to GND) = allowed
}

void startOTA() {
    if (otaRunning) return;
    ArduinoOTA.setHostname("esp32-energy");
    ArduinoOTA.onStart([]() { Serial.println("[OTA] Update starting..."); });
    ArduinoOTA.onEnd([]()   { Serial.println("\n[OTA] Update complete!"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[OTA] Progress: %u%%\r", (progress * 100) / total);
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]\n", error);
    });
    ArduinoOTA.begin();
    otaRunning = true;
    Serial.println("[OTA] Enabled (switch ON)");
}

void stopOTA() {
    if (!otaRunning) return;
    ArduinoOTA.end();
    otaRunning = false;
    Serial.println("[OTA] Disabled (switch OFF)");
}

// ═════════════════════════════════════════════════════════════════
// Task 1: RS485 polling (runs on core 0)
// ═════════════════════════════════════════════════════════════════

void pollBMS() {
    BMSData bms = readBmsStatus(SerialBMS);

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    deviceData.bms = bms;
    xSemaphoreGive(dataMutex);

    if (bms.online) {
        Serial.printf("[BMS] SOC=%u%% I=%dmA P=%umW\n", bms.soc, bms.current, bms.power);
    }
}

void pollInverter() {
    // TODO: send actual Modbus request and parse response
    // For now — stub with dummy data
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    deviceData.invVoltageIn  = 230.5;
    deviceData.invVoltageOut = 220.1;
    deviceData.invLoadPercent = 42.0;
    deviceData.invOnline     = false;
    deviceData.lastInvUpdate = millis();
    xSemaphoreGive(dataMutex);
}

void rs485Task(void* param) {
    Serial.println("[RS485] Task started on core " + String(xPortGetCoreID()));
    for (;;) {
        pollBMS();
        delay(100);       // small gap between devices
        pollInverter();
        delay(1000);      // poll cycle ~1 second
    }
}

// ═════════════════════════════════════════════════════════════════
// Task 2: HTTP + OTA (runs on core 1, same as Arduino loop)
// ═════════════════════════════════════════════════════════════════

void setup() {
    Serial.begin(115200);
    Serial.println("\n[BOOT] ESP32 Energy Controller starting...");

    // Disable WiFi & Bluetooth completely
    WiFi.mode(WIFI_OFF);
    esp_bt_controller_disable();
    Serial.println("[BOOT] WiFi & Bluetooth disabled");

    // OTA protection pin (external 10k pull-up, no internal pull available on GPIO35)
    pinMode(PIN_OTA_PROTECT, INPUT);

    // RS485 UARTs
    SerialBMS.begin(9600, SERIAL_8N1, PIN_BMS_RX, PIN_BMS_TX);
    Serial.println("[UART1] BMS ready (GPIO5 RX, GPIO17 TX)");
    SerialINV.begin(9600, SERIAL_8N1, PIN_INV_RX, PIN_INV_TX);
    Serial.println("[UART2] Inverter ready (GPIO32 RX, GPIO33 TX)");

    // Shared data mutex
    dataMutex = xSemaphoreCreateMutex();

    // Ethernet
    Network.onEvent(onEthEvent);
    ETH.begin();

    // HTTP server
    server.on("/api/v1/status", HTTP_GET, handleStatus);
    server.on("/api/v1/mode",   HTTP_POST, handleMode);
    server.on("/api/v1/system", HTTP_GET, handleSystem);
    server.on("/api/v1/reboot", HTTP_POST, handleReboot);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("[HTTP] Server started on port 80");

    // OTA — start only if switch allows
    if (isOtaAllowed()) {
        startOTA();
    } else {
        Serial.println("[OTA] Disabled (switch OFF)");
    }

    // Start RS485 polling task on core 0 (network runs on core 1)
    xTaskCreatePinnedToCore(rs485Task, "rs485", 8192, NULL, 1, NULL, 0);
}

// ── Loop ─────────────────────────────────────────────────────────
void loop() {
    server.handleClient();

    // Dynamically enable/disable OTA based on switch position
    if (isOtaAllowed()) {
        if (!otaRunning) startOTA();
        ArduinoOTA.handle();
    } else {
        if (otaRunning) stopOTA();
    }
}
