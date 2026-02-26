/**
 * BJJ Gym Timer - Main Application
 * Raspberry Pi - Rotary Encoder + Passive Buzzer
 * 
 * Run: sudo ./bjj_timer
 * Requires: pigpio daemon (sudo pigpiod)
 */

#include "hardware.hpp"
#include "timer_logic.hpp"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <csignal>

using namespace bjj;

// ============================================================================
// ANSI ESCAPE CODES - Beautiful CLI
// ============================================================================
namespace ansi {
    const char* RESET    = "\033[0m";
    const char* BOLD     = "\033[1m";
    const char* DIM      = "\033[2m";
    
    const char* FG_BLACK   = "\033[30m";
    const char* FG_RED     = "\033[31m";
    const char* FG_GREEN   = "\033[32m";
    const char* FG_YELLOW  = "\033[33m";
    const char* FG_BLUE    = "\033[34m";
    const char* FG_MAGENTA = "\033[35m";
    const char* FG_CYAN    = "\033[36m";
    const char* FG_WHITE   = "\033[37m";
    
    const char* BG_RED     = "\033[41m";
    const char* BG_GREEN   = "\033[42m";
    const char* BG_YELLOW  = "\033[43m";
    const char* BG_BLUE    = "\033[44m";
    
    void clearScreen() { std::cout << "\033[2J\033[H"; }
    void hideCursor() { std::cout << "\033[?25l"; }
    void showCursor() { std::cout << "\033[?25h"; }
}

// ============================================================================
// LARGE 7-SEGMENT STYLE DIGITS (for clock display)
// ============================================================================
// Each digit is 5 lines tall, 5 chars wide
static const char* DIGITS[][5] = {
    {" ### ", "#   #", "#   #", "#   #", " ### "},
    {"    #", "    #", "    #", "    #", "    #"},
    {" ####", "    #", " ####", "#    ", " ####"},
    {" ####", "    #", " ####", "    #", " ####"},
    {"#   #", "#   #", " ####", "    #", "    #"},
    {" ####", "#    ", " ####", "    #", " ####"},
    {" ####", "#    ", " ####", "#   #", " ####"},
    {" ####", "    #", "    #", "    #", "    #"},
    {" ### ", "#   #", " ### ", "#   #", " ### "},
    {" ####", "#   #", " ####", "    #", " ####"},
};

static const char* COLON[] = {"   ", " # ", "   ", " # ", "   "};

// ============================================================================
// GLOBAL STATE
// ============================================================================
static std::atomic<bool> g_running{true};
static std::atomic<int> g_rotateDelta{0};
static std::atomic<bool> g_shortPress{false};
static std::atomic<bool> g_longPress{false};
static std::mutex g_displayMutex;
static Buzzer* g_buzzer = nullptr;
static TimerLogic* g_timer = nullptr;

// ============================================================================
// ENCODER CALLBACKS (called from ISR - set flags only)
// ============================================================================
void onRotate(int delta) {
    g_rotateDelta += delta;
}

void onPress(bool isLong) {
    if (isLong) {
        g_longPress = true;
    } else {
        g_shortPress = true;
    }
}

// ============================================================================
// RENDER LARGE CLOCK
// ============================================================================
void renderLargeClock(unsigned seconds, std::ostream& out) {
    unsigned m = seconds / 60;
    unsigned s = seconds % 60;
    int d0 = m / 10, d1 = m % 10;
    int d2 = s / 10, d3 = s % 10;
    
    for (int row = 0; row < 5; ++row) {
        out << "    ";
        for (int c = 0; c < 5; ++c) out << DIGITS[d0][row][c];
        out << " ";
        for (int c = 0; c < 5; ++c) out << DIGITS[d1][row][c];
        out << " ";
        for (int c = 0; c < 3; ++c) out << COLON[row][c];
        out << " ";
        for (int c = 0; c < 5; ++c) out << DIGITS[d2][row][c];
        out << " ";
        for (int c = 0; c < 5; ++c) out << DIGITS[d3][row][c];
        out << "\n";
    }
}

