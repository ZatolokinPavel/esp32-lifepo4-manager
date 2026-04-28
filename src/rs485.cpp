#include "rs485.h"
#include "crc16.h"

// Forward declarations
static size_t readResponse(HardwareSerial& port, uint8_t* buf,
                           size_t maxLen, unsigned long timeoutMs);
static RS485Result parseModbusResponse(const uint8_t* rxBuf, size_t rxLen,
                                       uint8_t address, uint8_t function);

// ── modbus_message ───────────────────────────────────────────────

RS485Result modbus_message(HardwareSerial& port, uint8_t address,
                           uint8_t function, const uint8_t* data,
                           size_t dataLen, unsigned long timeoutMs) {
    // Build ADU: [address, function, data..., crc_lo, crc_hi]
    uint8_t frame[256];
    size_t frameLen = 0;
    frame[frameLen++] = address;
    frame[frameLen++] = function;
    memcpy(&frame[frameLen], data, dataLen);
    frameLen += dataLen;

    uint16_t crc = crc16_modbus(frame, frameLen);
    frame[frameLen++] = crc & 0xFF;
    frame[frameLen++] = (crc >> 8) & 0xFF;

    // Send
    port.write(frame, frameLen);
    port.flush();

    // Receive
    uint8_t rxBuf[512];
    size_t rxLen = readResponse(port, rxBuf, sizeof(rxBuf), timeoutMs);

    if (rxLen == 0) {
        RS485Result result = {};
        result.error = RS485Error::NO_RESPONSE;
        return result;
    }

    return parseModbusResponse(rxBuf, rxLen, address, function);
}

// ── jk_bms_message ───────────────────────────────────────────────

RS485Result jk_bms_message(HardwareSerial& port, uint8_t address,
                           uint8_t function, const uint8_t* data,
                           size_t dataLen, unsigned long timeoutMs) {
    // Build ADU
    uint8_t frame[256];
    size_t frameLen = 0;
    frame[frameLen++] = address;
    frame[frameLen++] = function;
    memcpy(&frame[frameLen], data, dataLen);
    frameLen += dataLen;

    uint16_t crc = crc16_modbus(frame, frameLen);
    frame[frameLen++] = crc & 0xFF;
    frame[frameLen++] = (crc >> 8) & 0xFF;

    // Send
    port.write(frame, frameLen);
    port.flush();

    // Receive — JK-BMS may send up to 300 + normal Modbus frame
    uint8_t rxBuf[600];
    size_t rxLen = readResponse(port, rxBuf, sizeof(rxBuf), timeoutMs);

    if (rxLen == 0) {
        RS485Result result = {};
        result.error = RS485Error::NO_RESPONSE;
        return result;
    }

    // If response is large enough, first 300 bytes are a data packet
    size_t offset = 0;
    RS485Result result = {};
    if (rxLen > 300 && rxBuf[300] == address && rxBuf[301] == function) {
        memcpy(result.dataPacket, rxBuf, 300);
        result.hasDataPacket = true;
        offset = 300;
    }

    RS485Result parsed = parseModbusResponse(&rxBuf[offset], rxLen - offset,
                                             address, function);
    if (result.hasDataPacket) {
        parsed.hasDataPacket = true;
        memcpy(parsed.dataPacket, result.dataPacket, 300);
    }
    return parsed;
}

// ═════════════════════════════════════════════════════════════════
// Internal helpers
// ═════════════════════════════════════════════════════════════════

static size_t readResponse(HardwareSerial& port, uint8_t* buf,
                           size_t maxLen, unsigned long timeoutMs) {
    size_t pos = 0;
    unsigned long start = millis();
    unsigned long lastByte = 0;

    while (millis() - start < timeoutMs) {
        if (port.available()) {
            if (pos >= maxLen) break;
            buf[pos++] = port.read();
            lastByte = millis();
        } else if (pos > 0 && millis() - lastByte > 50) {
            // 50ms silence after receiving data = end of frame
            break;
        }
    }
    return pos;
}

static RS485Result parseModbusResponse(const uint8_t* buf, size_t len,
                                       uint8_t address, uint8_t function) {
    RS485Result result = {};
    result.hasDataPacket = false;

    // Check address
    if (buf[0] != address) {
        result.error = RS485Error::WRONG_ADDRESS;
        return result;
    }

    // Check for Modbus exception (function | 0x80)
    if (buf[1] == (function | 0x80) && len == 5) {
        uint16_t rxCrc = crc16_modbus(buf, 3);
        uint16_t gotCrc = (uint16_t)buf[3] | ((uint16_t)buf[4] << 8);
        if (rxCrc != gotCrc) {
            result.error = RS485Error::CRC_MISMATCH;
        } else {
            result.error = RS485Error::MODBUS_EXCEPTION;
            result.modbusException = buf[2];
        }
        return result;
    }

    // Validate CRC
    if (len < 4) {
        result.error = RS485Error::CRC_MISMATCH;
        return result;
    }

    uint16_t rxCrc = crc16_modbus(buf, len - 2);
    uint16_t gotCrc = (uint16_t)buf[len - 2] | ((uint16_t)buf[len - 1] << 8);
    if (rxCrc != gotCrc) {
        result.error = RS485Error::CRC_MISMATCH;
        return result;
    }

    // Extract payload (skip address + function, trim CRC)
    size_t payloadLen = len - 4;
    if (payloadLen > sizeof(result.data)) {
        result.error = RS485Error::BUFFER_OVERFLOW;
        return result;
    }
    memcpy(result.data, &buf[2], payloadLen);
    result.dataLen = payloadLen;
    result.error = RS485Error::OK;
    return result;
}
