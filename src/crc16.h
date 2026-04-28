#pragma once
#include <stdint.h>
#include <stddef.h>

/// CRC16 Modbus (polynomial 0xA001, init 0xFFFF, result byte-swapped)
uint16_t crc16_modbus(const uint8_t* data, size_t len);
