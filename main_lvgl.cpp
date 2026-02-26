/**
 * BJJ Gym Timer - LVGL GUI Version
 * Raspberry Pi 5 - Framebuffer + Rotary Encoder
 * Combat Sports theme: CATCH JIU JITSU
 */

#include "hardware.hpp"
#include "timer_logic.hpp"
#include "ui.hpp"
#include "lvgl_port.hpp"
#include <lvgl.h>
#include <lgpio.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

using namespace bjj;

static volatile sig_atomic_t g_running = 1;
static Buzzer* g_buzzer = nullptr;
static TimerLogic* g_timer = nullptr;
static BJJTimerUI* g_ui = nullptr;
static int g_gpio_handle = -1;

static void signal_handler(int) {
    g_running = 0;
}

static void on_buzzer(const DisplayInfo& info) {
    if (!g_buzzer || g_gpio_handle < 0) return;
    if (info.roundStartDue)      g_buzzer->playStartRound(g_gpio_handle);
    if (info.tenSecondWarningDue) g_buzzer->play10SecondWarning(g_gpio_handle);
    if (info.roundEndDue)        g_buzzer->playEndRound(g_gpio_handle);
    if (info.switchDue)          g_buzzer->playDrillingSwitch(g_gpio_handle);
}

static void encoder_cb(int delta, bool pressed, bool long_press) {
    if (!g_timer) return;
    if (long_press) {
        g_timer->onLongPress();
    } else if (pressed) {
        g_timer->onShortPress();
    } else if (delta != 0) {
        g_timer->onRotate(delta);
    }
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    fprintf(stderr, "[bjj_timer_gui] Starting...\n");
    int h = lgGpiochipOpen(4);
    if (h < 0) h = lgGpiochipOpen(0);
    if (h < 0) {
        fprintf(stderr, "[bjj_timer_gui] GPIO init FAILED\n");
        return 1;
    }
    g_gpio_handle = h;
    fprintf(stderr, "[bjj_timer_gui] GPIO ok (handle=%d)\n", h);

    Buzzer buzzer;
    buzzer.init(h);
    g_buzzer = &buzzer;

    TimerLogic timer;
    g_timer = &timer;

    if (lvgl_port_init(h, encoder_cb) != 0) {
        fprintf(stderr, "[bjj_timer_gui] LVGL init FAILED\n");
        lgGpiochipClose(h);
        return 1;
    }
    fprintf(stderr, "[bjj_timer_gui] LVGL/display ok\n");

    g_ui = new BJJTimerUI();
    g_ui->setTimerLogic(&timer);
    g_ui->setBuzzerCallback(on_buzzer);
    g_ui->create(nullptr);

    timer.setEventCallback([](const DisplayInfo& info) {
        if (g_ui) g_ui->update(info);
    });

    signal(SIGINT, signal_handler);

    on_buzzer(timer.getDisplayInfo());
    g_ui->update(timer.getDisplayInfo());

    fprintf(stderr, "[bjj_timer_gui] Main loop running (Ctrl+C to exit)\n");
    unsigned loop_count = 0;
    while (g_running) {
        lvgl_port_encoder_poll();
        lv_timer_handler();
        usleep(5000);
        if (++loop_count % 2000 == 0) {
            fprintf(stderr, "[bjj_timer_gui] alive (%u)\n", loop_count);
        }
    }

    delete g_ui;
    g_ui = nullptr;
    g_buzzer = nullptr;
    g_timer = nullptr;

    buzzer.silence(h);
    lgGpioFree(h, BUZZER_PIN);
    lvgl_port_deinit(h);
    lgGpiochipClose(h);

    return 0;
}
