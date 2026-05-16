# MUST RS485 Modbus RTU communication protocol 1.4.15
MUST PH1800 PV1800 EP1800 PV3500 EP3500 RS485 Modbud RTU communication protocol 1.4.15

## PV Charger ModbusRTU (ID == 4)

### PV Charger Control Message

| Register<br>Address | Type | Name | Physical Unit | Effective Range | Note |
|---|---|---|---|---|---|
| 0x2711 | R | The type of machine | PC1600 | | |
| 0x2712 | RW | Serial number High | | | |
| 0x2713 | RW | Serial number Low | | | |
| 0x2714 | R | Hardware version | 1.0.00 | | |
| 0x2715 | R | Software version | 1.0.00 | | |
| 0x2716 | RW | PV voltage calibration coefficient | 16384 | | |
| 0x2717 | RW | Battery voltage calibration coefficient | 16384 | | |
| 0x2718 | RW | Charger current calibration coefficient | 16384 | | |
| 0x2719 - 0x2774 | RW | reserved | reserved | | |
| 0x2777 | RW | Float voltage | 0.1V | 480~584 (48.0-58.4)V the default value is 54.0V | |
| 0x2778 | RW | Absorption voltage | 0.1V | 480~584 (48.0-58.4)V the default value is 56.4V | |
| 0x2779 | RW | Battery low voltage (for PV;PH) | 0.1V | 340~440 (34.0-44.0)V the default value is 34.0V | |
| 0x277A | RW | reserved | | | |
| 0x277B | RW | Battery High voltage (for PV;PH) | 0.1V | 580~600 (58.0-60.0)V the default value is 60.0V | |
| 0x277C | RW | PV max charger current (for PV;PH) | 0.1A | 0.1A effective range: (0-80.0)A | |
| 0x277E | RW | Battery type | | 0:no choose, 1:Use defined battery, 2:lithium battery, 3:SEALED_LEAD battery, 4:AGM battery, 5:GEL battery, 6:FLOODED battery | the default value is 4; effective range: 0,6 |
| 0x277F | RW | Battery AH | 1AH | effective range: (0-900)AH the default value is 100AH | |
| 0x2780 | RW | Remove the accumulated data | | 0: No remove the accumulated data, 1: Remove the accumulated data | the default value is 0; effective range: 0,1 |
| 0x2786 | RW | Battery equalization enable | | 0:Disable 1:Enable | effective range: 0,1 the default value is 0 |
| 0x2787 | RW | Battery Equalization voltage | 0.1V | 48V:480-640 (48.0-64.0)V the default value is 58.4V; 24V:240-320 (24.0-32.0)V the default value is 29.2V; 12V:120-160 (12.0-16.0)V the default value is 14.6V | |
| 0x2788 | RW | reserved | reserved | | |
| 0x2789 | RW | Battery equalized time | 1min | 5-900 (5-900)min the default value is 150min | |
| 0x278A | RW | Battery Equalized timeout | 1min | 5-900 (5-900)min the default value is 150min | |
| 0x278B | RW | Equalization interval | 1day | 0-90 (0-90)day the default value is 30 days | |
| 0x278C | RW | Equalization actived immediately | | 0:No effect, 1:Action | the default value is 0; effective range: 0,1 |

### PV Charger Display Message

