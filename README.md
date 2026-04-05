# STM32 W5500 TCP Server with MQ-2 Gas Sensor

## Project Overview
This project implements a TCP server on an STM32 Blue Pill (STM32F103C8T6) using the W5500 Ethernet module. The server uses a static IP address and allows remote control of an LED and monitoring of an MQ-2 gas sensor over a network connection on port 5000.

Key Features:
- TCP server with static IP configuration (no DHCP)
- Remote LED control (ON/OFF/TOGGLE/STATUS)
- MQ-2 gas sensor monitoring (voltage, PPM, level)
- Periodic sensor data streaming (every 5 seconds)
- Real-time command response without blocking

## Project Functionality

### Hardware Configuration

https://github.com/user-attachments/assets/54fb3c90-6c4b-4000-b320-3ebbdf4ceb6b

### TCP client control

https://github.com/user-attachments/assets/ee12a8c4-3a94-4d38-b045-1f45fb906525

Left: UART debug output showing STM32-PC communication and waiting for client connection <br>
Right: Hercules TCP client connected to server, sending commands to control LED and read MQ-2 sensor

### Static IP Configuration in windows

<img width="610" height="931" alt="static_ip_config" src="https://github.com/user-attachments/assets/c71905ab-f94f-45d1-b1cd-92355ae35575" />

## Project Schematic

<img width="1460" height="555" alt="schematic diagram" src="https://github.com/user-attachments/assets/45e85333-ec85-48a9-9ba0-5a8ab77099a4" />

## Pin Configuration
| Peripheral | Pin | Connection | Notes |
|------------|-----|------------|-------|
| **MQ-2 Gas Sensor** | PA0 | A0 | Analog Connection |
| | 5V | VCC | Power |
| | GND | GND | Common ground |
| **Wiznet W5500** | PA5 | SCK | SPI1 Clock |
| | PA6 | MISO | SPI1 Master In Slave Out |
| | PA7 | MOSI | SPI1 Master Out Slave In |
| | PA4 | CS | Chip Select |
| | PA3 | Reset | Reset Pin |
| | 3.3V | VCC | Power |
| | GND | GND | Common ground |
| **USART1** | PA9 | TX to USB-Serial RX | 115200 baud, 8-N-1 |
| | PA10 | RX to USB-Serial TX | Optional for commands |

## TCP Server

The TCP server runs on the STM32 and listens for incoming client connections on port 5000 with a static IP address (192.168.1.10).

### How it works:
- Initializes the W5500 Ethernet controller via SPI communication
- Creates a TCP socket and enters listening mode
- Waits for client connection (Hercules, PuTTY, or custom client)
- Once connected, processes incoming commands in a non-blocking loop
- Supports simultaneous operations: LED control while streaming sensor data
- Maintains connection until client disconnects, then waits for new client

### Command processing is non-blocking:
- Periodic sensor data sends every 5 seconds (if START command received)
- LED commands execute immediately without interrupting sensor streaming
- Single GAS command provides instant sensor reading on demand

🔗 [View TCP Server Code](https://github.com/rubin-khadka/STM32_W5500_TCP_Server/blob/main/Core/Src/tcp_server.c)

## MQ-2 Gas Sensor Driver

The MQ-2 sensor detects combustible gases (LPG, propane, methane, smoke, hydrogen, alcohol) by measuring resistance changes in its tin dioxide (SnO₂) sensing element.

### How it works:
- Sensor is powered with 5V and outputs analog voltage (0-5V)
- A voltage divider (10kΩ + 20kΩ) scales the output to 0-3.3V for STM32 ADC
- Clean air produces ~0.2-0.3V; gas presence increases voltage up to 4-5V

### Driver features:
- Reads raw ADC value from PA0 pin
- Applies scaling factor (8x) to compensate for sensor variations
- Converts voltage to PPM using linear formula: ppm = 50 + (voltage - 0.20) * 500
- Classifies gas level: NORMAL (<0.5V), LOW (0.5-1.2V), MEDIUM (1.2-2.0V), HIGH (2.0-3.0V), CRITICAL (>3.0V)
- Provides alarm detection when voltage exceeds 2.5V

No calibration required - driver uses fixed formulas tuned for this specific sensor.

🔗 [View MQ-2 Gas Sensor Driver](https://github.com/rubin-khadka/STM32_W5500_TCP_Server/blob/main/Core/Src/mq2_sensor.c)

## Available Commands
| Command | Description |
|---------|-------------|
| `ON` | Turn LED on |
| `OFF` | Turn LED off |
| `TOGGLE` | Toggle LED state |
| `STATUS` | Check LED status |
| `GAS` | Get single gas reading |
| `START` | Begin periodic sensor data (5 sec interval) |
| `STOP` | Stop periodic sensor data |
| `HELP` | Show all commands |

## Related Projects 
- [STM32_FreeRTOS_W5500_TCP_Server](https://github.com/rubin-khadka/STM32_FreeRTOS_W5500_TCP_Server)

## Resources
- [STM32F103 Datasheet](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf)
- [STM32F103 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [WIZNET W5500 Datasheet](https://cdn.sparkfun.com/datasheets/Dev/Arduino/Shields/W5500_datasheet_v1.0.2_1.pdf)
- [MQ-2 Gas Sensor Datasheet](https://www.handsontec.com/dataspecs/MQ2-Gas%20Sensor.pdf)

## Project Status
- **Status**: Complete
- **Version**: v1.0
- **Last Updated**: April 2026

## Contact
**Rubin Khadka Chhetri**  
📧 rubinkhadka84@gmail.com <br>
🐙 GitHub: https://github.com/rubin-khadka
