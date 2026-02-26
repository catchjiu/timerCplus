/**
 * BJJ Gym Timer - Main Application
 * Raspberry Pi 5 - Rotary Encoder + Passive Buzzer
 * Uses lgpio (no daemon required)
 * Run: sudo ./bjj_timer
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
#include <string>

using namespace bjj;

// ============================================================================
// ANSI - Professional dark theme
// ============================================================================
namespace ansi {
    const char* R       = "\033[0m";
    const char* BOLD    = "\033[1m";
    const char* DIM     = "\033[2m";
    const char* INV     = "\033[7m";
    
    const char* WHITE   = "\033[97m";
    const char* GRAY    = "\033[90m";
    const char* RED     = "\033[91m";
    const char* GREEN   = "\033[92m";
    const char* YELLOW  = "\033[93m";
    const char* BLUE    = "\033[94m";
    const char* MAGENTA = "\033[95m";
    const char* CYAN    = "\033[96m";
    
    const char* BG_DARK = "\033[48;5;235m";
    const char* BG_DKR  = "\033[48;5;232m";
    
    void clear()   { std::cout << "\033[2J\033[H\033[40m"; }
    void hideCur() { std::cout << "\033[?25l"; }
    void showCur() { std::cout << "\033[?25h"; }
}

// ============================================================================
// LARGE LED-STYLE DIGITS (7 lines, professional)
// ============================================================================
static const char* LED[][7] = {
    {" ███████ ", "██     ██", "██     ██", "██     ██", "██     ██", "██     ██", " ███████ "},
    {"      ██ ", "      ██ ", "      ██ ", "      ██ ", "      ██ ", "      ██ ", "      ██ "},
    {" ███████ ", "      ██ ", "      ██ ", " ███████ ", "██       ", "██       ", " ███████ "},
    {" ███████ ", "      ██ ", "      ██ ", " ███████ ", "       ██", "       ██", " ███████ "},
    {"██     ██", "██     ██", "██     ██", " ███████ ", "      ██ ", "      ██ ", "      ██ "},
    {" ███████ ", "██       ", "██       ", " ███████ ", "       ██", "       ██", " ███████ "},
    {" ███████ ", "██       ", "██       ", " ███████ ", "██     ██", "██     ██", " ███████ "},
    {" ███████ ", "      ██ ", "      ██ ", "      ██ ", "      ██ ", "      ██ ", "      ██ "},
    {" ███████ ", "██     ██", "██     ██", " ███████ ", "██     ██", "██     ██", " ███████ "},
    {" ███████ ", "██     ██", "██     ██", " ███████ ", "       ██", "       ██", " ███████ "},
};
static const char* LED_COLON[] = {"   ", " █ ", "   ", " █ ", "   ", " █ ", "   "};

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
static int g_gpioHandle = -1;

void onRotate(int delta) { g_rotateDelta += delta; }
void onPress(bool isLong) { isLong ? (void)(g_longPress = true) : (void)(g_shortPress = true); }

// ============================================================================
// RENDER LED CLOCK
// ============================================================================
void renderClock(unsigned sec, const char* color, std::ostream& out) {
    unsigned m = sec / 60, s = sec % 60;
    int d[] = { static_cast<int>(m/10), static_cast<int>(m%10), static_cast<int>(s/10), static_cast<int>(s%10) };
    
    out << color;
    for (int row = 0; row < 7; ++row) {
        out << "        ";
        out << LED[d[0]][row] << " " << LED[d[1]][row];
        out << LED_COLON[row];
        out << LED[d[2]][row] << " " << LED[d[3]][row];
        out << "\n";
    }
    out << ansi::R;
}

// ============================================================================
// PROFESSIONAL DISPLAY
// ============================================================================
void renderDisplay(const DisplayInfo& info) {
    std::lock_guard<std::mutex> lock(g_displayMutex);
    ansi::clear();
    
    const int W = 52;
    auto line = [W](const std::string& s, int visibleLen = -1) {
        int v = visibleLen >= 0 ? visibleLen : static_cast<int>(s.size());
        int left = (W - 2 - v) / 2;
        int right = W - 2 - v - left;
        std::cout << " " << ansi::GRAY << "│" << ansi::R;
        std::cout << std::string(std::max(0, left), ' ') << s << std::string(std::max(0, right), ' ');
        std::cout << ansi::GRAY << "│" << ansi::R << "\n";
    };
    
    auto boxTop = [W]() {
        std::cout << " " << ansi::GRAY << "┌" << std::string(W-2, '─') << "┐" << ansi::R << "\n";
    };
    auto footer = [W]() {
        std::cout << " " << ansi::GRAY << "└" << std::string(W-2, '─') << "┘" << ansi::R << "\n";
    };
    
    // Header
    std::cout << "\n ";
    std::cout << ansi::GRAY << "┌" << std::string(W-2, '─') << "┐" << ansi::R << "\n";
    std::cout << " " << ansi::GRAY << "│" << ansi::R;
    std::cout << std::string((W-18)/2, ' ') << ansi::BOLD << ansi::WHITE << "BJJ GYM TIMER" << ansi::R;
    std::cout << std::string(W-18-(W-18)/2, ' ') << ansi::GRAY << "│" << ansi::R << "\n";
    std::cout << " " << ansi::GRAY << "└" << std::string(W-2, '─') << "┘" << ansi::R << "\n\n";
    
    boxTop();
    switch (info.state) {
        case TimerState::MENU: {
            line("");
            line(std::string(ansi::DIM) + "Rotate to select  ·  Press to confirm" + ansi::R, 37);
            line("");
            std::string mode = "  " + info.menuLabel + "  ";
            std::cout << " " << ansi::GRAY << "│" << ansi::R;
            std::cout << std::string((W-2-static_cast<int>(mode.size()))/2, ' ');
            std::cout << ansi::BLUE << ansi::BOLD << ansi::INV << mode << ansi::R;
            std::cout << std::string(W-2-static_cast<int>(mode.size())-(W-2-static_cast<int>(mode.size()))/2, ' ');
            std::cout << ansi::GRAY << "│" << ansi::R << "\n";
            line("");
            footer();
            break;
        }
        
        case TimerState::SETUP_WORK:
        case TimerState::SETUP_REST:
        case TimerState::SETUP_ROUNDS: {
            line("");
            std::string title;
            if (info.state == TimerState::SETUP_WORK) title = "ROUND TIME";
            else if (info.state == TimerState::SETUP_REST) title = "REST TIME";
            else title = (info.mode == TimerMode::DRILLING) ? "INTERVAL PER PERSON" : "NUMBER OF ROUNDS";
            
            line(std::string(ansi::GRAY) + title + ansi::R, static_cast<int>(title.size()));
            line("");
            line(std::string(ansi::DIM) + "Rotate: change  ·  Press: next  ·  Hold: menu" + ansi::R, 42);
            line("");
            std::cout << " " << ansi::GRAY << "│" << ansi::R;
            int vw = static_cast<int>(info.valueLabel.size());
            std::cout << std::string((W-2-vw)/2, ' ');
            std::cout << ansi::GREEN << ansi::BOLD << info.valueLabel << ansi::R;
            std::cout << std::string(W-2-vw-(W-2-vw)/2, ' ');
            std::cout << ansi::GRAY << "│" << ansi::R << "\n";
            line("");
            footer();
            break;
        }
        
        case TimerState::PAUSED: {
            line("");
            std::cout << " " << ansi::GRAY << "│" << ansi::R;
            std::cout << std::string((W-14)/2, ' ') << ansi::RED << ansi::BOLD << "  PAUSED  " << ansi::R;
            std::cout << std::string(W-14-(W-14)/2, ' ') << ansi::GRAY << "│" << ansi::R << "\n";
            line("");
            std::cout << "\n";
            renderClock(info.secondsRemaining, ansi::RED, std::cout);
            line("");
            line(std::string(ansi::DIM) + "Press: resume  ·  Hold 2 sec: menu" + ansi::R, 33);
            footer();
            break;
        }
        
        case TimerState::RUNNING:
        case TimerState::FINISHED: {
            std::string phaseTag, phaseColor;
            if (info.phase == Phase::WORK) {
                phaseTag = " WORK "; phaseColor = ansi::GREEN;
            } else if (info.phase == Phase::REST) {
                phaseTag = " REST "; phaseColor = ansi::YELLOW;
            } else {
                phaseTag = " SWITCH "; phaseColor = ansi::CYAN;
            }
            
            std::string roundInfo;
            if (info.totalRounds > 1) {
                roundInfo = "Round " + std::to_string(info.currentRound) + "/" + std::to_string(info.totalRounds);
            } else if (info.totalRounds == 1) {
                roundInfo = "COMPETITION";
            } else {
                roundInfo = "DRILLING";
            }
            
            std::cout << " " << ansi::GRAY << "│" << ansi::R;
            int rw = static_cast<int>(roundInfo.size()), pw = static_cast<int>(phaseTag.size());
            std::cout << std::string((W-2-rw-pw-2)/2, ' ');
            std::cout << ansi::WHITE << roundInfo << ansi::R << "  ";
            std::cout << phaseColor << ansi::BOLD << phaseTag << ansi::R;
            std::cout << std::string(W-2-rw-pw-2-(W-2-rw-pw-2)/2, ' ');
            std::cout << ansi::GRAY << "│" << ansi::R << "\n";
            line("");
            
            const char* clockColor = (info.secondsRemaining <= 10 && info.phase != Phase::REST) ? ansi::RED : ansi::GREEN;
            renderClock(info.secondsRemaining, clockColor, std::cout);
            
            if (info.state == TimerState::FINISHED) {
                line("");
                std::cout << " " << ansi::GRAY << "│" << ansi::R;
                std::cout << std::string((W-20)/2, ' ') << ansi::GREEN << ansi::BOLD << " MATCH COMPLETE " << ansi::R;
                std::cout << std::string(W-20-(W-20)/2, ' ') << ansi::GRAY << "│" << ansi::R << "\n";
            } else {
                line(std::string(ansi::DIM) + "Rotate: ±30s  ·  Press: pause  ·  Hold: reset" + ansi::R, 41);
            }
            footer();
            break;
        }
    }
    
    std::cout << std::flush;
}

// ============================================================================
// DISPLAY + AUDIO
// ============================================================================
void onDisplayEvent(const DisplayInfo& info) {
    renderDisplay(info);
    if (!g_buzzer || g_gpioHandle < 0) return;
    if (info.roundStartDue)      g_buzzer->playStartRound(g_gpioHandle);
    if (info.tenSecondWarningDue) g_buzzer->play10SecondWarning(g_gpioHandle);
    if (info.roundEndDue)        g_buzzer->playEndRound(g_gpioHandle);
    if (info.switchDue)          g_buzzer->playDrillingSwitch(g_gpioHandle);
}

void signalHandler(int) { g_running = false; }

// ============================================================================
// MAIN
// ============================================================================
int main(int argc, char* argv[]) {
    std::cout << "BJJ Gym Timer - Initializing...\n";
    
    int h = lgGpiochipOpen(4);
    if (h < 0) h = lgGpiochipOpen(0);
    if (h < 0) {
        std::cerr << "ERROR: lgpio failed. Run: sudo ./bjj_timer\n";
        return 1;
    }
    g_gpioHandle = h;
    
    Buzzer buzzer;
    buzzer.init(h);
    g_buzzer = &buzzer;
    
    TimerLogic timer;
    g_timer = &timer;
    timer.setEventCallback(onDisplayEvent);
    
    RotaryEncoder encoder(onRotate, onPress);
    encoder.init(h);
    encoder.attachInterrupts();
    
    signal(SIGINT, signalHandler);
    ansi::hideCur();
    
    onDisplayEvent(timer.getDisplayInfo());
    
    auto lastTick = std::chrono::steady_clock::now();
    auto lastDisplay = lastTick;
    
    while (g_running) {
        auto now = std::chrono::steady_clock::now();
        encoder.poll();
        
        int delta = g_rotateDelta.exchange(0);
        if (delta != 0) timer.onRotate(delta);
        if (g_shortPress.exchange(false)) timer.onShortPress();
        if (g_longPress.exchange(false)) timer.onLongPress();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick).count();
        if (elapsed >= 1000) { lastTick = now; timer.tick(); }
        
        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastDisplay).count();
        if (elapsed >= 100) { lastDisplay = now; renderDisplay(timer.getDisplayInfo()); }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    encoder.detachInterrupts();
    buzzer.silence(g_gpioHandle);
    encoder.freeGpio(g_gpioHandle);
    lgGpioFree(g_gpioHandle, bjj::BUZZER_PIN);
    lgGpiochipClose(g_gpioHandle);
    ansi::showCur();
    
    std::cout << "\nBJJ Gym Timer - Shutdown complete.\n";
    return 0;
}