| Register<br>Address | Type | Name | Physical Unit | Effective Range |
|---|---|---|---|---|
| 0x3B61 | R | Charger workstate | | 0:Initialization mode, 1:Selftest Mode, 2:Work Mode, 3:Stop Mode |
| 0x3B62 | R | Mppt state | | 0:Stop, 1:MPPT, 2:Current limiting |
| 0x3B63 | R | Charging state | | 0:Stop, 1:Absorb charge, 2:Float charge, 3:EQ charge |
| 0x3B64 | R | reserved | | |
| 0x3B65 | R | PV voltage | 0.1V | (0.0-150.0)V |
| 0x3B66 | R | Battery voltage | 0.1V | (0.0-80.0)V |
| 0x3B67 | R | Charger current | 0.1A | (0.0-90.0)A |
| 0x3B68 | R | Charger power | 1W | (0-5000)W |
| 0x3B69 | R | Radiator temperature | 1℃ | (-40-150)℃ |
| 0x3B6A | R | External temperature | 1℃ | (-40-150)℃ |
| 0x3B6B | R | Battery Relay | | 0:Disconnect 1:Connect |
| 0x3B6C | R | PV Relay | | 0:Disconnect 1:Connect |
| 0x3B6D | R | Error message | | Refer to frame Charger Error message 1 |
| 0x3B6E | R | Warning message | | Refer to frame Charger Warning message 1 |
| 0x3B6F | R | BattVol Grade | 1V | |
| 0x3B70 | R | Rated Current | 0.1A | |
| 0x3B71 | R | Accumulated PV power high | 1000KWH | |
| 0x3B72 | R | Accumulated PV power low | 0.1KWH | |
| 0x3B73 | R | Accumulated day | 1day | |
| 0x3B74 | R | Accumulated hour | 1hour | |
| 0x3B75 | R | Accumulated minute | 1minute | |

### PV Charger Alarm

#### Charger Error message 1

| Bit | Meaning | Description |
|---|---|---|
| 0 | Hardware protection | 0:normal 1:fault |
| 1 | Over current | 0:normal 1:fault |
| 2 | Current sensor error | 0:normal 1:fault |
| 3 | Over temperature | 0:normal 1:fault |
| 4 | PV voltage is too high | 0:normal 1:fault |
| 5 | PV voltage is too low | 0:normal 1:fault |
| 6 | Battery voltage is too high | 0:normal 1:fault |
| 7 | Battery voltage is too Low | 0:normal 1:fault |
| 8 | Current is uncontrollable | 0:normal 1:fault |
| 9 | Parameter error | 0:normal 1:fault |
| 10 | reserved | 0:normal 1:fault |
| 11 | reserved | 0:normal 1:fault |
| 12 | reserved | 0:normal 1:fault |
| 13 | reserved | 0:normal 1:fault |
| 14 | reserved | 0:normal 1:fault |
| 15 | reserved | 0:normal 1:fault |

#### Charger Warning message 1

| Bit | Meaning | Description |
|---|---|---|
| 0 | Fan Error | 0:normal 1:fault |
| 1 | reserved | 0:normal 1:fault |
| 2 | reserved | 0:normal 1:fault |
| 3 | reserved | 0:normal 1:fault |
| 4 | reserved | 0:normal 1:fault |
| 5 | reserved | 0:normal 1:fault |
| 6 | reserved | 0:normal 1:fault |
| 7 | reserved | 0:normal 1:fault |
| 8 | reserved | 0:normal 1:fault |
| 9 | reserved | 0:normal 1:fault |
| 10 | reserved | 0:normal 1:fault |
| 11 | reserved | 0:normal 1:fault |
| 12 | reserved | 0:normal 1:fault |
| 13 | reserved | 0:normal 1:fault |
| 14 | reserved | 0:normal 1:fault |
| 15 | reserved | 0:normal 1:fault |


## Inverter ModbusRTU (ID == 4)

### Inverter Control Message

