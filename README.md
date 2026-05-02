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
| `0x1624` | `05` | undocumented | System Log (compact event history with BMS uptime timestamps) |
| `0x1622` | `06` | undocumented | Detail Log (full event history with BMS state snapshots) |

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

**Modifying BMS parameters:**

Parameters are written using standard Modbus Write Multiple Registers (function `0x10`). The register address is `base + offset` as described above. All RW registers in the [register map](protocols/BMS%20RS485%20Modbus%20V1.1%20Register%20Map%20(for%20PB2A16S20P).pdf) can be modified this way. No authentication is required.

Example — enable balancing (BalanEN at `0x1078`, UINT32, value `1`):
```
04 10 10 78 00 02 04 00 00 00 01 E8 E1
```
Response: `04 10 10 78 00 02 C5 44`

Example — disable balancing (same register, value `0`):
```
04 10 10 78 00 02 04 00 00 00 00 29 21
```
Response: `04 10 10 78 00 02 C5 44`

**System Log (trigger register `0x1624`, response type `0x05`):**

Writing `00 00` to register `0x1624` triggers a single 300-byte response containing a compact event history. This is the data shown in the "Logging" tab of the JK-BMS-MONITOR application, displayed as `Before [XD YH ZM WS] - [event]`.

Request:
```
04 10 16 24 00 01 02 00 00 [CRC_Lo] [CRC_Hi]
```

Response: a single 300-byte data packet followed by a Modbus write confirmation.

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0–3 | 4 | Header | `55 AA EB 90` |
| 4 | 1 | Response Type | `0x05` |
| 5 | 1 | (zero) | Always `0x00` |
| 6–10 | 5 | Metadata | First 5-byte slot — purpose unclear (observed: `35 01 00 00 08`) |
| 11–260 | 250 | Log Entries | Up to 50 entries × 5 bytes each, stored as a **circular buffer** |
| 261–298 | 38 | Padding | Zero-filled |
| 299 | 1 | Checksum | Sum of bytes 0–298, mod 256 |

After the data packet, the standard Modbus confirmation follows:
```
04 10 16 24 00 01 [CRC_Lo] [CRC_Hi]
```

**Log entry structure (5 bytes, little-endian):**

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0–3 | 4 | UINT32 | BMS Uptime | Seconds since BMS power-on (not wall-clock time) |
| 4 | 1 | UINT8 | Event Type | Same event type codes as Detail Log (see table below) |

The entries are stored in a **circular buffer** — when the buffer is full, the oldest entries are overwritten. To read entries in chronological order, find the wrap-around point (where the uptime value jumps down) and read from there.

The JK-BMS-MONITOR application displays each entry as `Before [XD YH ZM WS]`, computed as `current_uptime − entry_uptime`. The current BMS uptime can be derived from any entry: `current_uptime = entry_uptime + displayed_before_duration`.

> **Note:** Unlike the Detail Log (type `0x06`) which uses absolute wall-clock timestamps (epoch 2020-01-01), the System Log uses **relative BMS uptime** — seconds since the BMS was powered on. This means the timestamps lose meaning after a power cycle.

**Detail Log (trigger register `0x1622`, response type `0x06`):**

Writing `00 00` to register `0x1622` triggers a multi-packet response containing the full event history with BMS state snapshots. This is the data shown in the "Detail Logs" tab of the JK-BMS-MONITOR 2.7.0 application.

Request:
```
04 10 16 22 00 01 02 00 00 [CRC_Lo] [CRC_Hi]
```

The BMS responds with **N data packets** (each 300 bytes) followed by a standard Modbus write confirmation. Unlike other trigger registers that return a single 300-byte packet, this one streams dozens of packets sequentially.

