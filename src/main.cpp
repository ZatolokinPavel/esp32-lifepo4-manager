#include <Arduino.h>
#include <ETH.h>
#include <WebServer.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

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

// ── Globals ──────────────────────────────────────────────────────
WebServer server(80);
static bool ethConnected = false;

// UART for RS485 devices (baud rates are placeholders — will set actual values with protocols)
HardwareSerial SerialBMS(1);   // UART1
HardwareSerial SerialINV(2);   // UART2

// ── Ethernet event handler ───────────────────────────────────────
void onEthEvent(WiFiEvent_t event) {
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
void handleTest() {
    server.send(200, "application/json", "{\"status\":\"ok\",\"device\":\"esp32-energy\"}");
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

    ArduinoOTA.onStart([]() {
        Serial.println("[OTA] Update starting...");
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] Update complete!");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[OTA] Progress: %u%%\r", (progress * 100) / total);
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]: ", error);
        switch (error) {
            case OTA_AUTH_ERROR:    Serial.println("Auth Failed");    break;
            case OTA_BEGIN_ERROR:   Serial.println("Begin Failed");   break;
            case OTA_CONNECT_ERROR: Serial.println("Connect Failed"); break;
            case OTA_RECEIVE_ERROR: Serial.println("Receive Failed"); break;
            case OTA_END_ERROR:     Serial.println("End Failed");     break;
        }
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


// ── Setup ────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial.println("\n[BOOT] ESP32 Energy Controller starting...");

    // Disable WiFi & Bluetooth completely
    WiFi.mode(WIFI_OFF);
    btStop();
    Serial.println("[BOOT] WiFi & Bluetooth disabled");

    // OTA protection pin (external 10k pull-up, no internal pull available on GPIO35)
    pinMode(PIN_OTA_PROTECT, INPUT);

    // RS485 UARTs (baud rates TBD — using 9600 as safe default)
    SerialBMS.begin(9600, SERIAL_8N1, PIN_BMS_RX, PIN_BMS_TX);
    Serial.println("[UART1] BMS ready (GPIO5 RX, GPIO17 TX)");

    SerialINV.begin(9600, SERIAL_8N1, PIN_INV_RX, PIN_INV_TX);
    Serial.println("[UART2] Inverter ready (GPIO32 RX, GPIO33 TX)");

    // Ethernet
    WiFi.onEvent(onEthEvent);
    ETH.begin();

    // HTTP server
    server.on("/test", HTTP_GET, handleTest);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("[HTTP] Server started on port 80");

    // OTA — start only if switch allows
    if (isOtaAllowed()) {
        startOTA();
    } else {
        Serial.println("[OTA] Disabled (switch OFF)");
    }
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
