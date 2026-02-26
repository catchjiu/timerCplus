/**
 * LVGL Port - Framebuffer Display + GPIO Encoder
 */

#include "lvgl_port.hpp"
#include <lvgl.h>
#include <lgpio.h>
#include <atomic>
#include <cstring>

static lv_display_t* g_disp = nullptr;
static lv_indev_t* g_indev = nullptr;
static lv_group_t* g_group = nullptr;
static int g_gpio_handle = -1;
static bjj::RotaryEncoder* g_encoder = nullptr;

static std::atomic<int> g_enc_delta{0};
static std::atomic<bool> g_btn_pressed{false};
static std::atomic<bool> g_long_press_pending{false};
static lvgl_encoder_cb_t g_encoder_cb = nullptr;

static void on_rotate(int delta) {
    g_enc_delta += delta;
    if (g_encoder_cb) g_encoder_cb(delta, false, false);
}

static void on_press(bool is_long) {
    if (is_long) {
        g_long_press_pending = true;
        if (g_encoder_cb) g_encoder_cb(0, false, true);
    } else {
        g_btn_pressed = true;
        if (g_encoder_cb) g_encoder_cb(0, true, false);
    }
}

static void encoder_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    (void)indev;
    data->enc_diff = g_enc_delta.exchange(0);
    data->state = g_btn_pressed.exchange(false) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

extern "C" int lvgl_port_init(int gpio_handle, lvgl_encoder_cb_t encoder_cb) {
    g_gpio_handle = gpio_handle;
    g_encoder_cb = encoder_cb;

    lv_init();

#if LV_USE_LINUX_FBDEV
    g_disp = lv_linux_fbdev_create();
    if (!g_disp) return -1;
    lv_linux_fbdev_set_file(g_disp, "/dev/fb0");
    lv_linux_fbdev_set_force_refresh(g_disp, false);
#endif

    g_indev = lv_indev_create();
    if (!g_indev) return -2;
    lv_indev_set_type(g_indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(g_indev, encoder_read_cb);
    lv_indev_set_display(g_indev, g_disp);

    g_group = lv_group_create();
    lv_indev_set_group(g_indev, g_group);

    g_encoder = new bjj::RotaryEncoder(on_rotate, on_press);
    g_encoder->init(gpio_handle);

    return 0;
}

extern "C" void lvgl_port_deinit(int gpio_handle) {
    if (g_encoder) {
        g_encoder->freeGpio(gpio_handle);
        delete g_encoder;
        g_encoder = nullptr;
    }
    if (g_group) {
        lv_group_delete(g_group);
        g_group = nullptr;
    }
    if (g_indev) {
        lv_indev_delete(g_indev);
        g_indev = nullptr;
    }
    if (g_disp) {
        lv_display_delete(g_disp);
        g_disp = nullptr;
    }
    lv_deinit();
}

extern "C" lv_display_t* lvgl_port_get_display(void) {
    return g_disp;
}

extern "C" lv_indev_t* lvgl_port_get_indev(void) {
    return g_indev;
}

extern "C" lv_group_t* lvgl_port_get_group(void) {
    return g_group;
}

extern "C" void lvgl_port_encoder_poll(void) {
    if (g_encoder) g_encoder->poll();
}

extern "C" void lvgl_port_encoder_add_delta(int delta) {
    g_enc_delta += delta;
}

extern "C" void lvgl_port_encoder_set_pressed(bool pressed) {
    g_btn_pressed = pressed;
}

extern "C" bool lvgl_port_encoder_get_long_press_pending(void) {
    return g_long_press_pending.exchange(false);
}

extern "C" void lvgl_port_encoder_clear_long_press(void) {
    g_long_press_pending = false;
}
