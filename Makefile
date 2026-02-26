# BJJ Gym Timer - Raspberry Pi 5 (lgpio)
# Install: sudo apt install liblgpio-dev
# Build: make
# Run: sudo ./bjj_timer

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS = -llgpio -lpthread

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
