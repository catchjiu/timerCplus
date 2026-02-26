/**
 * BJJ Gym Timer - LVGL UI
 * Combat Sports theme: #1A1A1A, #D4AF37, White
 */

#pragma once

#include "timer_logic.hpp"

struct _lv_obj_t;
typedef struct _lv_obj_t lv_obj_t;

struct _lv_timer_t;
typedef struct _lv_timer_t lv_timer_t;

namespace bjj {

// Combat Sports theme colors (RGB565)
#define THEME_BG       0x18E3   // #1A1A1A dark charcoal
#define THEME_GOLD     0xBD55   // #D4AF37 gold
#define THEME_WHITE    0xFFFF
#define THEME_GREEN    0x07E0   // WORK
#define THEME_RED      0xF800   // REST
#define THEME_GRAY     0x7BEF

class BJJTimerUI {
public:
    BJJTimerUI();
    ~BJJTimerUI();

    void create(lv_obj_t* parent);
    void update(const DisplayInfo& info);
    void setTimerLogic(TimerLogic* logic) { timer_ = logic; }
    void setBuzzerCallback(void (*cb)(const DisplayInfo&)) { buzzerCb_ = cb; }

private:
    static void tickTimerCb(lv_timer_t* t);

    void buildMenuScreen();
    void buildSetupScreen();
    void buildRunningScreen();
    void showScreen(int screenId);
    void updateClock(unsigned sec, bool isRest, bool warn10);
    void updateArc(unsigned sec, unsigned total, bool isRest);

    lv_obj_t* screenMenu_ = nullptr;
    lv_obj_t* screenSetup_ = nullptr;
    lv_obj_t* screenRunning_ = nullptr;

    lv_obj_t* headerLabel_ = nullptr;
    lv_obj_t* clockLabel_ = nullptr;
    lv_obj_t* progressArc_ = nullptr;
    lv_obj_t* phaseLabel_ = nullptr;
    lv_obj_t* roundLabel_ = nullptr;
    lv_obj_t* modeRoller_ = nullptr;
    lv_obj_t* setupTitleLabel_ = nullptr;
    lv_obj_t* valueLabel_ = nullptr;

    lv_timer_t* tickTimer_ = nullptr;
    TimerLogic* timer_ = nullptr;
    void (*buzzerCb_)(const DisplayInfo&) = nullptr;

    int currentScreen_ = 0;
    unsigned lastSeconds_ = 0;
};

} // namespace bjj
