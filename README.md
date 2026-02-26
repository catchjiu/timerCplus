# BJJ Gym Timer for Raspberry Pi

Professional Brazilian Jiu-Jitsu gym timer with rotary encoder control and passive buzzer audio feedback. **Works on Raspberry Pi 5** (and Pi 4/older) using the lgpio library.

## Hardware Requirements

- **Raspberry Pi** 5, 4, 3, or Zero 2
- **Passive Buzzer** → GPIO 22 (Physical Pin 15), GND
- **Rotary Encoder** (KY-040):
  - CLK → Physical 11 (GPIO 17)
  - DT  → Physical 12 (GPIO 18)
  - SW  → Physical 13 (GPIO 27)
  - VCC → Physical 1 (3.3V)
  - GND → Physical 6

## Build

```bash
sudo apt install liblgpio-dev
make
```

## Run

```bash
sudo ./bjj_timer
```

No daemon required—lgpio runs directly.

## Modes

| Mode | Description |
|------|-------------|
| **SPARRING** | Customizable round time, rest time, round count |
| **DRILLING** | Interval timer (e.g., 2 min per person) with "Switch!" chirp |
| **COMPETITION** | 5, 6, 8, or 10-minute straight countdown |

## Controls

- **Rotate**: Select menu / Adjust settings (15s) / Running: ±30s
- **Short Press**: Confirm / Pause / Resume
- **Long Press** (2+ sec): Reset to Main Menu

## Audio Cues

- **Start**: Two air-horn pulses
- **10 sec warning**: Three low beeps
- **End round/rest**: 2-second buzzer
- **Drilling switch**: Rapid double-chirp
