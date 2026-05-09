#pragma once
#include <Arduino.h>

struct BMSData {
    bool     online;
    uint32_t timestamp;       // millis() of last successful read

    // Pack
    uint32_t voltage;         // mV, напряжение батареи, Battery Voltage
    uint32_t power;           // mW, мощность, передаваемая в/из батареи, Battery Power
    int32_t  current;         // mA, signed (negative = discharging), ток батареи, Current
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

// ── BMS Control ──────────────────────────────────────────────────
/// Переключение контролов BMS: заряд, разряд, балансировка.
enum class BMSControl : uint8_t {
    Charge,
    Discharge,
    Balance
};

/// Отправить команду включения/выключения контрола BMS.
/// @return true если BMS подтвердила команду
bool toggleBmsControl(HardwareSerial& port, BMSControl control, bool enable);

// ── Управление балансировкой ─────────────────────────────────────

/// Батарея 24В собрана в двух корпусах (по 4 ячейки), соединённых длинным
/// проводом с одной общей BMS. Балансировка допустима только при отсутствии
/// значительного тока через батарею, иначе падение напряжения на проводе
/// приведёт к ложной разбалансировке.
///
/// Условия:
///   Включить:  SOC == 100% && ток зарядный >= 0 && ток < 2000 мА
///   Выключить: ток разрядный (< 0) ИЛИ ток зарядный > 2500 мА
///
/// Гистерезис 2.0A / 2.5A предотвращает дребезг переключений.

/// Тристейт: после старта состояние неизвестно (Unknown).
/// Мы не запрашиваем BMS — просто ждём условий для переключения.
enum class BalanceAllowed : uint8_t {
    Unknown,  // начальное состояние, ещё не переключали
    Yes,      // балансировка включена
    No        // балансировка выключена
};

struct BalancingState {
    BalanceAllowed allowed = BalanceAllowed::Unknown;
};

/// Вызывается после каждого успешного чтения статуса BMS.
/// При необходимости отправляет команду включения/выключения балансировки.
/// @param bms      — текущие данные BMS
/// @param state    — состояние балансировки (обновляется на месте)
/// @param port     — UART порт для отправки команды в BMS
void balancingManagement(const BMSData& bms, BalancingState& state,
                         HardwareSerial& port);
