/**
 * BJJ Gym Timer - Hardware Configuration (lgpio for Pi 5)
 * Raspberry Pi GPIO - Passive buzzer + Rotary encoder
 * 
 * Uses lgpio library - works on Pi 5 (gpiochip4) and Pi 4/older (gpiochip0)
 * Install: sudo apt install liblgpio-dev
 */

#pragma once

#include <cstdint>
#include <ctime>
#include <lgpio.h>

namespace bjj {

// ============================================================================
// GPIO CHIP - Pi 5: gpiochip4. Pi 4/Zero 2: gpiochip0 (auto-detected in main)
// ============================================================================

// ============================================================================
// BUZZER CONFIGURATION
// ============================================================================
// Passive Buzzer: Signal->Phy.16, GND->Phy.14, (Phy.2=5V optional)
constexpr unsigned BUZZER_PIN = 23;  // Physical pin 16

// Tone frequencies (Hz) - BJJ Style
namespace Tones {
    constexpr unsigned AIR_HORN_HIGH = 1200;   // Start round - high intensity
    constexpr unsigned WARNING_LOW   = 400;    // 10 sec warning - urgent
    constexpr unsigned END_BUZZER    = 800;    // End round/rest - sustained
    constexpr unsigned SWITCH_CHIRP  = 1000;   // Drilling switch - attention
}

// ============================================================================
// ROTARY ENCODER CONFIGURATION (Physical pins 11, 12, 13)
// ============================================================================
// KY-040: CLK->Phy.11, DT->Phy.12, SW->Phy.13 | VCC->Phy.1, GND->Phy.6
constexpr unsigned ENCODER_CLK = 17;  // Physical pin 11
constexpr unsigned ENCODER_DT  = 18;  // Physical pin 12
constexpr unsigned ENCODER_SW  = 27;  // Physical pin 13
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
// ROTARY ENCODER DRIVER (Polling - reliable on Pi 5)
// ============================================================================
class RotaryEncoder {
public:
    using RotateCallback = void (*)(int delta);
    using PressCallback = void (*)(bool isLong);
    
    RotaryEncoder(RotateCallback onRotate, PressCallback onPress)
        : rotateCb_(onRotate), pressCb_(onPress) {}
    
    void init(int h) {
        handle_ = h;
        lgGpioClaimInput(h, LG_SET_PULL_UP, ENCODER_CLK);
        lgGpioClaimInput(h, LG_SET_PULL_UP, ENCODER_DT);
        lgGpioClaimInput(h, LG_SET_PULL_UP, ENCODER_SW);
        lastClk_ = readPin(ENCODER_CLK);
        lastDt_  = readPin(ENCODER_DT);
        lastSw_  = readPin(ENCODER_SW);
    }
    
    // Call every main loop iteration - polls encoder and button
    void poll() {
        int clk = readPin(ENCODER_CLK);
        int dt  = readPin(ENCODER_DT);
        int sw  = readPin(ENCODER_SW);
        
        // Quadrature decode on CLK edges (inverted: CW=+1, CCW=-1)
        if (clk != lastClk_) {
            int delta = (clk == dt) ? -1 : 1;
            lastClk_ = clk;
            lastDt_ = dt;
            if (rotateCb_) rotateCb_(delta);
        } else {
            lastDt_ = dt;
        }
        
        // Button: 0=pressed (active low), 1=released
        if (sw == 0 && !swPressed_) {
            swPressed_ = true;
            pressStartMs_ = nowMs();
        } else if (sw == 1 && swPressed_) {
            swPressed_ = false;
            unsigned duration = nowMs() - pressStartMs_;
            bool longPress = (duration >= LONG_PRESS_MS);
            if (pressCb_) pressCb_(longPress);
        }
    }
    
    void attachInterrupts() {}   // No-op for polling
    void detachInterrupts() {}
    
    void freeGpio(int h) {
        lgGpioFree(h, ENCODER_CLK);
        lgGpioFree(h, ENCODER_DT);
        lgGpioFree(h, ENCODER_SW);
    }
    
private:
    int readPin(unsigned gpio) {
        int v = lgGpioRead(handle_, gpio);
        return (v > 0) ? 1 : 0;
    }
    static unsigned nowMs() {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return static_cast<unsigned>(ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000);
    }
    
    int handle_{0};
    RotateCallback rotateCb_;
    PressCallback pressCb_;
    int lastClk_{1}, lastDt_{1}, lastSw_{1};
    unsigned pressStartMs_{0};
    bool swPressed_{false};
};

} // namespace bjj