| Register<br>Address | Type | Name | Physical Unit | Effective Range | Note |
|---|---|---|---|---|---|
| 0x4E20 | R | The type of machine High | PH/EP/PV | | |
| 0x4E21 | R | The type of machine Low | 1800/3000 | | |
| 0x4E22 | RW | Serial number High | | | |
| 0x4E23 | RW | Serial number Low | | | |
| 0x4E24 | R | Hardware version | 1.00.00 | | |
| 0x4E25 | R | Software version | 1.00.00 | | |
| 0x4E26 | R | Communication Protocal Edition | 1.00.00 | | 1.04.15 |
| 0x4E27 | RW | reserved | | | |
| 0x4E28 | RW | reserved | | | |
| 0x4E29 | RW | Battery voltage calibration coefficient | 16384 | | X=(actual value/displayed value)*last calibration value, or 0xffff (-1) as no correction. For base value use 16384 |
| 0x4E2A | RW | Inverter voltage calibration coefficient | 16384 | | |
| 0x4E2B | RW | Grid voltage calibration coefficient | 16384 | | |
| 0x4E2C | RW | Bus voltage calibration coefficient | 16384 | | |
| 0x4E2D | RW | Control Current calibration coefficient | 16384 | | |
| 0x4E2E | RW | Inverter Current calibration coefficient | 16384 | | |
| 0x4E2F | RW | Grid Current calibration coefficient | 16384 | | |
| 0x4E30 | RW | Load Current calibration coefficient | 16384 | | |
| 0x4E85 | RW | Inverter offgrid work enable | 0:OFF 1:ON | effective range: 0,1 the default value is 1 | 1:Turn on the inverter output when the grid is off. 0:Shut down the inverter output when the grid is off. |
| 0x4E86 | RW | Inverter output voltage Set | 220.0V-240.0V | (2200-2400) | Set the output voltage amplitude. |
| 0x4E87 | RW | Inverter output frequency Set | 50.00Hz/60.00Hz | 5000/6000 | Set the output frequency. |
| 0x4E88 | RW | Inverter search mode enable | 0:OFF 1:ON | effective range: 0,1 the default value is 0 | 0:If disabled, no matter connect load is low or high, the on/off status of inverter output will not be effected. 1:If enable, the inverter begins search mode if the AC load connected is pretty low or not detected. |
| 0x4E89 | RW | ONGrid switch | 0:OFF, 1:ON | effective range: 0,1 the default value is 0 | 0: Inverter detached from grid, using only PV and battery. 1: Inverter connected to the grid |
| 0x4E8C | RW | Inverter discharger to grid enable | 48V: 0:OFF 1:ON; 24V: Null | effective range: 0,1 the default value is 1 | 1:Enable Inverter discharging from battery to grid when connected to Grid. 0:Disable |
| 0x4E8D | RW | Energy use mode | | 48V: 1:SBU; 2:SUB; 3:UTI; 4:SOL (for PV;PH), 1:BAU; 3:UTI; 4:BOU (for EP); 12V/24V: 1:SBU; 3:UTI; 4:SOL (for PV;PH), 1:BU; 3:UTI (for EP) | refer to user's manual |
| 0x4E8F | RW | Grid protect standard | | 0:VDE4105; 1:UPS; 2:home; 3:GEN | refer to user's manual |
| 0x4E90 | RW | SolarUse Aim | | 0:LBU 1:BLU (default) (for PV;PH); 0:LB 1:LU (default) (for EP) | |
| 0x4E91 | RW | Inverter max discharger current | 48V: 0.1A (AC); 12V/24V: Null | 1.0A-21.7A (10-217) | Set the max discharging current from Inverter |
| 0x4E96 | RW | Battery stop discharging voltage | 0.1V | 440~580 (44.0-58.0)V the default value is 46.0V | refer to user's manual |
| 0x4E97 | RW | Battery stop charging voltage | 0.1V | 440~580 (44.0-58.0)V the default value is 54.0V | refer to user's manual |
| 0x4E9D | RW | Grid max charger current set | 0.1A (DC) | 10~800 (1.0-80.0)A the default value is 60.0A | |
| 0x4E9F | RW | Battery low voltage | 0.1V | 400~480 (40.0-48.0)V the default value is 40.8V | |
| 0x4EA0 | RW | Battery high voltage | 0.1V | 580~600 (58.0-60.0)V the default value is 60.0V | |
| 0x4EA4 | RW | Max Combine charger current | 0.1A (DC) (for PV;PH) | 10~1400 (1.0-140.0)A the default value is 60.0A | |
| 0x4EAE | RW | System setting | | refer to the frame System setting bit | |
| 0x4EAF | RW | Charger source priority | | 0:Solar first (for PV;PH), 2:Solar and Utility (default) (for PV;PH), 3:Only Solar (for PV;PH); 2:Utility charger enable (default) (for EP), 3:Utility charger disable (for EP) | |
| 0x4EB0 | RW | Solar power balance | | 0:SBD, 1:SBE | |
| 0x4EF5 | RW | Remove the accumulated data | | 0: No remove the accumulated data, 1: Remove the accumulated data | the default value is 0; effective range: 0,1 |
| 0x4EF6 | RW | Reset the parameter | | 0: No effect, 1: Action | the default value is 0; effective range: 0,1 |

