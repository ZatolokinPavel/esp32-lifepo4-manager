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

### Initial installation of dependencies:
Install python: `sudo apt-get install python` or `brew install python`  
`brew install pipx`  
Install PlatformIO: `pipx install platformio`  

First compilation: `pio run`  
First flashing via UART: `pio run -t upload`  
Debug via UART: `pio device monitor`

---

## Protocols

#### JK-BMS (RS485 Modbus)

Register map: [JK-BMS RS485 Modbus V1.1 Register Map](protocols/BMS%20RS485%20Modbus%20V1.1%20Register%20Map%20(for%20PB2A16S20P).pdf) (for PB2A16S20P) — source: [esphome-jk-bms/docs](https://github.com/syssi/esphome-jk-bms/tree/main/docs/pb2a16s20p)

**Reading BMS status:**
> The BMS Modbus address is configured in BMS settings ("Device Addr."), in our case `0x04`.

There are two ways to read the BMS state.

**Way 1 — Standard Modbus Read Holding Registers (function 0x03):**

Send a standard Modbus message to read a range of registers according to the [register map](protocols/BMS%20RS485%20Modbus%20V1.1%20Register%20Map%20(for%20PB2A16S20P).pdf). The register address is `base + offset`, where base is the address field from the register map (e.g. `0x1200` for runtime data) and offset is taken directly from the offset column.

Example — read 32 registers starting from `0x1290` (runtime data, doc offsets 144–175):
```
04 03 12 90 00 20 [CRC_Lo] [CRC_Hi]
```

The BMS responds with a standard Modbus response containing the register data. JK-BMS uses byte-level addressing (each address = 1 byte), so the byte offset within the response data corresponds directly to `doc_offset - start_doc_offset`.

**Way 2 — Bulk read via trigger registers (non-standard):**

Writing value `00 00` to a special "trigger" register causes the BMS to dump an entire data area as a single response. Three trigger registers are known:

| Trigger Register | Response Type | Data Area | Content |
|-----------------|---------------|-----------|---------|
| `0x161E` | `01` | `0x1000` | Settings/Parameters (voltages, currents, temperatures, switches) |
| `0x1620` | `02` | `0x1200` | Realtime Data (cell voltages, current, temperatures, SOC, alarms) |
| `0x161C` | `03` | `0x1400` | Device Info (model, HW/SW versions, serial numbers, protocol config) |

Example — read realtime data via `0x1620`:
```
04 10 16 20 00 01 02 00 00 [CRC_Lo] [CRC_Hi]
```

The BMS responds with a 300-byte data array followed by a standard Modbus write confirmation:

1. **Data array (always 300 bytes):** starts with 4-byte header `55 AA EB 90`, followed by 1-byte **response type** (`01`, `02`, or `03` — identifies which data area is being returned), then `00`, then 294 bytes of data (zero-padded if the actual data is shorter). Register map offsets are relative to the first byte after this 6-byte header, and correspond to the offset column within the matching data area (e.g. `0x1200` for type `02`). See the [register map PDF](protocols/BMS%20RS485%20Modbus%20V1.1%20Register%20Map%20(for%20PB2A16S20P).pdf) for field definitions.

2. **Modbus write confirmation (8 bytes):**
   ```
   04 10 16 20 00 01 [CRC_Lo] [CRC_Hi]
   ```
