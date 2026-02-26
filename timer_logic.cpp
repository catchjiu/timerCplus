/**
 * BJJ Gym Timer - Timer Logic Implementation
 */

#include "timer_logic.hpp"
#include "hardware.hpp"
#include <algorithm>
#include <sstream>

namespace bjj {

TimerLogic::TimerLogic() {
    totalRounds_ = config_.roundCount;
    menuLabel_ = "SPARRING";  // Default mode
}

unsigned TimerLogic::getWorkSeconds() const {
    if (mode_ == TimerMode::COMPETITION) {
        return COMPETITION_TIMES[config_.compTimeIndex];
    }
    return config_.workSeconds;
}

unsigned TimerLogic::getRestSeconds() const {
    return config_.restSeconds;
}

void TimerLogic::enterMenu() {
    state_ = TimerState::MENU;
    switch (mode_) {
        case TimerMode::SPARRING:   menuLabel_ = "SPARRING"; break;
        case TimerMode::DRILLING:   menuLabel_ = "DRILLING"; break;
        case TimerMode::COMPETITION: menuLabel_ = "COMPETITION"; break;
    }
    notifyDisplay();
}

void TimerLogic::enterSetupWork() {
    state_ = TimerState::SETUP_WORK;
    if (mode_ == TimerMode::COMPETITION) {
        setupValue_ = config_.compTimeIndex;
    } else {
        setupValue_ = config_.workSeconds;
    }
    notifyDisplay();
}

void TimerLogic::enterSetupRest() {
    state_ = TimerState::SETUP_REST;
    setupValue_ = config_.restSeconds;
    notifyDisplay();
}

void TimerLogic::enterSetupRounds() {
    state_ = TimerState::SETUP_ROUNDS;
    if (mode_ == TimerMode::DRILLING) {
        setupValue_ = config_.workSeconds;  // Interval per person
    } else {
        setupValue_ = config_.roundCount;
    }
    notifyDisplay();
}

void TimerLogic::enterRunning() {
    state_ = TimerState::RUNNING;
    currentRound_ = 1;
    if (mode_ == TimerMode::SPARRING) {
        totalRounds_ = config_.roundCount;
    } else if (mode_ == TimerMode::COMPETITION) {
        totalRounds_ = 1;  // Single match
    } else {
        totalRounds_ = 0;  // Drilling = continuous
    }
    phase_ = Phase::WORK;
    
    if (mode_ == TimerMode::DRILLING) {
        secondsRemaining_ = config_.workSeconds;
    } else {
        secondsRemaining_ = getWorkSeconds();
    }
    lastSecondsRemaining_ = secondsRemaining_.load();
    tenSecondPlayed_ = false;
    
    roundStartDue_ = true;  // Trigger start buzzer
    roundEndDue_ = false;
    switchDue_ = false;
    tenSecondWarningDue_ = false;
    notifyDisplay();
}

void TimerLogic::enterPaused() {
    state_ = TimerState::PAUSED;
    notifyDisplay();
}

void TimerLogic::enterFinished() {
    state_ = TimerState::FINISHED;
    roundEndDue_ = true;  // Final buzzer
    notifyDisplay();
}

void TimerLogic::advanceMenu(int delta) {
    int m = static_cast<int>(mode_) + delta;
    if (m < 0) m = 2;
    if (m > 2) m = 0;
    mode_ = static_cast<TimerMode>(m);
    
    switch (mode_) {
        case TimerMode::SPARRING:   menuLabel_ = "SPARRING"; break;
        case TimerMode::DRILLING:   menuLabel_ = "DRILLING"; break;
        case TimerMode::COMPETITION: menuLabel_ = "COMPETITION"; break;
    }
    notifyDisplay();
}

void TimerLogic::advanceSetupWork(int delta) {
    if (mode_ == TimerMode::COMPETITION) {
        int idx = static_cast<int>(config_.compTimeIndex) + delta;
        if (idx < 0) idx = COMPETITION_COUNT - 1;
        if (idx >= static_cast<int>(COMPETITION_COUNT)) idx = 0;
        config_.compTimeIndex = idx;
        setupValue_ = config_.compTimeIndex;
    } else {
        int val = static_cast<int>(config_.workSeconds) + delta * ROUND_INCREMENT;
        val = std::max(60, std::min(3600, val));
        config_.workSeconds = val;
        setupValue_ = config_.workSeconds;
    }
    notifyDisplay();
}

void TimerLogic::advanceSetupRest(int delta) {
    int val = static_cast<int>(config_.restSeconds) + delta * ROUND_INCREMENT;
    val = std::max(0, std::min(600, val));
    config_.restSeconds = val;
    setupValue_ = config_.restSeconds;
    notifyDisplay();
}

void TimerLogic::advanceSetupRounds(int delta) {
    if (mode_ == TimerMode::DRILLING) {
        int val = static_cast<int>(config_.workSeconds) + delta * ROUND_INCREMENT;
        val = std::max(30, std::min(600, val));
        config_.workSeconds = val;
        setupValue_ = config_.workSeconds;
    } else {
        int val = static_cast<int>(config_.roundCount) + delta;
        val = std::max(1, std::min(20, val));
        config_.roundCount = val;
        setupValue_ = config_.roundCount;
    }
    notifyDisplay();
}

void TimerLogic::adjustRunningTime(int delta) {
    int adj = delta * RUNTIME_ADJUST;
    int s = static_cast<int>(secondsRemaining_.load()) + adj;
    s = std::max(0, std::min(3600, s));
    secondsRemaining_ = s;
    lastSecondsRemaining_ = s;
    if (s <= 10) tenSecondPlayed_ = true;
    notifyDisplay();
}

void TimerLogic::onRotate(int delta) {
    switch (state_) {
        case TimerState::MENU:
            advanceMenu(delta);
            break;
        case TimerState::SETUP_WORK:
            advanceSetupWork(delta);
            break;
        case TimerState::SETUP_REST:
            advanceSetupRest(delta);
            break;
        case TimerState::SETUP_ROUNDS:
            advanceSetupRounds(delta);
            break;
        case TimerState::RUNNING:
        case TimerState::PAUSED:
            adjustRunningTime(delta);
            break;
        default:
            break;
    }
}

void TimerLogic::onShortPress() {
    switch (state_) {
        case TimerState::SETUP_WORK:
            if (mode_ == TimerMode::SPARRING) {
                enterSetupRest();
            } else if (mode_ == TimerMode::DRILLING || mode_ == TimerMode::COMPETITION) {
                enterRunning();  // Drilling/Comp go straight to run
            }
            break;
        case TimerState::SETUP_REST:
            enterSetupRounds();
            break;
        case TimerState::SETUP_ROUNDS:
            enterRunning();
            break;
        case TimerState::RUNNING:
            enterPaused();
            break;
        case TimerState::PAUSED:
            state_ = TimerState::RUNNING;
            notifyDisplay();
            break;
        case TimerState::MENU:
            enterSetupWork();
            break;
        case TimerState::FINISHED:
            enterMenu();
            break;
        default:
            break;
    }
    
}

void TimerLogic::onLongPress() {
    if (state_ == TimerState::RUNNING || state_ == TimerState::PAUSED || 
        state_ == TimerState::FINISHED) {
        enterMenu();
    } else if (state_ == TimerState::SETUP_WORK || state_ == TimerState::SETUP_REST ||
               state_ == TimerState::SETUP_ROUNDS) {
        enterMenu();
    }
}

void TimerLogic::tick() {
    if (state_ != TimerState::RUNNING) return;
    
    unsigned sec = secondsRemaining_.load();
    
    // 10-second warning
    if (sec == TEN_SECOND_MARK && !tenSecondPlayed_) {
        tenSecondWarningDue_ = true;
        tenSecondPlayed_ = true;
    }
    
    if (sec == 0) {
        if (mode_ == TimerMode::DRILLING) {
            switchDue_ = true;
            secondsRemaining_ = config_.workSeconds;  // Next person's turn
            tenSecondPlayed_ = false;
        } else if (mode_ == TimerMode::SPARRING) {
            if (phase_ == Phase::WORK) {
                roundEndDue_ = true;
                if (currentRound_ >= totalRounds_) {
                    enterFinished();
                    return;
                }
                phase_ = Phase::REST;
                secondsRemaining_ = getRestSeconds();
                tenSecondPlayed_ = false;
            } else {
                roundEndDue_ = true;
                currentRound_++;
                phase_ = Phase::WORK;
                secondsRemaining_ = getWorkSeconds();
                roundStartDue_ = true;
                tenSecondPlayed_ = false;
            }
        } else {
            // Competition - single round
            enterFinished();
            return;
        }
    } else {
        secondsRemaining_--;
    }
    
    lastSecondsRemaining_ = secondsRemaining_.load();
    notifyDisplay();
}

DisplayInfo TimerLogic::getDisplayInfo() const {
    DisplayInfo info;
    info.state = state_;
    info.mode = mode_;
    info.phase = phase_;
    info.currentRound = currentRound_;
    info.totalRounds = totalRounds_;
    info.secondsRemaining = secondsRemaining_.load();
    info.phaseTotalSeconds = (phase_ == Phase::REST) ? getRestSeconds() : getWorkSeconds();
    info.menuLabel = menuLabel_;
    info.valueLabel = valueLabel_;
    info.setupValue = setupValue_;
    info.tenSecondWarningDue = tenSecondWarningDue_;
    info.roundStartDue = roundStartDue_;
    info.roundEndDue = roundEndDue_;
    info.switchDue = switchDue_;
    return info;
}

void TimerLogic::notifyDisplay() {
    // Build value labels for setup screens
    if (state_ == TimerState::SETUP_WORK) {
        if (mode_ == TimerMode::COMPETITION) {
            unsigned mins = COMPETITION_TIMES[config_.compTimeIndex] / 60;
            valueLabel_ = std::to_string(mins) + " min";
        } else {
            unsigned m = config_.workSeconds / 60;
            unsigned s = config_.workSeconds % 60;
            valueLabel_ = std::to_string(m) + ":" + (s < 10 ? "0" : "") + std::to_string(s);
        }
    } else if (state_ == TimerState::SETUP_REST) {
        unsigned m = config_.restSeconds / 60;
        unsigned s = config_.restSeconds % 60;
        valueLabel_ = std::to_string(m) + ":" + (s < 10 ? "0" : "") + std::to_string(s);
    } else if (state_ == TimerState::SETUP_ROUNDS) {
        if (mode_ == TimerMode::DRILLING) {
            unsigned m = config_.workSeconds / 60;
            unsigned s = config_.workSeconds % 60;
            valueLabel_ = std::to_string(m) + ":" + (s < 10 ? "0" : "") + std::to_string(s) + " each";
        } else {
            valueLabel_ = std::to_string(config_.roundCount) + " rounds";
        }
    }
    
    DisplayInfo info = getDisplayInfo();
    if (eventCb_) eventCb_(info);
    
    // Clear one-shot audio flags after consumption
    tenSecondWarningDue_ = false;
    roundStartDue_ = false;
    roundEndDue_ = false;
    switchDue_ = false;
}

void TimerLogic::playAudioEvents() {
    // Called from main after getDisplayInfo - audio handled in main via Buzzer
    // This is a no-op; audio triggered by flags in DisplayInfo
}

} // namespace bjj