Each 300-byte data packet has the following structure:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0–3 | 4 | Header | `55 AA EB 90` |
| 4 | 1 | Response Type | `0x06` |
| 5 | 1 | (varies) | Not a counter — appears to be the first byte of the first log entry's timestamp that was displaced by the header fields |
| 6–7 | 2 | Entry Offset | LE uint16 — index of the first log entry in this packet (increments by 12 each packet) |
| 8 | 1 | Entry Count | Number of log entries in this packet (usually `0x0C` = 12, last packet may have fewer) |
| 9–296 | 288 | Log Entries | Up to 12 entries × 24 bytes each |
| 297–299 | 3 | Padding + Checksum | Last byte is a simple checksum (sum of bytes 0–298, mod 256) |

After all data packets, the standard Modbus confirmation follows:
```
04 10 16 22 00 01 [CRC_Lo] [CRC_Hi]
```

**Log entry structure (24 bytes, little-endian):**

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0–3 | 4 | UINT32 | Timestamp | Seconds since **2020-01-01 00:00:00 UTC** |
| 4 | 1 | UINT8 | Event Type | See event type table below |
| 5 | 1 | UINT8 | State Flags | Bit flags: bit0=Charge MOS, bit1=Discharge MOS, bit2=Balance, bit3=Heating |
| 6 | 1 | UINT8 | Max Cell V # | Cell number with highest voltage (0-based) |
| 7 | 1 | UINT8 | Min Cell V # | Cell number with lowest voltage (0-based) |
| 8–9 | 2 | UINT16 | Max Cell Voltage | mV (e.g. 3352 = 3.352V) |
| 10–11 | 2 | UINT16 | Min Cell Voltage | mV |
| 12–13 | 2 | UINT16 | Battery Voltage | 10mV (e.g. 2680 = 26.80V) |
| 14–15 | 2 | INT16 | Battery Current | 0.1A, signed (e.g. -10 = -1.0A discharging) |
| 16–17 | 2 | UINT16 | Capacity Remain | 0.1Ah (e.g. 2593 = 259.3Ah) |
| 18–19 | 2 | UINT16 | Full Charge Cap | 0.1Ah (e.g. 2800 = 280.0Ah) |
| 20 | 1 | UINT8 | Max Temperature | °C |
| 21 | 1 | UINT8 | Min Temperature | °C |
| 22 | 1 | UINT8 | MOS Temperature | °C |
| 23 | 1 | UINT8 | Heat Current | A (0 in all observed data) |

**Event type codes (byte 4 of log entry):**

| Code | Hex | Event |
|------|-----|-------|
| 1 | `0x01` | Boot |
| 3 | `0x03` | APP close charge |
| 4 | `0x04` | APP open charge |
| 5 | `0x05` | APP close discharge |
| 6 | `0x06` | APP open discharge |
| 18 | `0x12` | Cell overcharge protection released |
| 45 | `0x2D` | Button to turn it off |
| 59 | `0x3B` | Time calibration |
| 100+ | `0x64`+ | Cell N overcharge protection (N = code − 100, 0-based) |

> **Note on byte 5 of the packet header:** This byte is *not* a packet counter. It appears to be the first byte of the first log entry's 4-byte timestamp that was "pushed out" by the 6-byte packet header. The actual entry data starts at offset 9, and the entry offset field at bytes 6–7 provides the sequential entry index.

> **Note on timestamp epoch:** The BMS uses **2020-01-01 00:00:00 UTC** as epoch. Boot events always show timestamp `186451200` which decodes to `2025-11-28 00:00:00` — this appears to be a fixed "unknown time" placeholder used when the RTC hasn't been synchronized yet.

> **Note on duplicate entries:** Most log events appear as pairs of consecutive entries with identical or near-identical data (timestamps may differ by ±1 second). This matches the behavior observed in the Excel export from JK-BMS-MONITOR.

**Password register (undocumented):**  
The BMS password is stored in plaintext at register `0x1470` (area `0x1400`, offset `0x0070`), 12 bytes, ASCII, zero-padded. This register is not listed in the official register map PDF.  
Example — set password to "123456":  
`04 10 14 70 00 06 0C 31 32 33 34 35 36 00 00 00 00 00 00 [CRC_Lo] [CRC_Hi]`  
The password is a UI-level lock only — the BMS does not require authentication for register writes over RS485. Any device on the bus can read/write settings directly via standard Modbus commands.
