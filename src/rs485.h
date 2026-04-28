#pragma once
#include <Arduino.h>

// ── RS485 result ─────────────────────────────────────────────────
enum class RS485Error : uint8_t {
    OK = 0,
    NO_RESPONSE,
    CRC_MISMATCH,
    WRONG_ADDRESS,
    MODBUS_EXCEPTION,
    BUFFER_OVERFLOW,
};

struct RS485Result {
    RS485Error error;
    uint8_t    modbusException;  // error code from device (if MODBUS_EXCEPTION)
    uint8_t    data[512];        // response payload (without address, function, crc)
    size_t     dataLen;
    // For JK-BMS: optional 300-byte data packet that precedes the Modbus frame
    uint8_t    dataPacket[300];
    bool       hasDataPacket;

    bool ok() const { return error == RS485Error::OK; }
};

// ── Send & receive ───────────────────────────────────────────────

/// Build Modbus RTU frame and send it, then read and validate response.
RS485Result modbus_message(HardwareSerial& port, uint8_t address,
                           uint8_t function, const uint8_t* data,
                           size_t dataLen, unsigned long timeoutMs = 500);

/// Same as modbus_message but handles JK-BMS quirk:
/// response may start with a 300-byte data packet before the Modbus frame.
RS485Result jk_bms_message(HardwareSerial& port, uint8_t address,
                           uint8_t function, const uint8_t* data,
                           size_t dataLen, unsigned long timeoutMs = 500);
