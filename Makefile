CXX := g++

CXXFLAGS := -std=c++17 -Wall -Wextra -pthread -Iinclude
LDLIBS := -lmosquitto

BUILD_DIR := build
TARGET := $(BUILD_DIR)/rt_inspection
BENCHMARK_TARGET := $(BUILD_DIR)/benchmark_temporal

SRCS := $(wildcard src/*.cpp)

.PHONY: all run benchmark clean

all: $(TARGET)

$(TARGET): $(SRCS)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

$(BENCHMARK_TARGET): tests/benchmark_temporal.cpp \
		src/distancia_percorrida.cpp src/reconstrucao_superficie.cpp \
		src/coletor_dados.cpp src/inspecao_camera.cpp src/log.cpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $(BENCHMARK_TARGET)

benchmark: $(BENCHMARK_TARGET)
	cd /tmp && "$(CURDIR)/$(BENCHMARK_TARGET)"

clean:
	rm -rf $(BUILD_DIR) surface_points.csv
