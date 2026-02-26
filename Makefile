# BJJ Gym Timer - Raspberry Pi
# Requires: pigpio (sudo apt install pigpio)
# Build: make
# Run: sudo ./bjj_timer (pigpio needs root for hardware access)

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS = -lpigpio -lpthread -lrt

TARGET = bjj_timer
SRCS = main.cpp timer_logic.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp hardware.hpp timer_logic.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

# Note: Run 'sudo pigpiod' before starting the timer if not using systemd
