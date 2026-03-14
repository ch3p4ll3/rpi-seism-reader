# rpi-seism-reader

Firmware for an Arduino-based 3-channel geophone digitizer that communicates with a Raspberry Pi over RS-422.  
Part of the [rpi-seism](https://github.com/rpi-seism) ecosystem — reads analog signals from a GD-4.5 geophone via an ADS1256 ADC, formats samples into binary packets, and streams them to the Pi over a heartbeat-controlled RS-422 link.

---

## Features

- 3-channel differential sampling — reads EHZ, EHN, EHE via ADS1256
- Runtime-configurable ADC settings — sampling rate, PGA, and data rate are sent by the Pi at startup; no recompilation needed
- Settings handshake — blocks until a valid `SettingsPacket` is received, echoes it back for end-to-end verification
- Heartbeat-based streaming — starts sending data only after receiving a heartbeat byte; stops after 1 second without one
- Jitter-free timing — uses `lastSampleTime += interval` to prevent drift accumulation
- XOR-checksummed 16-byte packet format
- Multi-platform — AVR, RP2040, STM32, ESP32, Teensy

---

## Supported platforms

| Architecture | Boards | SPI macro |
|---|---|---|
| AVR | Uno, Nano (328P), Mega 2560 | Default SPI pins |
| RP2040 | Raspberry Pi Pico, Pico 2 | `USE_SPI1` for SPI1 |
| STM32 | Blue Pill (F103), Black Pill (F411) | `USE_SPI2` for SPI2 |
| ESP32 | ESP32 WROOM, S3, C3 | `USE_HSPI` for HSPI |
| Teensy | Teensy 4.0, 4.1 | `USE_SPI1` / `USE_SPI2` |

---

## Wiring

### ADS1256

| ADS1256 pin | Connect to |
|---|---|
| SCLK | SCK |
| DIN | MOSI |
| DOUT | MISO |
| CS | Digital pin (e.g. 10) |
| DRDY | Digital pin (e.g. 2) |
| RESET | Optional |
| SYNC/PDWN | Optional |

The ADS1256 requires a stable 2.5 V reference. Most modules include a REF5025; if yours does not, provide an external 2.5 V reference.

### RS-422 transceiver (MAX485 / THVD1406)

| Transceiver pin | Connect to |
|---|---|
| DI | TX |
| RO | RX |
| DE + RE | Single GPIO (`RE_DE_PIN`, default pin 3) |
| A (+) | RS-422 bus |
| B (−) | RS-422 bus |

`RE_DE_PIN` is set HIGH before transmitting and LOW after `Serial.flush()` — releases the bus promptly for the Pi's next heartbeat.

---

## Installation

### PlatformIO (recommended)

```bash
git clone https://github.com/rpi-seism/reader
cd reader
```

Upload to your board:

```bash
pio run -e nanoatmega328new --target upload   # Arduino Nano
pio run -e rpipico          --target upload   # Raspberry Pi Pico
pio run -e esp32dev         --target upload   # ESP32
pio run -e blackpill_f411ce --target upload   # STM32 Black Pill
pio run -e teensy40         --target upload   # Teensy 4.0
```

To use an alternate SPI bus, add `build_flags` to the relevant environment in `platformio.ini`:

```ini
[env:rpipico]
...
build_flags = -D USE_SPI1   # use SPI1 instead of SPI0
```

Monitor serial output after upload:

```bash
pio device monitor -e nanoatmega328new
```

### Arduino IDE

1. Install the [ADS1256 library](https://github.com/CuriousScientist0/ADS1256) via **Sketch → Include Library → Add .ZIP Library**
2. Open `src/main.cpp`
3. Select your board and port
4. Upload

---

## Protocol

### Settings handshake

On boot the firmware calls `getSettings()` which blocks until a `SettingsPacket` is received from the Pi.

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0 | `header1` | uint8_t | `0xCC` |
| 1 | `header2` | uint8_t | `0xDD` |
| 2–3 | `samplingSpeed` | uint16_t | Desired output rate in Hz |
| 4 | `ADCGain` | uint8_t | PGA index 0–6 |
| 5 | `ADCDataRate` | uint8_t | Data rate index 0–15 |

Out-of-range values are clamped. The packet is echoed verbatim — the Pi verifies the echo before starting its heartbeat loop.

#### PGA index

| Index | PGA |
|-------|-----|
| 0 | ×1 |
| 1 | ×2 |
| 2 | ×4 |
| 3 | ×8 |
| 4 | ×16 |
| 5 | ×32 |
| 6 | ×64 |

#### Data rate index

| Index | Rate | Index | Rate |
|-------|------|-------|------|
| 0 | 2 SPS | 8 | 100 SPS |
| 1 | 5 SPS | 9 | 500 SPS |
| 2 | 10 SPS | 10 | 1000 SPS |
| 3 | 15 SPS | 11 | 2000 SPS |
| 4 | 25 SPS | 12 | 3750 SPS |
| 5 | 30 SPS | 13 | 7500 SPS |
| 6 | 50 SPS | 14 | 15000 SPS |
| 7 | 60 SPS | 15 | 30000 SPS |

Index 11 (2000 SPS) is the recommended data rate for a 100 Hz output — provides sufficient oversampling margin.

### Data packet

Fixed 16-byte `ADC_Packet` frame sent at the configured output rate:

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0 | `header1` | uint8_t | `0xAA` |
| 1 | `header2` | uint8_t | `0xBB` |
| 2–5 | `ch0` | int32_t | EHZ differential reading |
| 6–9 | `ch1` | int32_t | EHN differential reading |
| 10–13 | `ch2` | int32_t | EHE differential reading |
| 14 | `checksum` | uint8_t | XOR of bytes 0–13 |

### Streaming state machine

- **STOP** — LED blinks at 1 Hz, ADC halted, waiting for heartbeat
- **STREAMING** — ADC sampled at configured rate, packets sent over RS-422

Any byte received from the Pi resets the 1000 ms heartbeat timeout. Missing the timeout returns to STOP.

### MUX cycling

Each cycle calls `cycleDifferential()` four times — the fourth call is discarded but required to advance the ADS1256 multiplexer back to the first channel pair. Removing it causes channels to rotate incorrectly.

---

## Compile-time parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `SAMPLING_SPEED` | `100` | Fallback rate before handshake (Hz) |
| `TIMEOUT_DURATION` | `1000` | Heartbeat timeout (ms) |
| `RE_DE_PIN` | `3` | RS-422 direction control GPIO |

---

## Troubleshooting

**Stuck in `getSettings()`** — the Pi must send the `SettingsPacket` first. Verify RS-422 wiring and that the daemon's `Reader` thread is running.

**No data after handshake** — check A/B line polarity and `RE_DE_PIN` wiring. Verify the Pi is sending heartbeats. Both sides must be at 250 000 baud.

**LED not blinking** — device is likely stuck waiting for the settings handshake (solid off). If blinking at 1 Hz, it received the handshake but is not getting heartbeats.

**Noisy ADC readings** — verify the 2.5 V reference is stable. Check shielding and grounding on geophone cables.

**Channels rotating** — the fourth `cycleDifferential()` (discard) call has been removed. Restore it.

**Compilation errors** — confirm the correct environment is selected in `platformio.ini`. Adjust SPI pin definitions in the matching `#if defined` block.

---

## Related repositories

| Repository | Description |
|---|---|
| [rpi-seism/daemon](https://github.com/rpi-seism/daemon) | Python acquisition daemon |
| [rpi-seism/api](https://github.com/rpi-seism/api) | FastAPI archive browser |
| [rpi-seism/web](https://github.com/rpi-seism/web) | Angular frontend |
| [rpi-seism/stack](https://github.com/rpi-seism/stack) | Docker Compose deployment |

---

## License

[GNU General Public License v3.0](LICENSE)