### Inverter Display Message

| Register<br>Address | Type | Name | Physical Unit | Note |
|---|---|---|---|---|
| 0x6271 | R | Work state | | 0:PowerOn 1:SelfTest 2:OffGrid 3:Grid-Tie 4:ByPass 5:Stop 6:Grid charging |
| 0x6272 | R | AC voltage grade | | 230:230V, 120:120V |
| 0x6273 | R | Rated power (VA) | VA | |
| 0x6274 | R | reserved | | |
| 0x6275 | R | Battery voltage | 0.1V | |
| 0x6276 | R | Inverter voltage | 0.1V | |
| 0x6277 | R | Grid voltage | 0.1V | |
| 0x6278 | R | BUS voltage | 0.1V | |
| 0x6279 | R | Control current | 0.1A | |
| 0x627A | R | Inverter current | 0.1A | |
| 0x627B | R | Grid current | 0.1A | |
| 0x627C | R | Load current | 0.1A | |
| 0x627D | R | PInverter | 1W | |
| 0x627E | R | PGrid | 1W | |
| 0x627F | R | PLoad | 1W | |
| 0x6280 | R | Load percent | 1% | |
| 0x6281 | R | SInverter | 1VA | |
| 0x6282 | R | SGrid | 1VA | |
| 0x6283 | R | Sload | 1VA | |
| 0x6284 | R | reserved | | |
| 0x6285 | R | Qinverter | 1var | |
| 0x6286 | R | Qgrid | 1var | |
| 0x6287 | R | Qload | 1var | |
| 0x6288 | R | reserved | | |
| 0x6289 | R | Inverter frequency | 0.01Hz | |
| 0x628A | R | Grid frequency | 0.01Hz | |
| 0x628B | R | reserved | | |
| 0x628C | R | reserved | | |
| 0x628D | R | Inverter max number | | |
| 0x628E | R | Combine type | | |
| 0x628F | R | Inverter number | | |
| 0x6290 | R | reserved | | |
| 0x6291 | R | AC radiator temperature | 1℃ | |
| 0x6292 | R | Transformer temperature | 1℃ | |
| 0x6293 | R | DC radiator temperature | 1℃ | |
| 0x6294 | R | reserved | | |
| 0x6295 | R | Inverter relay state | | 0:Disconnect 1:Connect |
| 0x6296 | R | Grid relay state | | 0:Disconnect 1:Connect |
| 0x6297 | R | Load relay state | | 0:Disconnect 1:Connect |
| 0x6298 | R | N_Line relay state | | 0:Disconnect 1:Connect |
| 0x6299 | R | DC relay state | | 0:Disconnect 1:Connect |
| 0x629A | R | Earth relay state | | 0:Disconnect 1:Connect |
| 0x629B | R | reserved | | |
| 0x629C | R | reserved | | |
| 0x629D | R | Accumulated charger power high | 1000KWH | |
| 0x629E | R | Accumulated charger power low | 0.1KWH | |
| 0x629F | R | Accumulated discharger power high | 1000KWH | |
| 0x62A0 | R | Accumulated discharger power low | 0.1KWH | |
| 0x62A1 | R | Accumulated buy power high | 1000KWH | |
| 0x62A2 | R | Accumulated buy power low | 0.1KWH | |
| 0x62A3 | R | Accumulated sell power high | 1000KWH | |
| 0x62A4 | R | Accumulated sell power low | 0.1KWH | |
| 0x62A5 | R | Accumulated load power high | 1000KWH | |
| 0x62A6 | R | Accumulated load power low | 0.1KWH | |
| 0x62A7 | R | Accumulated self_use power high | 1000KWH | |
| 0x62A8 | R | Accumulated self_use power low | 0.1KWH | |
| 0x62A9 | R | Accumulated PV_sell power high | 1000KWH | |
| 0x62AA | R | Accumulated PV_sell power low | 0.1KWH | |
| 0x62AB | R | Accumulated grid_charger power high | 1000KWH | |
| 0x62AC | R | Accumulated grid_charger power low | 0.1KWH | |
| 0x62AD | R | Error message 1 | | Refer to frame Error message 1 |
| 0x62AE | R | Error message 2 | | Refer to frame Error message 2 |
| 0x62AF | R | Error message 3 | | Refer to frame Error message 3 |
| 0x62B0 | R | reserved | | |
| 0x62B1 | R | Warning message 1 | | Refer to frame Warning message 1 |
| 0x62B2 | R | Warning message 2 | | Refer to frame Warning message 2 |
| 0x62B3 | R | reserved | | |
| 0x62B4 | R | reserved | | |
| 0x62B5 | R | Serial number High | | |
| 0x62B6 | R | Serial number Low | | |
| 0x62B7 | R | Hardware version | 1.00.00 | |
| 0x62B8 | R | Software version | 1.00.00 | |
| 0x62B9 | R | Batt power | W | |
| 0x62BA | R | Batt current | A | |
| 0x62BB | R | Batt voltage grade | | 48:48V, 24:24V, 12:12V |
| 0x62BC | R | reserved | | |
| 0x62BD | R | Rated power (W) | W | |
| 0x62BE | R | Communication Protocal Edition | 1.00.00 | 1.04.15 |
| 0x62BF | R | Arrow Flag | bit | Refer to the frame Arrow Flag bit |

