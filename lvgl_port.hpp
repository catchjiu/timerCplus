/**
 * LVGL Port - Display + Encoder Input
 * Raspberry Pi: /dev/fb0 + GPIO encoder
 */

#pragma once

#include "hardware.hpp"

#ifdef __cplusplus
extern "C" {
#endif

struct _lv_display_t;
struct _lv_indev_t;
struct _lv_indev_data_t;
struct _lv_group_t;

typedef struct _lv_display_t lv_display_t;
typedef struct _lv_indev_t lv_indev_t;
typedef struct _lv_indev_data_t lv_indev_data_t;
typedef struct _lv_group_t lv_group_t;

// Encoder event callback: void fn(int delta, bool pressed, bool long_press)
typedef void (*lvgl_encoder_cb_t)(int delta, bool pressed, bool long_press);

// Initialize display (framebuffer) and input (encoder)
// encoder_cb: called with delta/pressed/long_press - can drive timer logic
// Returns 0 on success
int lvgl_port_init(int gpio_handle, lvgl_encoder_cb_t encoder_cb);

// Cleanup
void lvgl_port_deinit(int gpio_handle);

// Get LVGL display (for UI)
lv_display_t* lvgl_port_get_display(void);

// Get LVGL input device
lv_indev_t* lvgl_port_get_indev(void);

// Get group for encoder focus (add widgets to this)
lv_group_t* lvgl_port_get_group(void);

// Must be called every main loop - polls GPIO encoder into LVGL
void lvgl_port_encoder_poll(void);

// Set encoder delta (from hardware callback)
void lvgl_port_encoder_add_delta(int delta);

// Set button state for LVGL (short press = click)
void lvgl_port_encoder_set_pressed(bool pressed);

// Long press - not fed to LVGL, handled by app
bool lvgl_port_encoder_get_long_press_pending(void);
void lvgl_port_encoder_clear_long_press(void);

#ifdef __cplusplus
}
#endif
