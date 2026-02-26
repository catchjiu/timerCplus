# BJJ Gym Timer for Raspberry Pi

Professional Brazilian Jiu-Jitsu gym timer with rotary encoder control and passive buzzer audio feedback.

## Hardware Requirements

- **Raspberry Pi** (3/4/Zero 2 recommended)
- **Passive Buzzer** → GPIO 18 (Physical Pin 12), GND
- **Rotary Encoder** (KY-040): CLK→GPIO11, DT→GPIO12, SW→GPIO13, VCC→3.3V, GND→GND

## Build

```bash
sudo apt install pigpio  # If not already installed
make
```

## Run

```bash
sudo pigpiod   # Start pigpio daemon (once per boot)
sudo ./bjj_timer
```

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