// ============================================================================
// RENDER FULL DISPLAY
// ============================================================================
void renderDisplay(const DisplayInfo& info) {
    std::lock_guard<std::mutex> lock(g_displayMutex);
    ansi::clearScreen();
    
    std::cout << ansi::BOLD << ansi::FG_CYAN
              << "  ╔══════════════════════════════════════╗\n"
              << "  ║       BJJ GYM TIMER - OSS!           ║\n"
              << "  ╚══════════════════════════════════════╝\n" << ansi::RESET;
    
    switch (info.state) {
        case TimerState::MENU: {
            std::cout << "\n  " << ansi::DIM << "Rotate: Select Mode   Press: Confirm" << ansi::RESET << "\n\n";
            std::cout << "        " << ansi::BG_BLUE << ansi::FG_WHITE << ansi::BOLD
                      << "  " << info.menuLabel << "  " << ansi::RESET << "\n\n";
            break;
        }
        
        case TimerState::SETUP_WORK:
        case TimerState::SETUP_REST:
        case TimerState::SETUP_ROUNDS: {
            std::string title;
            if (info.state == TimerState::SETUP_WORK) title = "Round Time";
            else if (info.state == TimerState::SETUP_REST) title = "Rest Time";
            else title = (info.mode == TimerMode::DRILLING) ? "Interval (each)" : "Rounds";
            
            std::cout << "\n  " << ansi::DIM << title << ansi::RESET << "\n";
            std::cout << "  Rotate: Change   Press: Next   Long: Menu\n\n";
            std::cout << "        " << ansi::FG_GREEN << ansi::BOLD << info.valueLabel << ansi::RESET << "\n\n";
            break;
        }
        
        case TimerState::PAUSED: {
            std::cout << "\n\n";
            std::cout << ansi::FG_RED << ansi::BOLD
                      << "        ██████  █████  ██    ██ ███████ ███████ ██████  \n"
                      << "        ██   ██ ██   ██ ██    ██ ██      ██      ██   ██ \n"
                      << "        ██████  ███████ ██    ██ ███████ ███████ ██████  \n"
                      << "        ██      ██   ██  ██  ██       ██      ██ ██      \n"
                      << "        ██      ██   ██   ████   ███████ ███████ ██      \n"
                      << ansi::RESET << "\n";
            renderLargeClock(info.secondsRemaining, std::cout);
            std::cout << "\n  " << ansi::DIM << "Press: Resume   Long: Back to Menu" << ansi::RESET << "\n";
            break;
        }
        
        case TimerState::RUNNING:
        case TimerState::FINISHED: {
            std::string phaseStr;
            if (info.phase == Phase::WORK) {
                phaseStr = std::string(ansi::BG_GREEN) + ansi::FG_WHITE + " WORK " + ansi::RESET;
            } else if (info.phase == Phase::REST) {
                phaseStr = std::string(ansi::BG_YELLOW) + ansi::FG_BLACK + " REST " + ansi::RESET;
            } else {
                phaseStr = std::string(ansi::BG_BLUE) + ansi::FG_WHITE + " SWITCH! " + ansi::RESET;
            }
            
            if (info.totalRounds > 1) {
                std::cout << "\n  Round " << info.currentRound << "/" << info.totalRounds
                          << "    " << phaseStr << "\n\n";
            } else if (info.totalRounds == 1) {
                std::cout << "\n  " << ansi::FG_MAGENTA << "COMPETITION" << ansi::RESET
                          << "    " << phaseStr << "\n\n";
            } else {
                std::cout << "\n  " << ansi::FG_CYAN << "DRILLING" << ansi::RESET
                          << "    " << phaseStr << "\n\n";
            }
            
            if (info.secondsRemaining <= 10 && info.phase != Phase::REST) {
                std::cout << ansi::FG_RED << ansi::BOLD;
            } else {
                std::cout << ansi::FG_GREEN;
            }
            renderLargeClock(info.secondsRemaining, std::cout);
            std::cout << ansi::RESET;
            
            if (info.state == TimerState::FINISHED) {
                std::cout << "\n  " << ansi::FG_GREEN << ansi::BOLD << "MATCH COMPLETE! OSS!" << ansi::RESET << "\n";
            } else {
                std::cout << "\n  " << ansi::DIM << "Rotate: +/-30s   Press: Pause   Long: Reset" << ansi::RESET << "\n";
            }
            break;
        }
    }
    
    std::cout << std::flush;
}

// ============================================================================
// DISPLAY EVENT CALLBACK - also triggers audio
// ============================================================================
void onDisplayEvent(const DisplayInfo& info) {
    renderDisplay(info);
    
    if (!g_buzzer) return;
    
    if (info.roundStartDue) {
        g_buzzer->playStartRound();
    }
    if (info.tenSecondWarningDue) {
        g_buzzer->play10SecondWarning();
    }
    if (info.roundEndDue) {
        g_buzzer->playEndRound();
    }
    if (info.switchDue) {
        g_buzzer->playDrillingSwitch();
    }
}

// ============================================================================
// SIGNAL HANDLER
// ============================================================================
void signalHandler(int) {
    g_running = false;
}

// ============================================================================
// MAIN
// ============================================================================
int main(int argc, char* argv[]) {
    std::cout << "BJJ Gym Timer - Initializing...\n";
    
    if (gpioInitialise() < 0) {
        std::cerr << "ERROR: pigpio init failed. Run: sudo pigpiod\n";
        return 1;
    }
    
    Buzzer buzzer;
    buzzer.init();
    g_buzzer = &buzzer;
    
    TimerLogic timer;
    g_timer = &timer;
    timer.setEventCallback(onDisplayEvent);
    
    RotaryEncoder encoder(onRotate, onPress);
    encoder.init();
    encoder.attachInterrupts();
    
    signal(SIGINT, signalHandler);
    ansi::hideCursor();
    
    // Initial display
    onDisplayEvent(timer.getDisplayInfo());
    
    auto lastTick = std::chrono::steady_clock::now();
    auto lastDisplay = lastTick;
    
    while (g_running) {
        auto now = std::chrono::steady_clock::now();
        
        // Process encoder inputs (from ISR)
        int delta = g_rotateDelta.exchange(0);
        if (delta != 0) {
            timer.onRotate(delta);
        }
        if (g_shortPress.exchange(false)) {
            timer.onShortPress();
        }
        if (g_longPress.exchange(false)) {
            timer.onLongPress();
        }
        
        // 1-second tick for countdown
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick).count();
        if (elapsed >= 1000) {
            lastTick = now;
            timer.tick();
        }
        
        // Throttle display updates to ~10 Hz
        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastDisplay).count();
        if (elapsed >= 100) {
            lastDisplay = now;
            renderDisplay(timer.getDisplayInfo());
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    encoder.detachInterrupts();
    buzzer.silence();
    gpioTerminate();
    ansi::showCursor();
    
    std::cout << "\nBJJ Gym Timer - Shutdown complete. OSS!\n";
    return 0;
}
