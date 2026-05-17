CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pthread -Iinclude

BUILD_DIR := build
TARGET := $(BUILD_DIR)/rt_inspection
SRCS := $(wildcard src/*.cpp)

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRCS)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) surface_points.csv
