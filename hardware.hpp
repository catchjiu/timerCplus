/**
 * BJJ Gym Timer - Hardware Configuration (lgpio for Pi 5)
 * Raspberry Pi GPIO - Passive buzzer + Rotary encoder
 * 
 * Uses lgpio library - works on Pi 5 (gpiochip4) and Pi 4/older (gpiochip0)
 * Install: sudo apt install liblgpio-dev
 */

#pragma once

#include <cstdint>
#include <lgpio.h>

namespace bjj {

// ============================================================================
// GPIO CHIP - Pi 5: gpiochip4. Pi 4/Zero 2: gpiochip0 (auto-detected in main)
// ============================================================================

// ============================================================================
// BUZZER CONFIGURATION
// ============================================================================
// Passive Buzzer: Signal->BUZZER_PIN, Other lead->GND
constexpr unsigned BUZZER_PIN = 18;

// Tone frequencies (Hz) - BJJ Style
namespace Tones {
    constexpr unsigned AIR_HORN_HIGH = 1200;   // Start round - high intensity
    constexpr unsigned WARNING_LOW   = 400;    // 10 sec warning - urgent
    constexpr unsigned END_BUZZER    = 800;    // End round/rest - sustained
    constexpr unsigned SWITCH_CHIRP  = 1000;   // Drilling switch - attention
}

// ============================================================================
// ROTARY ENCODER CONFIGURATION (BCM GPIO)
// ============================================================================
// KY-040: CLK->GPIO11, DT->GPIO12, SW->GPIO13, VCC->3.3V, GND->GND
constexpr unsigned ENCODER_CLK = 11;
constexpr unsigned ENCODER_DT  = 12;
constexpr unsigned ENCODER_SW  = 13;
constexpr unsigned LONG_PRESS_MS = 2000;

// ============================================================================
// BUZZER DRIVER
// ============================================================================
class Buzzer {
public:
    Buzzer() = default;
    
    void init(int h) {
        lgGpioClaimOutput(h, 0, BUZZER_PIN, 0);
    }
    
    void tone(int h, unsigned freq_hz, unsigned duration_ms) {
        if (freq_hz == 0) {
            lgTxPwm(h, BUZZER_PIN, 0, 0, 0, 0);
            return;
        }
        lgTxPwm(h, BUZZER_PIN, static_cast<float>(freq_hz), 50.0f, 0, 0);
        lguSleep(static_cast<double>(duration_ms) / 1000.0);
        lgTxPwm(h, BUZZER_PIN, 0, 0, 0, 0);
    }
    
    void silence(int h) {
        lgTxPwm(h, BUZZER_PIN, 0, 0, 0, 0);
    }
    
    void playStartRound(int h) {
        tone(h, Tones::AIR_HORN_HIGH, 400);
        lguSleep(0.15);
        tone(h, Tones::AIR_HORN_HIGH, 400);
    }
    
    void play10SecondWarning(int h) {
        for (int i = 0; i < 3; ++i) {
            tone(h, Tones::WARNING_LOW, 120);
            lguSleep(0.12);
        }
    }
    
    void playEndRound(int h) {
        tone(h, Tones::END_BUZZER, 2000);
    }
    
    void playDrillingSwitch(int h) {
        tone(h, Tones::SWITCH_CHIRP, 80);
        lguSleep(0.06);
        tone(h, Tones::SWITCH_CHIRP, 80);
    }
};

// ============================================================================
// ROTARY ENCODER DRIVER (Alert-based callbacks)
// ============================================================================
class RotaryEncoder {
public:
    using RotateCallback = void (*)(int delta);
    using PressCallback = void (*)(bool isLong);
    
    RotaryEncoder(RotateCallback onRotate, PressCallback onPress)
        : rotateCb_(onRotate), pressCb_(onPress) {}
    
    void init(int h) {
        handle_ = h;
        lgGpioClaimInput(h, LG_SET_PULL_UP, ENCODER_DT);
        lgGpioClaimAlert(h, LG_SET_PULL_UP, LG_BOTH_EDGES, ENCODER_CLK, -1);
        lgGpioClaimAlert(h, LG_SET_PULL_UP, LG_BOTH_EDGES, ENCODER_SW, -1);
        lgGpioSetDebounce(h, ENCODER_CLK, 1000);
        lgGpioSetDebounce(h, ENCODER_DT, 1000);
        lgGpioSetDebounce(h, ENCODER_SW, 50000);
        lastClk_ = lgGpioRead(h, ENCODER_CLK) != 0 ? 1 : 0;
        lastDt_  = lgGpioRead(h, ENCODER_DT) != 0 ? 1 : 0;
    }
    
    void onAlert(int num_alerts, lgGpioAlert_p alerts, void* /*userdata*/) {
        for (int i = 0; i < num_alerts && alerts; ++i) {
            unsigned gpio = alerts[i].report.gpio;
            int level = alerts[i].report.level;
            uint64_t tick = alerts[i].report.timestamp;
            
            if (gpio == ENCODER_CLK) {
                int clk = (level == 1) ? 1 : 0;
                int dt = lgGpioRead(handle_, ENCODER_DT);
                if (dt < 0) dt = 0;
                if (clk != lastClk_) {
                    int delta = (clk == dt) ? 1 : -1;
                    lastClk_ = clk;
                    lastDt_ = dt;
                    if (rotateCb_) rotateCb_(delta);
                }
            } else if (gpio == ENCODER_SW) {
                if (level == 0) {
                    pressStartTick_ = tick;
                    swPressed_ = true;
                } else if (swPressed_) {
                    swPressed_ = false;
                    uint64_t duration_ns = tick - pressStartTick_;
                    bool longPress = (duration_ns >= static_cast<uint64_t>(LONG_PRESS_MS) * 1000000ULL);
                    if (pressCb_) pressCb_(longPress);
                }
            }
        }
    }
    
    static RotaryEncoder* instance_;
    static void alertFunc(int e, lgGpioAlert_p evt, void* data) {
        if (e >= 0 && instance_ && evt) {
            instance_->onAlert(e, evt, data);
        }
    }
    
    void attachInterrupts() {
        instance_ = this;
        lgGpioSetAlertsFunc(handle_, ENCODER_CLK, alertFunc, nullptr);
        lgGpioSetAlertsFunc(handle_, ENCODER_SW, alertFunc, nullptr);
    }
    
    void detachInterrupts() {
        lgGpioSetAlertsFunc(handle_, ENCODER_CLK, nullptr, nullptr);
        lgGpioSetAlertsFunc(handle_, ENCODER_SW, nullptr, nullptr);
        instance_ = nullptr;
    }
    
    void freeGpio(int h) {
        lgGpioFree(h, ENCODER_CLK);
        lgGpioFree(h, ENCODER_DT);
        lgGpioFree(h, ENCODER_SW);
    }
    
private:
    int handle_{0};
    RotateCallback rotateCb_;
    PressCallback pressCb_;
    int lastClk_{1}, lastDt_{1};
    uint64_t pressStartTick_{0};
    bool swPressed_{false};
};

inline RotaryEncoder* RotaryEncoder::instance_ = nullptr;

} // namespace bjj