### System setting bit

| Bit | Meaning |
|---|---|
| 0 | OverLoadRestartForbid |
| 1 | OverTempRestartForbid |
| 2 | OverLoadBypassForbid |
| 3 | AutoTurnPageFlagForbid |
| 4 | GridBuzzEnable (only use by PV1800) |
| 5 | BuzzForbide (only use by PV1800) |
| 6 | LcdLightEnable |
| 7 | RecordFaultForbid |
| 8 | reserved |
| 9 | reserved |
| 10 | reserved |
| 11 | reserved |
| 12 | reserved |
| 13 | reserved |
| 14 | reserved |
| 15 | reserved |

### Arrow Flag bit

| Bit | Name | Meaning |
|---|---|---|
| 0 | PVFlag | 0:Inexistence, 1:Existence |
| 1 | LoadFlag | 0:Inexistence, 1:Existence |
| 2 | BattFlag | 0:Inexistence, 1:Existence |
| 3 | GridFlag | 0:Inexistence, 1:Existence |
| 4 | PV-to-Machine-Arrow | 0:Disconnect, 1:PV-to-Machine |
| 5 | Machine-to-Load-Arrow | 0:Disconnect, 1:Machine-to-Load |
| 6-7 | Machine-Batt-Arrow | 00:Disconnect, 01:Machine-to-Batt, 10:Batt-to-Machine, 11:Connect |
| 8-9 | Machine-Grid-Arrow | 00:Disconnect, 01:Machine-to-Grid, 10:Grid-to-Machine, 11:Connect |
| 10 | reserved | |
| 11 | reserved | |
| 12 | reserved | |
| 13 | reserved | |
| 14 | reserved | |
| 15 | reserved | |

### Inverter Alarm

#### Inverter Error message 1

