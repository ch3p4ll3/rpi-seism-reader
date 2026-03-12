# RPI-SEISM Reader

Firmware for an Arduino‑based 3‑channel geophone digitizer that communicates with a Raspberry Pi over RS485.  
This project is part of the [rpi‑seism](https://github.com/ch3p4ll3/rpi-seism) ecosystem – it reads analog signals from a geophone via an ADS1256 ADC, formats the data into binary packets, and streams them to the Raspberry Pi when a heartbeat signal is received.

---

## Features

- **3‑channel differential sampling** – reads three geophone components (EHZ, EHN, EHE) using an ADS1256 ADC.
- **Runtime-configurable ADC settings** – sampling rate, PGA, and data rate are received from the Raspberry Pi at startup via a binary settings frame; no recompilation needed to change them.
- **RS485 communication** – half‑duplex at 250 000 baud with automatic direction control via a GPIO pin.
- **Settings handshake** – waits for a valid `SettingsPacket` from the Pi before initialising the ADC or entering the main loop; echoes the packet back for end-to-end verification.
- **Heartbeat‑based streaming** – starts sending data only after receiving a heartbeat byte from the Pi; stops after 1 second without a heartbeat.
- **Jitter-free timing** – uses `lastSampleTime += interval` (not `= micros()`) to prevent drift accumulation across sampling cycles.
- **Robust packet format** – includes a header, three 32‑bit signed integers, and an XOR checksum.
- **Multi‑platform support** – tested on AVR (Uno/Nano), RP2040 (Pico, Zero, Waveshare), STM32 (Blue Pill), ESP32, and Teensy 4.0.
- **Configurable SPI pins** – easily adapt to different board layouts via compile-time macros.

---

## Hardware Requirements

- **Arduino‑compatible board** (see [Supported Platforms](#supported-platforms))
- **ADS1256 ADC module** (8‑channel, 24‑bit)
- **MAX485** or equivalent RS485 transceiver
- **3‑component geophone** (e.g., 4.5 Hz) with appropriate signal conditioning (if needed)
- **Wires and breadboard** for connections

### Supported Platforms

The firmware includes conditional compilation for several architectures.  
Uncomment the appropriate macros in the code to match your board.

| Architecture | Boards tested                            | SPI selection macros                        |
|---|---|---|
| AVR          | Arduino Uno, Nano                        | Default (uses standard SPI pins)            |
| RP2040       | Raspberry Pi Pico, Zero, Waveshare Pico  | `USE_SPI1` to switch between SPI0/SPI1      |
| STM32        | STM32F103 "Blue Pill"                    | `USE_SPI2` to use SPI2 instead of SPI1      |
| ESP32        | ESP32 WROOM                              | `USE_HSPI` to use HSPI instead of VSPI      |
| Teensy       | Teensy 4.0, 4.1                          | `USE_SPI1` or `USE_SPI2` for alternate ports|

---

## Wiring

### 1. ADS1256 to Arduino

The ADS1256 communicates via SPI. Below is a typical connection for an Arduino Uno — **check your board's pinout and the `#if defined` blocks in the code for platform-specific pin assignments**.

| ADS1256 pin | Function          | Connect to Arduino                      |
|---|---|---|
| SCLK        | SPI Clock         | SCK pin                                 |
| DIN         | SPI MOSI          | MOSI pin                                |
| DOUT        | SPI MISO          | MISO pin                                |
| CS          | Chip Select       | Digital pin (e.g., 10 on Uno)           |
| DRDY        | Data Ready        | Digital pin (e.g., 2 on Uno)            |
| RESET       | Reset             | Optional – can be left unconnected      |
| SYNC/PDWN   | Sync/Power Down   | Optional – can be left unconnected      |

**Important:** The ADS1256 requires a stable 2.5 V reference. Many modules include a REF5025 or similar; if yours does not, you must provide an external 2.5 V reference.

### 2. MAX485 (RS485) to Arduino

| MAX485 pin | Function        | Connect to Arduino                          |
|---|---|---|
| DI         | Driver Input    | TX pin                                      |
| RO         | Receiver Output | RX pin                                      |
| DE         | Driver Enable   | Digital pin (e.g., 3) – direction control   |
| RE         | Receiver Enable | Same pin as DE (tied together)              |
| A          | RS485 A (+)     | RS485 bus (twisted pair with B)             |
| B          | RS485 B (-)     | RS485 bus                                   |
| VCC        | Power           | 5V or 3.3V (check module specs)             |
| GND        | Ground          | GND                                         |

The firmware toggles `RE_DE_PIN` (default 3) HIGH before transmitting and LOW after. Connect both DE and RE to this single pin for half‑duplex operation.

---

## Installation (PlatformIO)

This project is built with [PlatformIO](https://platformio.org/).

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

1. Install the [ADS1256 library](https://github.com/CuriousScientist0/ADS1256) via **Sketch → Include Library → Add .ZIP Library**.
2. Open `src/main.cpp`.
3. Select your board and port.
4. Adjust the `ADS1256 A(...)` constructor call to match your wiring (see examples in the source).
5. Upload.

---

## How It Works

### Startup: Settings Handshake

Before entering the main loop the firmware calls `getSettings()`, which **blocks indefinitely** until a valid `SettingsPacket` is received from the Raspberry Pi.

The `SettingsPacket` binary frame is:

| Byte offset | Field          | Type     | Description                                         |
|---|---|---|---|
| 0           | `header1`      | uint8_t  | Always `0xCC`                                       |
| 1           | `header2`      | uint8_t  | Always `0xDD`                                       |
| 2–3         | `samplingSpeed`| uint16_t | Desired sampling rate in Hz (e.g., 100)             |
| 4           | `ADCGain`      | uint8_t  | PGA index (0–6, see table below)                    |
| 5           | `ADCDataRate`  | uint8_t  | Data rate index (0–15, see table below)             |

On receipt, `validateSettings()` clamps out-of-range indices (`ADCGain` to 6, `ADCDataRate` to 11 if > 15), recomputes the sampling interval, and initialises the ADS1256 with the validated values. The packet is then echoed back verbatim — the Pi verifies the echo before starting its own streaming loop.

#### PGA index table (`ADCGain`)

| Index | PGA setting |
|---|---|
| 0 | PGA_1  |
| 1 | PGA_2  |
| 2 | PGA_4  |
| 3 | PGA_8  |
| 4 | PGA_16 |
| 5 | PGA_32 |
| 6 | PGA_64 |

#### Data rate index table (`ADCDataRate`)

| Index | Data rate  | Index | Data rate    |
|---|---|---|---|
| 0  | 2 SPS      | 8  | 100 SPS   |
| 1  | 5 SPS      | 9  | 500 SPS   |
| 2  | 10 SPS     | 10 | 1000 SPS  |
| 3  | 15 SPS     | 11 | 2000 SPS  |
| 4  | 25 SPS     | 12 | 3750 SPS  |
| 5  | 30 SPS     | 13 | 7500 SPS  |
| 6  | 50 SPS     | 14 | 15000 SPS |
| 7  | 60 SPS     | 15 | 30000 SPS |

The ADS1256 data rate must be **higher than the desired sampling rate**. Index 11 (2000 SPS) is safe for a 100 Hz output rate.

---

### Data Packet Format

Each streaming data packet is a fixed **16-byte** `ADC_Packet` frame:

| Byte offset | Field      | Type    | Description                                  |
|---|---|---|---|
| 0           | `header1`  | uint8_t | Always `0xAA`                                |
| 1           | `header2`  | uint8_t | Always `0xBB`                                |
| 2–5         | `ch0`      | int32_t | Differential reading from ADC channel 0 (EHZ)|
| 6–9         | `ch1`      | int32_t | Differential reading from ADC channel 1 (EHN)|
| 10–13       | `ch2`      | int32_t | Differential reading from ADC channel 2 (EHE)|
| 14          | `checksum` | uint8_t | XOR of all preceding bytes (bytes 0–13)      |

The checksum is calculated by XOR-ing every byte **except the checksum byte itself**.

---

### Streaming Control

The main loop implements a simple two-state machine:

- **`STOP`**: The firmware waits for a heartbeat from the Pi. The built-in LED blinks at 1 Hz to indicate "waiting". `A.stopConversion()` is called to halt the ADC.
- **`STREAMING`**: The ADC is sampled at the configured rate and packets are sent over RS485.

**Heartbeat**: The Pi must send any byte over RS485 at least once per second (`TIMEOUT_DURATION = 1000 ms`). On receipt, the serial buffer is drained, `lastHeartbeat` is updated, and the state transitions to `STREAMING` if it was in `STOP`.

**Timeout**: If no byte is received within `TIMEOUT_DURATION` milliseconds, the state returns to `STOP` and the ADC is halted.

---

### Sampling Timing

Samples are triggered using `micros()`. The interval is pre-computed as `1 000 000 / samplingSpeed` microseconds.

Crucially, the next deadline is set with:
```cpp
lastSampleTime += interval;
```
rather than `lastSampleTime = micros()`. This prevents jitter from accumulating — any processing delay in one cycle does not push back subsequent deadlines.

---

### ADC Multiplexer Cycling

Each sampling cycle calls `A.cycleDifferential()` **four** times, but only the first three values are placed in the packet:

```cpp
frame.ch0 = A.cycleDifferential();  // EHZ
frame.ch1 = A.cycleDifferential();  // EHN
frame.ch2 = A.cycleDifferential();  // EHE
A.cycleDifferential();              // discarded — advances MUX back to the first pair
```

The fourth call is required to keep the ADS1256 multiplexer in sync for the next cycle. Skipping it would cause channel rotation to drift.

---

### RS485 Direction Control

Before writing each packet, `RE_DE_PIN` is set **HIGH** (transmit mode). After `Serial.flush()` confirms all bytes have left the UART, the pin is set **LOW** (receive mode). This ensures the bus is released promptly so the Pi can send its next heartbeat without collision.

---

## Configuration Options

The following parameters can be adjusted at compile time by editing the sketch. ADC gain and data rate are additionally overridable at runtime via the settings handshake.

| Parameter          | Default | Description                                          |
|---|---|---|
| `SAMPLING_SPEED`   | `100`   | Fallback sampling rate in Hz (used before handshake) |
| `TIMEOUT_DURATION` | `1000`  | Milliseconds without heartbeat before stopping       |
| `RE_DE_PIN`        | `3`     | GPIO pin connected to MAX485 DE/RE                   |
| `ADS1256 A(...)`   | —       | Constructor pins: DRDY, RESET, SYNC, CS, VREF, &SPI  |

Commented-out constructor examples for all supported boards are provided in `main.cpp`.

---

## Integration with Raspberry Pi

This firmware is designed to work with the [rpi‑seism](https://github.com/ch3p4ll3/rpi-seism) Python application. The Pi:

1. Sends a `SettingsPacket` (`0xCC 0xDD` header) at startup and verifies the echo before proceeding.
2. Sends a heartbeat byte (`0x01`) every 500 ms over RS485 to keep the Arduino streaming.
3. Reads and parses the 16-byte `ADC_Packet` frames, verifies the XOR checksum, and distributes samples to its internal processing threads.
4. Writes a SeisComp-compatible MiniSEED archive, runs STA/LTA earthquake detection, and serves live waveforms over WebSocket.

For full system setup, refer to the [rpi‑seism README](https://github.com/ch3p4ll3/rpi-seism).

---

## Troubleshooting

- **Firmware stuck in `getSettings()`** — the Pi must send the `SettingsPacket` before the Arduino will do anything else. Verify the RS485 wiring and that the Pi-side `Reader` thread is starting and transmitting the settings frame correctly.

- **No data received on Pi after handshake** — check the RS485 A/B line polarity and the direction control pin. Verify the Pi is sending heartbeats (a logic analyser on the A/B lines helps). Ensure baud rate is 250 000 on both sides.

- **LED not blinking / device appears frozen** — if the LED is solid off, the device is likely stuck waiting for the settings handshake. If it blinks slowly (1 Hz), it is in `STOP` waiting for a heartbeat.

- **Incorrect or noisy ADC readings** — check the ADS1256 reference voltage (must be a stable 2.5 V). Verify the PGA index sent by the Pi matches expectations. Ensure proper shielding and grounding for geophone cables.

- **Channel values rotating unexpectedly** — ensure the fourth `cycleDifferential()` call (the discard step) is not removed. Without it the MUX sequence drifts and channels swap after each cycle.

- **Compilation errors for your board** — confirm the correct platform macro is defined in `platformio.ini` or the Arduino IDE board selector. Adjust the SPI pin definitions in the matching `#if defined` block. Verify the ADS1256 library is installed and up to date.

---

## License

[GNU General Public License v3.0](LICENSE)

---

## Links

- [Main rpi‑seism project](https://github.com/ch3p4ll3/rpi-seism) – Raspberry Pi data acquisition software
- [rpi‑seism‑web](https://github.com/ch3p4ll3/rpi-seism-web) – Angular‑based frontend for live waveform display
- [ADS1256 Arduino library by CuriousScientist0](https://github.com/CuriousScientist0/ADS1256) – used for ADC communication