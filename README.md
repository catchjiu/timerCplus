# BJJ Gym Timer for Raspberry Pi

Professional Brazilian Jiu-Jitsu gym timer with rotary encoder control and passive buzzer audio feedback. **Works on Raspberry Pi 5** (and Pi 4/older) using the lgpio library.

## Hardware Requirements

- **Raspberry Pi** 5, 4, 3, or Zero 2
- **Passive Buzzer** → GPIO 18 (Physical Pin 12), GND
- **Rotary Encoder** (KY-040): CLK→GPIO11, DT→GPIO12, SW→GPIO13, VCC→3.3V, GND→GND

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