| Bit | Meaning | Description |
|---|---|---|
| 0 | Fan is locked when inverter is off | 0:normal 1:fault |
| 1 | Inverter transformer over temperature | 0:normal 1:fault |
| 2 | Battery voltage is too high | 0:normal 1:fault |
| 3 | Battery voltage is too low | 0:normal 1:fault |
| 4 | Output short circuited | 0:normal 1:fault |
| 5 | Inverter output voltage is high | 0:normal 1:fault |
| 6 | Overload time out | 0:normal 1:fault |
| 7 | Inverter bus voltage is too high | 0:normal 1:fault |
| 8 | Bus soft start failed | 0:normal 1:fault |
| 9 | Main relay failed | 0:normal 1:fault |
| 10 | Inverter output voltage sensor error | 0:normal 1:fault |
| 11 | Inverter grid voltage sensor error | 0:normal 1:fault |
| 12 | Inverter output current sensor error | 0:normal 1:fault |
| 13 | Inverter grid current sensor error | 0:normal 1:fault |
| 14 | Inverter load current sensor error | 0:normal 1:fault |
| 15 | Inverter grid over current error | 0:normal 1:fault |

#### Inverter Error message 2

| Bit | Meaning | Description |
|---|---|---|
| 0 | Inverter radiator over temperature | 0:normal 1:fault |
| 1 | Solar charger battery voltage class error | 0:normal 1:fault |
| 2 | Solar charger current sensor error | 0:normal 1:fault |
| 3 | Solar charger current is uncontrollable | 0:normal 1:fault |
| 4 | Inverter grid voltage is low | 0:normal 1:fault |
| 5 | Inverter grid voltage is high | 0:normal 1:fault |
| 6 | Inverter grid under frequency | 0:normal 1:fault |
| 7 | Inverter grid over frequency | 0:normal 1:fault |
| 8 | Inverter over current protection error | 0:normal 1:fault |
| 9 | Inverter bus voltage is too low | 0:normal 1:fault |
| 10 | Inverter soft start failed | 0:normal 1:fault |
| 11 | Over DC voltage in AC output | 0:normal 1:fault |
| 12 | Battery connection is open | 0:normal 1:fault |
| 13 | Inverter control current sensor error | 0:normal 1:fault |
| 14 | Inverter output voltage is too low | 0:normal 1:fault |
| 15 | reserved | 0:normal 1:fault |

#### Inverter Warning message 1

| Bit | Meaning | Description |
|---|---|---|
| 0 | Fan is locked when inverter is on | 0:normal 1:fault |
| 1 | Fan2 is locked when inverter is on | 0:normal 1:fault |
| 2 | Battery is over-charged | 0:normal 1:fault |
| 3 | Low battery | 0:normal 1:fault |
| 4 | Overload | 0:normal 1:fault |
| 5 | Output power derating | 0:normal 1:fault |
| 6 | Solar charger stops due to low battery | 0:normal 1:fault |
| 7 | Solar charger stops due to high PV voltage | 0:normal 1:fault |
| 8 | Solar charger stops due to over load | 0:normal 1:fault |
| 9 | Solar charger over temperature | 0:normal 1:fault |
| 10 | PV charger communication error | 0:normal 1:fault |
| 11 | reserved | 0:normal 1:fault |
| 12 | reserved | 0:normal 1:fault |
| 13 | reserved | 0:normal 1:fault |
| 14 | reserved | 0:normal 1:fault |
| 15 | reserved | 0:normal 1:fault |

#### Inverter Warning message 2

| Bit | Meaning | Description |
|---|---|---|
| 0 | reserved | 0:normal 1:fault |
| 1 | reserved | 0:normal 1:fault |
| 2 | reserved | 0:normal 1:fault |
| 3 | reserved | 0:normal 1:fault |
| 4 | reserved | 0:normal 1:fault |
| 5 | reserved | 0:normal 1:fault |
| 6 | reserved | 0:normal 1:fault |
| 7 | reserved | 0:normal 1:fault |
| 8 | reserved | 0:normal 1:fault |
| 9 | reserved | 0:normal 1:fault |
| 10 | reserved | 0:normal 1:fault |
| 11 | reserved | 0:normal 1:fault |
| 12 | reserved | 0:normal 1:fault |
| 13 | reserved | 0:normal 1:fault |
| 14 | reserved | 0:normal 1:fault |
| 15 | reserved | 0:normal 1:fault |
