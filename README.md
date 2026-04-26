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
* JK-BMS RS485 Modbus V1.1 Register Map: [PDF](protocols/BMS%20RS485%20Modbus%20V1.1%20Register%20Map%20(for%20PB2A16S20P).pdf) (source: [esphome-jk-bms/docs](https://github.com/syssi/esphome-jk-bms/tree/main/docs/pb2a16s20p))


### Initial installation of dependencies:
Install python: `sudo apt-get install python` or `brew install python`  
`brew install pipx`  
Install PlatformIO: `pipx install platformio`  

First compilation: `pio run`  
First flashing via UART: `pio run -t upload`  
Debug via UART: `pio device monitor`
