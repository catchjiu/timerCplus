/**
 * BJJ Gym Timer - Hardware Configuration
 * Raspberry Pi GPIO definitions for passive buzzer and rotary encoder
 * 
 * COMPILE: Requires pigpio daemon running (sudo pigpiod)
 * WIRING: See comments for each component
 */

#pragma once

#include <cstdint>
#include <pigpio.h>

namespace bjj {

// ============================================================================
// BUZZER CONFIGURATION
// ============================================================================
// Passive Buzzer: Signal->BUZZER_PIN, Other lead->GND (e.g. Physical Pin 14)
// GPIO 18 = Physical Pin 12 (Hardware PWM0 - recommended for clean tones)
// For pins 2/14/16: Use GPIO 18 (Phy.12) for PWM; wire VCC to 3.3V, GND to Phy.6
constexpr unsigned BUZZER_PIN = 18;
constexpr unsigned PWM_DUTY_CYCLE = 500000;  // 50% for square wave (0-1000000)

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
// KY-040 or similar: CLK->GPIO11, DT->GPIO12, SW->GPIO13, VCC->3.3V, GND->GND
constexpr unsigned ENCODER_CLK = 11;  // Clock pin (Phase A)
constexpr unsigned ENCODER_DT  = 12;  // Data pin (Phase B)
constexpr unsigned ENCODER_SW  = 13;  // Switch/Button
constexpr unsigned LONG_PRESS_MS = 2000;  // 2+ seconds for reset

// ============================================================================
// BUZZER DRIVER
// ============================================================================
class Buzzer {
public:
    Buzzer() = default;
    
    void init() {
        gpioSetMode(BUZZER_PIN, PI_OUTPUT);
        gpioWrite(BUZZER_PIN, 0);  // Start silent
    }
    
    void tone(unsigned freq_hz, unsigned duration_ms) {
        if (freq_hz == 0) {
            gpioHardwarePWM(BUZZER_PIN, 0, 0);
            return;
        }
        gpioHardwarePWM(BUZZER_PIN, freq_hz, PWM_DUTY_CYCLE);
        gpioDelay(duration_ms * 1000);  // gpioDelay uses microseconds
        gpioHardwarePWM(BUZZER_PIN, 0, 0);
    }
    
    void silence() {
        gpioHardwarePWM(BUZZER_PIN, 0, 0);
    }
    
    // --- BJJ-Specific Audio Feedback ---
    
    // START ROUND: Two long, high-frequency "Air Horn" style pulses
    void playStartRound() {
        tone(Tones::AIR_HORN_HIGH, 400);
        gpioDelay(150000);  // 150ms gap
        tone(Tones::AIR_HORN_HIGH, 400);
    }
    
    // 10 SECONDS LEFT: Three short, low-frequency warning beeps
    void play10SecondWarning() {
        for (int i = 0; i < 3; ++i) {
            tone(Tones::WARNING_LOW, 120);
            gpioDelay(120000);
        }
    }
    
    // END ROUND/REST: One long, continuous 2-second buzzer
    void playEndRound() {
        tone(Tones::END_BUZZER, 2000);
    }
    
    // DRILLING SWITCH: Rapid double-chirp
    void playDrillingSwitch() {
        tone(Tones::SWITCH_CHIRP, 80);
        gpioDelay(60000);
        tone(Tones::SWITCH_CHIRP, 80);
    }
};

// ============================================================================
// ROTARY ENCODER DRIVER (Interrupt-based)
// ============================================================================
class RotaryEncoder {
public:
    using RotateCallback = void (*)(int delta);   // +1 CW, -1 CCW
    using PressCallback = void (*)(bool isLong);
    
    RotaryEncoder(RotateCallback onRotate, PressCallback onPress)
        : rotateCb_(onRotate), pressCb_(onPress) {}
    
    void init() {
        gpioSetMode(ENCODER_CLK, PI_INPUT);
        gpioSetMode(ENCODER_DT, PI_INPUT);
        gpioSetMode(ENCODER_SW, PI_INPUT);
        
        gpioSetPullUpDown(ENCODER_CLK, PI_PUD_UP);
        gpioSetPullUpDown(ENCODER_DT, PI_PUD_UP);
        gpioSetPullUpDown(ENCODER_SW, PI_PUD_UP);
        
        gpioGlitchFilter(ENCODER_CLK, 1000);  // 1ms debounce
        gpioGlitchFilter(ENCODER_DT, 1000);
        gpioGlitchFilter(ENCODER_SW, 50000);  // 50ms for button
        
        lastClk_ = gpioRead(ENCODER_CLK);
        lastDt_  = gpioRead(ENCODER_DT);
    }
    
    // Called from ISR context - keep minimal, update state only
    void onClkChange(int gpio, int level, uint32_t tick) {
        int clk = gpioRead(ENCODER_CLK);
        int dt  = gpioRead(ENCODER_DT);
        if (clk != lastClk_) {
            int delta = (clk == dt) ? 1 : -1;
            lastClk_ = clk;
            lastDt_  = dt;
            if (rotateCb_) rotateCb_(delta);
        }
    }
    
    void onSwChange(int gpio, int level, uint32_t tick) {
        if (level == 0) {  // Pressed (active low with pull-up)
            pressStartTick_ = tick;
            swPressed_ = true;
        } else if (swPressed_) {  // Released
            swPressed_ = false;
            uint32_t duration = tick - pressStartTick_;
            bool longPress = (duration >= LONG_PRESS_MS * 1000);
            if (pressCb_) pressCb_(longPress);
        }
    }
    
    static RotaryEncoder* instance_;
    static void isrClk(int gpio, int level, uint32_t tick) {
        if (instance_) instance_->onClkChange(gpio, level, tick);
    }
    static void isrSw(int gpio, int level, uint32_t tick) {
        if (instance_) instance_->onSwChange(gpio, level, tick);
    }
    
    void attachInterrupts() {
        instance_ = this;
        gpioSetISRFunc(ENCODER_CLK, EITHER_EDGE, 0, isrClk);
        gpioSetISRFunc(ENCODER_SW, EITHER_EDGE, 0, isrSw);
    }
    
    void detachInterrupts() {
        gpioSetISRFunc(ENCODER_CLK, EITHER_EDGE, 0, nullptr);
        gpioSetISRFunc(ENCODER_SW, EITHER_EDGE, 0, nullptr);
        instance_ = nullptr;
    }
    
private:
    RotateCallback rotateCb_;
    PressCallback pressCb_;
    int lastClk_{1}, lastDt_{1};
    uint32_t pressStartTick_{0};
    bool swPressed_{false};
};

inline RotaryEncoder* RotaryEncoder::instance_ = nullptr;

} // namespace bjj
