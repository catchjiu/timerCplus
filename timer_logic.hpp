/**
 * BJJ Gym Timer - Timer Logic & State Machine
 * Handles modes, state transitions, and timing calculations
 */

#pragma once

#include <cstdint>
#include <atomic>
#include <functional>
#include <string>

namespace bjj {

// ============================================================================
// ENUMS & CONSTANTS
// ============================================================================
enum class TimerMode : uint8_t {
    SPARRING,    // Rolling: rounds + rest
    DRILLING,    // Interval with switch
    COMPETITION  // 5/6/8/10 min straight
};

enum class TimerState : uint8_t {
    MENU,           // Selecting mode
    SETUP_WORK,     // Configuring work/round time
    SETUP_REST,     // Configuring rest time (Sparring only)
    SETUP_ROUNDS,   // Configuring round count (Sparring) or interval (Drilling)
    RUNNING,        // Timer active
    PAUSED,         // Timer paused
    FINISHED        // Session complete
};

enum class Phase : uint8_t {
    WORK,    // Round/Sparring time
    REST,    // Rest between rounds
    SWITCH   // Drilling partner switch
};

// Competition time options (seconds)
constexpr unsigned COMPETITION_TIMES[] = {300, 360, 480, 600};  // 5, 6, 8, 10 min
constexpr unsigned COMPETITION_COUNT = 4;
constexpr unsigned DEFAULT_WORK_SEC  = 300;   // 5 min
constexpr unsigned DEFAULT_REST_SEC  = 60;    // 1 min
constexpr unsigned DEFAULT_ROUNDS    = 5;
constexpr unsigned ROUND_INCREMENT   = 15;    // 15s for setup
constexpr unsigned RUNTIME_ADJUST    = 30;    // 30s when running
constexpr unsigned TEN_SECOND_MARK   = 10;

// ============================================================================
// TIMER CONFIGURATION
// ============================================================================
struct TimerConfig {
    unsigned workSeconds{DEFAULT_WORK_SEC};
    unsigned restSeconds{DEFAULT_REST_SEC};
    unsigned roundCount{DEFAULT_ROUNDS};
    unsigned compTimeIndex{0};  // 0=5min, 1=6min, 2=8min, 3=10min
};

// ============================================================================
// DISPLAY INFO (what UI should show)
// ============================================================================
struct DisplayInfo {
    TimerState state{TimerState::MENU};
    TimerMode mode{TimerMode::SPARRING};
    Phase phase{Phase::WORK};
    
    unsigned currentRound{0};
    unsigned totalRounds{0};
    unsigned secondsRemaining{0};
    
    std::string menuLabel;
    std::string valueLabel;
    unsigned setupValue{0};
    
    bool tenSecondWarningDue{false};
    bool roundStartDue{false};
    bool roundEndDue{false};
    bool switchDue{false};
};

// ============================================================================
// TIMER LOGIC ENGINE
// ============================================================================
class TimerLogic {
public:
    using EventCallback = std::function<void(const DisplayInfo&)>;
    
    TimerLogic();
    
    // --- State Machine Inputs ---
    void onRotate(int delta);
    void onShortPress();
    void onLongPress();
    void tick();  // Call every second from main loop
    
    // --- Getters ---
    DisplayInfo getDisplayInfo() const;
    TimerState getState() const { return state_; }
    TimerMode getMode() const { return mode_; }
    
    void setEventCallback(EventCallback cb) { eventCb_ = std::move(cb); }
    
private:
    void enterMenu();
    void enterSetupWork();
    void enterSetupRest();
    void enterSetupRounds();
    void enterRunning();
    void enterPaused();
    void enterFinished();
    
    void advanceMenu(int delta);
    void advanceSetupWork(int delta);
    void advanceSetupRest(int delta);
    void advanceSetupRounds(int delta);
    void adjustRunningTime(int delta);
    
    void notifyDisplay();
    void playAudioEvents();
    
    unsigned getWorkSeconds() const;
    unsigned getRestSeconds() const;
    
    TimerState state_{TimerState::MENU};
    TimerMode mode_{TimerMode::SPARRING};
    Phase phase_{Phase::WORK};
    TimerConfig config_;
    
    unsigned currentRound_{0};
    unsigned totalRounds_{0};
    std::atomic<unsigned> secondsRemaining_{0};
    unsigned lastSecondsRemaining_{0};
    
    bool tenSecondPlayed_{false};
    std::string menuLabel_;
    std::string valueLabel_;
    unsigned setupValue_{0};
    bool tenSecondWarningDue_{false};
    bool roundStartDue_{false};
    bool roundEndDue_{false};
    bool switchDue_{false};
    EventCallback eventCb_;
};

} // namespace bjj
