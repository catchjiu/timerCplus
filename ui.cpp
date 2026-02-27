/**
 * BJJ Gym Timer - LVGL UI Implementation
 */

#include "ui.hpp"
#include "hardware.hpp"
#include "lvgl_port.hpp"
#include <lvgl.h>
#include <cstdio>
#include <cstring>

namespace bjj {

BJJTimerUI::BJJTimerUI() = default;

BJJTimerUI::~BJJTimerUI() {
    if (tickTimer_) lv_timer_del(tickTimer_);
}

void BJJTimerUI::create(lv_obj_t* parent) {
    lv_obj_t* root = parent ? parent : lv_screen_active();
    if (!root) return;

    // Main container - full screen, dark bg
    lv_obj_set_style_bg_color(root, lv_color_hex(THEME_BG), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);

    // Create screens first (below header)
    screenMenu_ = lv_obj_create(root);
    lv_obj_set_size(screenMenu_, LV_PCT(100), LV_PCT(100));
    lv_obj_clear_flag(screenMenu_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(screenMenu_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screenMenu_, 0, 0);
    lv_obj_set_style_pad_all(screenMenu_, 0, 0);

    screenSetup_ = lv_obj_create(root);
    lv_obj_set_size(screenSetup_, LV_PCT(100), LV_PCT(100));
    lv_obj_clear_flag(screenSetup_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(screenSetup_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screenSetup_, 0, 0);

    screenRunning_ = lv_obj_create(root);
    lv_obj_set_size(screenRunning_, LV_PCT(100), LV_PCT(100));
    lv_obj_clear_flag(screenRunning_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(screenRunning_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screenRunning_, 0, 0);

    // Menu: mode roller
    lv_obj_t* menuTitle = lv_label_create(screenMenu_);
    lv_label_set_text(menuTitle, "Select Mode");
    lv_obj_set_style_text_color(menuTitle, lv_color_hex(THEME_GRAY), 0);
    lv_obj_align(menuTitle, LV_ALIGN_TOP_MID, 0, 100);

    modeRoller_ = lv_roller_create(screenMenu_);
    lv_roller_set_options(modeRoller_, "SPARRING\nDRILLING\nCOMPETITION", LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(modeRoller_, 3);
    lv_obj_set_style_text_color(modeRoller_, lv_color_hex(THEME_GOLD), LV_PART_SELECTED);
    lv_obj_set_style_text_color(modeRoller_, lv_color_hex(THEME_WHITE), 0);
    lv_obj_set_style_text_font(modeRoller_, &lv_font_montserrat_48, 0);
    lv_obj_align(modeRoller_, LV_ALIGN_CENTER, 0, 20);
    lv_group_add_obj(lvgl_port_get_group(), modeRoller_);

    // Setup: title + value
    setupTitleLabel_ = lv_label_create(screenSetup_);
    lv_label_set_text(setupTitleLabel_, "Work Time");
    lv_obj_set_style_text_color(setupTitleLabel_, lv_color_hex(THEME_WHITE), 0);
    lv_obj_set_style_text_font(setupTitleLabel_, &lv_font_montserrat_14, 0);
    lv_obj_align(setupTitleLabel_, LV_ALIGN_TOP_MID, 0, 140);

    valueLabel_ = lv_label_create(screenSetup_);
    lv_label_set_text(valueLabel_, "5:00");
    lv_obj_set_style_text_color(valueLabel_, lv_color_hex(THEME_GOLD), 0);
    lv_obj_set_style_text_font(valueLabel_, &lv_font_montserrat_48, 0);
    lv_obj_align(valueLabel_, LV_ALIGN_CENTER, 0, 0);

    // Running: arc + clock
    int arcSize = 280;
    progressArc_ = lv_arc_create(screenRunning_);
    lv_obj_set_size(progressArc_, arcSize, arcSize);
    lv_obj_center(progressArc_);
    lv_arc_set_range(progressArc_, 0, 100);
    lv_arc_set_value(progressArc_, 100);
    lv_arc_set_bg_angles(progressArc_, 0, 360);
    lv_arc_set_rotation(progressArc_, 270);
    lv_obj_set_style_arc_width(progressArc_, 12, LV_PART_MAIN);
    lv_obj_set_style_arc_width(progressArc_, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(progressArc_, lv_color_hex(THEME_GRAY), LV_PART_MAIN);
    lv_obj_set_style_arc_color(progressArc_, lv_color_hex(THEME_GREEN), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(progressArc_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(progressArc_, 0, 0);
    lv_obj_remove_flag(progressArc_, LV_OBJ_FLAG_CLICKABLE);

    clockLabel_ = lv_label_create(screenRunning_);
    lv_label_set_text(clockLabel_, "05:00");
    lv_obj_set_style_text_color(clockLabel_, lv_color_hex(THEME_WHITE), 0);
    lv_obj_set_style_text_font(clockLabel_, &lv_font_montserrat_48, 0);
    lv_obj_center(clockLabel_);

    phaseLabel_ = lv_label_create(screenRunning_);
    lv_label_set_text(phaseLabel_, "WORK");
    lv_obj_set_style_text_color(phaseLabel_, lv_color_hex(THEME_GREEN), 0);
    lv_obj_set_style_text_font(phaseLabel_, &lv_font_montserrat_14, 0);
    lv_obj_align(phaseLabel_, LV_ALIGN_TOP_MID, 0, 90);

    roundLabel_ = lv_label_create(screenRunning_);
    lv_label_set_text(roundLabel_, "Round 1/5");
    lv_obj_set_style_text_color(roundLabel_, lv_color_hex(THEME_GOLD), 0);
    lv_obj_set_style_text_font(roundLabel_, &lv_font_montserrat_14, 0);
    lv_obj_align(roundLabel_, LV_ALIGN_TOP_MID, 0, 110);

    // Header - "CATCH JIU JITSU" (created last so it stays on top)
    headerLabel_ = lv_label_create(root);
    lv_label_set_text(headerLabel_, "CATCH JIU JITSU");
    lv_obj_set_style_text_color(headerLabel_, lv_color_hex(THEME_GOLD), 0);
    lv_obj_set_style_text_font(headerLabel_, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_letter_space(headerLabel_, 4, 0);
    lv_obj_align(headerLabel_, LV_ALIGN_TOP_MID, 0, 20);

    // Initial screen: menu
    lv_obj_add_flag(screenSetup_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(screenRunning_, LV_OBJ_FLAG_HIDDEN);
    currentScreen_ = 1;

    // Tick timer - drives timer logic and UI update
    tickTimer_ = lv_timer_create(tickTimerCb, 1000, this);
}

void BJJTimerUI::tickTimerCb(lv_timer_t* t) {
    BJJTimerUI* ui = static_cast<BJJTimerUI*>(lv_timer_get_user_data(t));
    if (!ui->timer_) return;
    ui->timer_->tick();
    DisplayInfo info = ui->timer_->getDisplayInfo();
    ui->update(info);
    if (ui->buzzerCb_) ui->buzzerCb_(info);
    ui->timer_->clearAudioFlags();
}

void BJJTimerUI::update(const DisplayInfo& info) {
    char buf[32];

    switch (info.state) {
        case TimerState::MENU:
            showScreen(1);
            if (info.mode == TimerMode::SPARRING) lv_roller_set_selected(modeRoller_, 0, LV_ANIM_OFF);
            else if (info.mode == TimerMode::DRILLING) lv_roller_set_selected(modeRoller_, 1, LV_ANIM_OFF);
            else lv_roller_set_selected(modeRoller_, 2, LV_ANIM_OFF);
            break;

        case TimerState::SETUP_WORK:
        case TimerState::SETUP_REST:
        case TimerState::SETUP_ROUNDS:
            showScreen(2);
            lv_label_set_text(valueLabel_, info.valueLabel.c_str());
            if (info.state == TimerState::SETUP_WORK)
                lv_label_set_text(setupTitleLabel_, info.mode == TimerMode::COMPETITION ? "Match Time" : "Work Time");
            else if (info.state == TimerState::SETUP_REST)
                lv_label_set_text(setupTitleLabel_, "Rest Time");
            else
                lv_label_set_text(setupTitleLabel_, info.mode == TimerMode::DRILLING ? "Interval" : "Rounds");
            break;

        case TimerState::RUNNING:
        case TimerState::PAUSED:
        case TimerState::FINISHED:
            showScreen(3);
            snprintf(buf, sizeof(buf), "%02u:%02u", info.secondsRemaining / 60, info.secondsRemaining % 60);
            lv_label_set_text(clockLabel_, buf);

            if (info.state == TimerState::PAUSED) {
                lv_label_set_text(phaseLabel_, "PAUSED");
                lv_obj_set_style_text_color(phaseLabel_, lv_color_hex(THEME_RED), 0);
            } else {
                if (info.phase == Phase::WORK) {
                    lv_label_set_text(phaseLabel_, "WORK");
                    lv_obj_set_style_text_color(phaseLabel_, lv_color_hex(THEME_GREEN), 0);
                } else if (info.phase == Phase::REST) {
                    lv_label_set_text(phaseLabel_, "REST");
                    lv_obj_set_style_text_color(phaseLabel_, lv_color_hex(THEME_RED), 0);
                } else {
                    lv_label_set_text(phaseLabel_, "SWITCH!");
                    lv_obj_set_style_text_color(phaseLabel_, lv_color_hex(THEME_GOLD), 0);
                }
            }

            if (info.totalRounds > 1) {
                snprintf(buf, sizeof(buf), "Round %u/%u", info.currentRound, info.totalRounds);
            } else if (info.totalRounds == 1) {
                snprintf(buf, sizeof(buf), "COMPETITION");
            } else {
                snprintf(buf, sizeof(buf), "DRILLING");
            }
            lv_label_set_text(roundLabel_, buf);

            unsigned total = info.phaseTotalSeconds;
            if (total == 0) total = 60;
            updateArc(info.secondsRemaining, total, info.phase == Phase::REST);
            updateClock(info.secondsRemaining, info.phase == Phase::REST,
                       info.secondsRemaining <= 10 && info.phase != Phase::REST);
            break;
    }
}

void BJJTimerUI::showScreen(int id) {
    if (id == currentScreen_) return;
    currentScreen_ = id;
    lv_obj_add_flag(screenMenu_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(screenSetup_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(screenRunning_, LV_OBJ_FLAG_HIDDEN);
    if (id == 1) lv_obj_remove_flag(screenMenu_, LV_OBJ_FLAG_HIDDEN);
    else if (id == 2) lv_obj_remove_flag(screenSetup_, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(screenRunning_, LV_OBJ_FLAG_HIDDEN);
}

void BJJTimerUI::updateClock(unsigned sec, bool isRest, bool warn10) {
    if (warn10)
        lv_obj_set_style_text_color(clockLabel_, lv_color_hex(THEME_RED), 0);
    else if (isRest)
        lv_obj_set_style_text_color(clockLabel_, lv_color_hex(THEME_RED), 0);
    else
        lv_obj_set_style_text_color(clockLabel_, lv_color_hex(THEME_WHITE), 0);
}

void BJJTimerUI::updateArc(unsigned sec, unsigned total, bool isRest) {
    if (total == 0) return;
    int val = (int)((sec * 100) / total);
    if (val < 0) val = 0;
    if (val > 100) val = 100;
    lv_arc_set_value(progressArc_, val);
    lv_obj_set_style_arc_color(progressArc_, lv_color_hex(isRest ? THEME_RED : THEME_GREEN), LV_PART_INDICATOR);
}

} // namespace bjj
