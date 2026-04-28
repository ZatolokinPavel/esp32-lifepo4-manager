# ESP32 MUST Inverter & JK-BMS Controller

This project provides a monitoring and control solution for the **MUST PV18-3224 VPM II** inverter and **JK-B2A8S20P-HDC** (JK-B2A8S20P) BMS using an ESP32 and RS485 communication.

### Key Features:
* **RS485 Protocol Reverse Engineering:** Includes detailed data frames captured via logic analyzer.
* **Dual Device Support:** Simultaneous communication with both Inverter and LiFePo4 Battery.
* **No Cloud Required:** Designed for secure, local-only IoT setups.

### Hardware:
* **Inverter:** MUST PV18-3224 VPM II (Firmware: [Ваша версия, если знаете])
* **BMS:** JK-B2A8S20P-HDC (2A Active Balance, 200A, Smart BMS, Firmware: v19.20)
* **MCU:** ESP32 (WT32-ETH01 V1.4)
* **Interface:** TTL-to-RS485 modules

### Protocols:

#### JK-BMS (RS485 Modbus)

Register map: [JK-BMS RS485 Modbus V1.1 Register Map](protocols/BMS%20RS485%20Modbus%20V1.1%20Register%20Map%20(for%20PB2A16S20P).pdf) (for PB2A16S20P) — source: [esphome-jk-bms/docs](https://github.com/syssi/esphome-jk-bms/tree/main/docs/pb2a16s20p)

**Reading BMS status:**

To read the current state, send a standard Modbus message to write value `00 00` to register `0x1620`. The BMS Modbus address is configured in BMS settings ("Device Addr."), in our case `0x04`.

```
04 10 16 20 00 01 02 00 00 [CRC_Lo] [CRC_Hi]
```

**Response:**

The BMS responds with a 300-byte data array followed by a standard Modbus confirmation:

1. **Data array (300 bytes):** starts with header `55 AA EB 90`, followed by two unknown bytes `02 00`, then data bytes according to the register map address field `0x1200` in the [register map PDF](protocols/BMS%20RS485%20Modbus%20V1.1%20Register%20Map%20(for%20PB2A16S20P).pdf). Register map offsets are relative to the first byte after the 6-byte header.

2. **Modbus confirmation (8 bytes):**
   ```
   04 10 16 20 00 01 [CRC_Lo] [CRC_Hi]
   ```


### Initial installation of dependencies:
Install python: `sudo apt-get install python` or `brew install python`  
`brew install pipx`  
Install PlatformIO: `pipx install platformio`  

First compilation: `pio run`  
First flashing via UART: `pio run -t upload`  
Debug via UART: `pio device monitor`
