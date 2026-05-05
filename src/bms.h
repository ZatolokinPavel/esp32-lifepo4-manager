#pragma once
#include <Arduino.h>

struct BMSData {
    bool     online;
    uint32_t timestamp;       // millis() of last successful read

    // Pack
    int32_t  current;         // mA, signed (negative = discharging), ток батареи, Current
    uint32_t power;           // mW, мощность, передаваемая в/из батареи, Battery Power
    uint8_t  soc;             // %, процент заряда аккумулятора, State of Charge
    uint32_t capRemain;       // mAh, ёмкость оставшаяся, SOC Capacity Remain
    uint32_t capNominal;      // mAh, ёмкость полная, SOC Full Charge Capacity

    // MOS state
    bool     chargeMOS;       // транзисторы заряда: on - идёт заряд; off - зарядка не подключена, запрещена или ошибка
    bool     dischargeMOS;    // состояние транзисторов разряда
};

/// Poll BMS over RS485 and return parsed data.
/// Uses modbus_message internally (address=4, function=0x03 Read Holding Registers).
BMSData readBmsStatus(HardwareSerial& port);
