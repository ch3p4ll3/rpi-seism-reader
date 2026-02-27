# RPI-SEISM Reader

Firmware for an Arduino‑based 3‑channel geophone digitizer that communicates with a Raspberry Pi over RS485.  
This project is part of the [rpi‑seism](https://github.com/ch3p4ll3/rpi-seism) ecosystem – it reads analog signals from a geophone via an ADS1256 ADC, formats the data into binary packets, and streams them to the Raspberry Pi when a heartbeat signal is received.

---

## Features

- **3‑channel differential sampling** – reads three geophone components (EHZ, EHN, EHE) using an ADS1256 ADC.
- **Configurable sampling rate** – default 100 Hz, adjustable in the code.
- **RS485 communication** – half‑duplex with automatic direction control via a GPIO pin.
- **Heartbeat‑based streaming** – starts sending data only after receiving a heartbeat byte from the Pi; stops after 1 second without a heartbeat.
- **Robust packet format** – includes a header, three 32‑bit signed integers, and an XOR checksum.
- **Multi‑platform support** – tested on AVR (Uno/Nano), RP2040 (Pico, Zero, Waveshare), STM32 (Blue Pill), ESP32, and Teensy 4.0.
- **Configurable SPI pins** – easily adapt to different board layouts.

---

## Hardware Requirements

- **Arduino‑compatible board** (see [Supported Platforms](#supported-platforms))
- **ADS1256 ADC module** (8‑channel, 24‑bit)
- **MAX485** or equivalent RS485 transceiver
- **3‑component geophone** (e.g., 4.5 Hz) with appropriate signal conditioning (if needed)
- **Wires and breadboard** for connections

### Supported Platforms

The firmware includes conditional compilation for several architectures.  
Uncomment the appropriate sections in the code to match your board.

| Architecture | Boards tested               | SPI selection macros              |
|--------------|------------------------------|------------------------------------|
| AVR          | Arduino Uno, Nano            | Default (uses standard SPI pins)   |
| RP2040       | Raspberry Pi Pico, Zero, Waveshare Pico | `USE_SPI1` to switch between SPI0/SPI1 |
| STM32        | STM32F103 “Blue Pill”        | `USE_SPI2` to use SPI2 instead of SPI1 |
| ESP32        | ESP32 WROOM                  | `USE_HSPI` to use HSPI instead of VSPI |
| Teensy       | Teensy 4.0, 4.1              | `USE_SPI1` or `USE_SPI2` for alternate SPI ports |

---

## Wiring

### 1. ADS1256 to Arduino

The ADS1256 communicates via SPI. Below is a typical connection – **check your board’s pinout and the comments in the code for alternative pin assignments**.

| ADS1256 pin | Function  | Connect to Arduino                        |
|-------------|-----------|-------------------------------------------|
| SCLK        | SPI Clock | SCK pin (see platform‑specific section)   |
| DIN         | SPI MOSI  | MOSI pin                                  |
| DOUT        | SPI MISO  | MISO pin                                  |
| CS          | Chip Select | A digital pin (e.g., 10 on Uno)         |
| DRDY        | Data Ready | A digital pin (e.g., 2 on Uno)           |
| RESET       | Reset     | Optional – can be left unconnected        |
| SYNC/PDWN   | Sync/Power Down | Optional – can be left unconnected  |

**Important:** The ADS1256 requires a stable 2.5 V reference. Many modules include a REF5025 or similar; if yours does not, you must provide an external 2.5 V reference.

### 2. MAX485 (RS485) to Arduino

| MAX485 pin | Function       | Connect to Arduino                          |
|------------|----------------|---------------------------------------------|
| DI         | Driver Input   | TX pin (e.g., TX on Uno, or a UART TX pin)  |
| RO         | Receiver Output| RX pin (e.g., RX on Uno)                    |
| DE         | Driver Enable  | A digital pin (e.g., 3) – used for direction control |
| RE         | Receiver Enable| Connect to the same pin as DE (or GND if always enabled) |
| A          | RS485 A (+)    | To the RS485 bus (twisted pair with B)      |
| B          | RS485 B (-)    | To the RS485 bus                            |
| VCC        | Power          | 5V or 3.3V (check module specifications)    |
| GND        | Ground         | GND                                         |

**Note:** The firmware toggles the `RE_DE_PIN` (default 3) high before transmitting and low after. Connect both DE and RE to this pin for half‑duplex operation.

---

## Installation (PlatformIO)

This project is built with [PlatformIO](https://platformio.org/).  
Make sure you have PlatformIO installed (via the IDE, VSCode extension, or command line).

1. **Clone the repository**  
   ```bash
   git clone https://github.com/ch3p4ll3/rpi-seism-reader.git
   cd rpi-seism-reader
   ```

2. **Configure for your board**  
   Edit `platformio.ini` to select the correct board environment (e.g., `uno`, `pico`, `teensy40`, etc.).  
   Example for an Arduino Uno:

   ```ini
   [env:uno]
   platform = atmelavr
   board = uno
   framework = arduino
   lib_deps = https://github.com/CuriousScientist0/ADS1256.git
   ```

3. **Build and upload**  
   ```bash
   pio run --target upload
   ```

### Manual Installation (Arduino IDE)

If you prefer to use the Arduino IDE:

1. Install the [ADS1256 library](https://github.com/CuriousScientist0/ADS1256) – download the ZIP and add it via **Sketch → Include Library → Add .ZIP Library**.
2. Open the `src/main.cpp` file.
3. Select your board and port.
4. Adjust pin definitions if necessary.
5. Upload.

---

## How It Works

### Packet Format

Each data packet has a fixed size of **16 bytes**:

| Byte offset | Field      | Type     | Description                          |
|-------------|------------|----------|--------------------------------------|
| 0           | header1    | uint8_t  | Always `0xAA`                        |
| 1           | header2    | uint8_t  | Always `0xBB`                        |
| 2–5         | ch0        | int32_t  | Value from ADC channel 0 (EHZ)       |
| 6–9         | ch1        | int32_t  | Value from ADC channel 1 (EHN)       |
| 10–13       | ch2        | int32_t  | Value from ADC channel 2 (EHE)       |
| 14          | checksum   | uint8_t  | XOR of all preceding bytes            |

The checksum is calculated by XOR‑ing every byte **except the checksum byte itself**.

### Streaming Control

- **State `STOP`**: The device waits for a heartbeat from the Raspberry Pi.  
  The built‑in LED blinks slowly to indicate “waiting”.
- **Heartbeat**: The Pi must send a single byte (any value) over RS485 at least once every second.  
  Upon receiving any byte, the device clears its serial buffer, records the time, and switches to `STREAMING`.
- **Timeout**: If no byte is received for 1 second (`TIMEOUT_DURATION`), the device stops sampling and returns to `STOP`.
- **Sampling**: In `STREAMING` mode, the ADC is read at a precise interval (default 10 ms = 100 Hz) using `micros()` timing.  
  Four differential conversions are performed per cycle: three are sent, the fourth is discarded to maintain the correct multiplexer sequence.

### RS485 Direction Control

Before sending a packet, the `RE_DE_PIN` is set **HIGH** (transmit mode). After the packet is fully written and flushed, the pin is set **LOW** (receive mode). This ensures the bus is released for the Pi to send heartbeats.

---

## Configuration Options

You can adjust the following parameters by editing the sketch:

| Parameter          | Line(s)                         | Description                                  |
|--------------------|---------------------------------|----------------------------------------------|
| `SAMPLING_SPEED`   | `#define SAMPLING_SPEED 100`    | Sampling rate in Hz                          |
| `TIMEOUT_DURATION` | `#define TIMEOUT_DURATION 1000` | Milliseconds without heartbeat before stopping |
| `RE_DE_PIN`        | `#define RE_DE_PIN 3`           | GPIO pin connected to MAX485 DE/RE           |
| `PGA`              | `A.setPGA(PGA_64);`             | Programmable Gain Amplifier setting (e.g., `PGA_1`, `PGA_2`, … `PGA_64`) |
| `DRATE`            | `A.setDRATE(DRATE_2000SPS);`    | ADC data rate (must be ≥ sampling rate)      |
| Channel mapping    | `A.setMUX(DIFF_6_7);`           | Differential input pair for each read. The code cycles through three pairs (e.g., 6‑7, 4‑5, 2‑3) – adjust to match your geophone wiring. |

**Important:** The ADS1256’s data rate must be **higher than the desired sampling rate**. A rate of 2000 SPS is safe for 100 Hz output.

---

## Integration with Raspberry Pi

This firmware is designed to work with the [rpi‑seism](https://github.com/ch3p4ll3/rpi-seism) Python application running on a Raspberry Pi.  
The Pi:

- Sends a heartbeat byte every 500 ms over RS485.
- Reads and parses the binary packets.
- Writes MiniSEED files, runs STA/LTA detection, and serves live data via WebSocket.

For full system setup, refer to the [rpi‑seism README](https://github.com/ch3p4ll3/rpi-seism).

---

## Troubleshooting

- **No data received on Pi**  
  - Check wiring, especially the RS485 A/B lines and the direction control pin.  
  - Verify that the Pi is sending heartbeats (you can monitor with an oscilloscope or logic analyzer).  
  - Ensure the baud rate matches (default 250000).  
  - Confirm the ADC is properly initialised (LED blinking pattern can help).

- **LED not blinking**  
  - If the LED is off, the device might be in `STREAMING` mode (no heartbeat timeout).  
  - If the LED is blinking slowly, it is waiting for a heartbeat.

- **Incorrect or noisy readings**  
  - Check the ADS1256 reference voltage (must be stable 2.5V).  
  - Verify the PGA and data rate settings.  
  - Ensure proper shielding and grounding for geophone signals.

- **Compilation errors on your board**  
  - Make sure the correct platform macro is defined (e.g., `ARDUINO_ARCH_RP2040`).  
  - Adjust the SPI pin definitions in the corresponding `#if defined` block.  
  - Verify that the ADS1256 library is correctly installed and compatible.

---

## License

This project is licensed under the **GNU General Public License v3.0**. See the [LICENSE](LICENSE) file for details.

---

## Links

- [Main rpi‑seism project](https://github.com/ch3p4ll3/rpi-seism) – Raspberry Pi data acquisition software.
- [rpi‑seism‑web](https://github.com/ch3p4ll3/rpi-seism-web) – Angular‑based frontend for live waveform display.
- [ADS1256 Arduino library by CuriousScientist0](https://github.com/CuriousScientist0/ADS1256) – used for ADC communication.