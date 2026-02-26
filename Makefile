# BJJ Gym Timer
# CLI version: make
# LVGL GUI: use CMake (see CMakeLists.txt)

# ========== CLI (default) ==========
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS = -llgpio -lpthread

TARGET = bjj_timer
SRCS = main.cpp timer_logic.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean cli

all: cli

cli: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp hardware.hpp timer_logic.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

# ========== LVGL GUI ==========
# Prereq: ./setup_lvgl.sh  (or: git submodule add https://github.com/lvgl/lvgl.git lvgl && git submodule update --init)
# Build:  mkdir -p build && cd build && cmake .. && make
# Run:    sudo ./bjj_timer_gui  (on Raspberry Pi with /dev/fb0)
