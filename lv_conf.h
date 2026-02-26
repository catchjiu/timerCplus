/**
 * LVGL Configuration - BJJ Gym Timer
 * Raspberry Pi /dev/fb0 - Combat Sports Theme
 * Optimized for embedded Linux framebuffer
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
 * COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

/*====================
 * MEMORY
 *====================*/
#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_BUILTIN

#define LV_MEM_SIZE (128 * 1024U)
#define LV_MEM_POOL_EXPAND_SIZE 0
#define LV_MEM_ADR 0

/*====================
 * HAL
 *====================*/
#define LV_DEF_REFR_PERIOD 33
#define LV_DPI_DEF 130

/*====================
 * OS (none - bare metal style)
 *====================*/
#define LV_USE_OS LV_OS_NONE

/*====================
 * RENDERING
 *====================*/
#define LV_DRAW_BUF_STRIDE_ALIGN 1
#define LV_DRAW_BUF_ALIGN 4
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE (48 * 1024)
#define LV_DRAW_LAYER_MAX_MEMORY 0

#define LV_USE_DRAW_SW 1
#if LV_USE_DRAW_SW
#define LV_USE_DRAW_SW_ASM LV_DRAW_SW_ASM_NONE
#define LV_DRAW_SW_SUPPORT_RGB565 1
#define LV_DRAW_SW_SUPPORT_RGB888 1
#define LV_DRAW_SW_SUPPORT_XRGB8888 1
#define LV_DRAW_SW_DRAW_UNIT_CNT 1
#endif

/*====================
 * ASSERTS
 *====================*/
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1

/*====================
 * WIDGETS - Minimal set for timer
 *====================*/
#define LV_USE_ANIMIMG 0
#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_BUTTON 1
#define LV_USE_BUTTONMATRIX 0
#define LV_USE_CALENDAR 0
#define LV_USE_CANVAS 0
#define LV_USE_CHART 0
#define LV_USE_CHECKBOX 0
#define LV_USE_DROPDOWN 1
#define LV_USE_IMAGE 1
#define LV_USE_IMAGEBUTTON 0
#define LV_USE_KEYBOARD 0
#define LV_USE_LABEL 1
#define LV_USE_LED 0
#define LV_USE_LINE 0
#define LV_USE_LIST 0
#define LV_USE_MENU 0
#define LV_USE_MSGBOX 0
#define LV_USE_ROLLER 1
#define LV_USE_SCALE 0
#define LV_USE_SLIDER 0
#define LV_USE_SPAN 0
#define LV_USE_SPINBOX 0
#define LV_USE_SPINNER 0
#define LV_USE_SWITCH 0
#define LV_USE_TABLE 0
#define LV_USE_TABVIEW 0
#define LV_USE_TEXTAREA 1
#define LV_USE_TILEVIEW 0
#define LV_USE_WIN 0

/*====================
 * THEMES
 *====================*/
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
#define LV_THEME_DEFAULT_GROW 1
#define LV_THEME_DEFAULT_TRANSITION_TIME 0
#endif
#define LV_USE_THEME_SIMPLE 0
#define LV_USE_THEME_MONO 0

/*====================
 * LAYOUT
 *====================*/
#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/*====================
 * FONTS - Large clock font
 *====================*/
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_48 1
#define LV_FONT_DEFAULT &lv_font_montserrat_48

/*====================
 * LINUX FRAMEBUFFER
 *====================*/
#define LV_USE_LINUX_FBDEV 1
#if LV_USE_LINUX_FBDEV
#define LV_LINUX_FBDEV_BSD 0
#define LV_LINUX_FBDEV_RENDER_MODE LV_DISPLAY_RENDER_MODE_DIRECT
#define LV_LINUX_FBDEV_BUFFER_COUNT 2
#define LV_LINUX_FBDEV_BUFFER_SIZE 25
#define LV_LINUX_FBDEV_MMAP 1
#endif

/*====================
 * LOG
 *====================*/
#define LV_USE_LOG 0

/*====================
 * DEMOS (disabled)
 *====================*/
#define LV_USE_DEMO_WIDGETS 0
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0
#define LV_USE_DEMO_BENCHMARK 0

#endif /* LV_CONF_H */
