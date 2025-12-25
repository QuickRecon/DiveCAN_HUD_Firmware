# CppUTest Makefile for STM32 Menu State Machine Tests

# Project paths
PROJECT_ROOT = ..
CORE_SRC = $(PROJECT_ROOT)/Core/Src
MOCKS_DIR = Mocks
BUILD_DIR = build

# CppUTest configuration
CPPUTEST_HOME = cpputest
CPPUTEST_BUILD = build/cpputest

# CppUTest libraries
CPPUTEST_LIBS = $(CPPUTEST_BUILD)/src/CppUTest/libCppUTest.a \
                $(CPPUTEST_BUILD)/src/CppUTestExt/libCppUTestExt.a

# Compiler
CXX = g++
CC = gcc

# Compiler flags
# Note: Put MOCKS_DIR first so mock headers override real ones
CPPFLAGS += -I$(CPPUTEST_HOME)/include
CPPFLAGS += -I$(MOCKS_DIR)
CPPFLAGS += -I$(CORE_SRC)
CPPFLAGS += -DTESTING
CPPFLAGS += -Wno-unused-parameter

CXXFLAGS += -Wall -Wextra -std=c++11 -g -Og
CFLAGS += -Wall -Wextra -std=c11 -g

# Linker flags
LD_LIBRARIES = $(CPPUTEST_BUILD)/src/CppUTest/libCppUTest.a $(CPPUTEST_BUILD)/src/CppUTestExt/libCppUTestExt.a -lstdc++ -lpthread

# Test targets
TESTS = $(BUILD_DIR)/menu_state_machine_test $(BUILD_DIR)/hudcontrol_test

# Source files - Menu State Machine
MENU_STATE_MACHINE_SRC = $(CORE_SRC)/menu_state_machine.c
MENU_STATE_MACHINE_TEST_SRC = menu_state_machine/MenuStateMachineTest.cpp
MENU_STATE_MACHINE_MOCK_SRC = $(MOCKS_DIR)/MockHAL.cpp

# Source files - HUDControl
HUDCONTROL_SRC = $(CORE_SRC)/HUDControl.c
HUDCONTROL_TEST_SRC = HUDControl/HUDControlTest.cpp
HUDCONTROL_MOCK_SRC = $(MOCKS_DIR)/MockQueue.cpp $(MOCKS_DIR)/MockLEDs.cpp

# Object files
MENU_STATE_MACHINE_OBJS = $(BUILD_DIR)/menu_state_machine.o $(BUILD_DIR)/MenuStateMachineTest.o $(BUILD_DIR)/MockHAL.o
HUDCONTROL_OBJS = $(BUILD_DIR)/HUDControl.o $(BUILD_DIR)/HUDControlTest.o $(BUILD_DIR)/MockQueue.o $(BUILD_DIR)/MockLEDs.o $(BUILD_DIR)/MockHAL.o

.PHONY: all clean clean_all test verbose_test list_tests

all: $(TESTS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build CppUTest libraries if they don't exist
$(CPPUTEST_LIBS):
	@echo "Building CppUTest libraries..."
	@mkdir -p $(CPPUTEST_BUILD)
	@cd $(CPPUTEST_HOME) && cmake -B ../$(CPPUTEST_BUILD) -S .
	@cmake --build $(CPPUTEST_BUILD)
	@echo "CppUTest libraries built successfully"

$(BUILD_DIR)/menu_state_machine_test: $(MENU_STATE_MACHINE_OBJS) | $(CPPUTEST_LIBS)
	$(CXX) $^ -o $@ $(LD_LIBRARIES)

$(BUILD_DIR)/hudcontrol_test: $(HUDCONTROL_OBJS) | $(CPPUTEST_LIBS)
	$(CXX) $^ -o $@ $(LD_LIBRARIES)

$(BUILD_DIR)/menu_state_machine.o: $(MENU_STATE_MACHINE_SRC) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/MenuStateMachineTest.o: $(MENU_STATE_MACHINE_TEST_SRC) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/HUDControl.o: $(HUDCONTROL_SRC) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/HUDControlTest.o: $(HUDCONTROL_TEST_SRC) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/MockHAL.o: $(MOCKS_DIR)/MockHAL.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/MockQueue.o: $(MOCKS_DIR)/MockQueue.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/MockLEDs.o: $(MOCKS_DIR)/MockLEDs.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

test: $(TESTS)
	@echo "Running menu_state_machine tests..."
	@$(BUILD_DIR)/menu_state_machine_test -c
	@echo ""
	@echo "Running HUDControl tests..."
	@$(BUILD_DIR)/hudcontrol_test -c

clean:
	rm -rf $(BUILD_DIR)

# Note: clean already removes $(BUILD_DIR) which includes build/cpputest
clean_all: clean

# Additional helpful targets
verbose_test: $(TESTS)
	@echo "Running menu_state_machine tests with verbose output..."
	@$(BUILD_DIR)/menu_state_machine_test -v
	@echo ""
	@echo "Running HUDControl tests with verbose output..."
	@$(BUILD_DIR)/hudcontrol_test -v

list_tests: $(TESTS)
	@echo "=== menu_state_machine test groups ==="
	@$(BUILD_DIR)/menu_state_machine_test -lg
	@echo ""
	@echo "=== HUDControl test groups ==="
	@$(BUILD_DIR)/hudcontrol_test -lg
