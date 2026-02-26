# BJJ Gym Timer - LVGL GUI
# Requires: LVGL submodule, liblgpio
#
# Setup:
#   git submodule add https://github.com/lvgl/lvgl.git lvgl
#   git submodule update --init --recursive
#
# Build:
#   mkdir build && cd build
#   cmake ..
#   make
#
# Run: sudo ./bjj_timer_gui

cmake_minimum_required(VERSION 3.16)
project(bjj_timer_gui CXX C)

set(CMAKE_CXX_STANDARD 17)

# LVGL
set(LVGL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lvgl)
set(LV_BUILD_CONF_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h CACHE PATH "Path to lv_conf.h")
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "Disable LVGL demos")
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "Disable LVGL examples")

if(NOT EXISTS ${LVGL_DIR}/lvgl.h)
  message(FATAL_ERROR "LVGL not found. Run: git submodule add https://github.com/lvgl/lvgl.git lvgl && git submodule update --init")
endif()

add_subdirectory(${LVGL_DIR})

# Our sources
set(SRCS
  main_lvgl.cpp
  timer_logic.cpp
  ui.cpp
  lvgl_port.cpp
)

add_executable(bjj_timer_gui ${SRCS})

target_include_directories(bjj_timer_gui PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${LVGL_DIR}
)

find_package(SDL2 REQUIRED)

target_link_libraries(bjj_timer_gui
  PRIVATE
  lvgl
  lgpio
  SDL2::SDL2
  pthread
)

# LVGL config comes from add_subdirectory(lvgl) via LV_BUILD_CONF_PATH
