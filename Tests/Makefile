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
TESTS = $(BUILD_DIR)/menu_state_machine_test $(BUILD_DIR)/hudcontrol_test $(BUILD_DIR)/flash_test $(BUILD_DIR)/transciever_test $(BUILD_DIR)/divecan_test $(BUILD_DIR)/leds_test $(BUILD_DIR)/pwr_management_test $(BUILD_DIR)/printer_test

# Source files - Menu State Machine
MENU_STATE_MACHINE_SRC = $(CORE_SRC)/menu_state_machine.c
MENU_STATE_MACHINE_TEST_SRC = menu_state_machine/MenuStateMachineTest.cpp
MENU_STATE_MACHINE_MOCK_SRC = $(MOCKS_DIR)/MockHAL.cpp

# Source files - HUDControl
HUDCONTROL_SRC = $(CORE_SRC)/HUDControl.c
HUDCONTROL_TEST_SRC = HUDControl/HUDControlTest.cpp
HUDCONTROL_MOCK_SRC = $(MOCKS_DIR)/MockQueue.cpp $(MOCKS_DIR)/MockLEDs.cpp $(MOCKS_DIR)/MockPower.cpp

# Source files - Flash
FLASH_SRC = $(CORE_SRC)/Hardware/flash.c
FLASH_TEST_SRC = flash/FlashTest.cpp
FLASH_MOCK_SRC = $(MOCKS_DIR)/MockFlash.cpp $(MOCKS_DIR)/MockErrors.cpp

# Source files - Transciever
TRANSCIEVER_SRC = $(CORE_SRC)/DiveCAN/Transciever.c
TRANSCIEVER_TEST_SRC = Transciever/TranscieverTest.cpp
TRANSCIEVER_MOCK_SRC = $(MOCKS_DIR)/MockCAN.cpp $(MOCKS_DIR)/MockErrors.cpp $(MOCKS_DIR)/queue.cpp

# Source files - DiveCAN
DIVECAN_SRC = $(CORE_SRC)/DiveCAN/DiveCAN.c $(CORE_SRC)/DiveCAN/Transciever.c
DIVECAN_TEST_SRC = DiveCAN/DiveCANTest.cpp
DIVECAN_MOCK_SRC = $(MOCKS_DIR)/MockCAN.cpp $(MOCKS_DIR)/MockErrors.cpp $(MOCKS_DIR)/MockPower.cpp $(MOCKS_DIR)/queue.cpp $(MOCKS_DIR)/printer.cpp

# Source files - LEDs
LEDS_SRC = $(CORE_SRC)/Hardware/leds.c
LEDS_TEST_SRC = leds/LEDsTest.cpp
LEDS_MOCK_SRC = $(MOCKS_DIR)/MockHAL.cpp $(MOCKS_DIR)/MockDelay.cpp $(MOCKS_DIR)/queue.cpp

# Source files - Power Management
PWR_MANAGEMENT_SRC = $(CORE_SRC)/Hardware/pwr_management.c $(CORE_SRC)/Hardware/flash.c
PWR_MANAGEMENT_TEST_SRC = pwr_management/PwrManagementTest.cpp
PWR_MANAGEMENT_MOCK_SRC = $(MOCKS_DIR)/MockHAL.cpp $(MOCKS_DIR)/MockFlash.cpp $(MOCKS_DIR)/printer.cpp

# Source files - Printer
PRINTER_SRC = $(CORE_SRC)/Hardware/printer.c
PRINTER_TEST_SRC = printer/PrinterTest.cpp
PRINTER_MOCK_SRC = $(MOCKS_DIR)/queue.cpp

# Object files
MENU_STATE_MACHINE_OBJS = $(BUILD_DIR)/menu_state_machine.o $(BUILD_DIR)/MenuStateMachineTest.o $(BUILD_DIR)/MockHAL.o
HUDCONTROL_OBJS = $(BUILD_DIR)/HUDControl.o $(BUILD_DIR)/HUDControlTest.o $(BUILD_DIR)/MockQueue.o $(BUILD_DIR)/MockLEDs.o $(BUILD_DIR)/MockHAL.o $(BUILD_DIR)/MockPower_hud.o
FLASH_OBJS = $(BUILD_DIR)/flash.o $(BUILD_DIR)/FlashTest.o $(BUILD_DIR)/MockFlash.o $(BUILD_DIR)/MockErrors.o
TRANSCIEVER_OBJS = $(BUILD_DIR)/Transciever.o $(BUILD_DIR)/TranscieverTest.o $(BUILD_DIR)/MockCAN.o $(BUILD_DIR)/MockErrors.o $(BUILD_DIR)/queue.o
DIVECAN_OBJS = $(BUILD_DIR)/DiveCAN.o $(BUILD_DIR)/Transciever_divecan.o $(BUILD_DIR)/DiveCANTest.o $(BUILD_DIR)/MockCAN.o $(BUILD_DIR)/MockErrors.o $(BUILD_DIR)/MockPower.o $(BUILD_DIR)/queue.o $(BUILD_DIR)/printer.o
LEDS_OBJS = $(BUILD_DIR)/leds.o $(BUILD_DIR)/LEDsTest.o $(BUILD_DIR)/MockHAL.o $(BUILD_DIR)/MockDelay.o $(BUILD_DIR)/queue.o
PWR_MANAGEMENT_OBJS = $(BUILD_DIR)/pwr_management.o $(BUILD_DIR)/flash_pwr.o $(BUILD_DIR)/PwrManagementTest.o $(BUILD_DIR)/MockHAL.o $(BUILD_DIR)/MockFlash.o $(BUILD_DIR)/MockPower_pwr.o $(BUILD_DIR)/MockErrors.o $(BUILD_DIR)/printer.o
PRINTER_OBJS = $(BUILD_DIR)/printer_real.o $(BUILD_DIR)/PrinterTest.o $(BUILD_DIR)/queue.o

.PHONY: all clean clean_all test verbose_test list_tests

all: $(TESTS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build CppUTest libraries if they don' exist
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

$(BUILD_DIR)/flash_test: $(FLASH_OBJS) | $(CPPUTEST_LIBS)
	$(CXX) $^ -o $@ $(LD_LIBRARIES)

$(BUILD_DIR)/transciever_test: $(TRANSCIEVER_OBJS) | $(CPPUTEST_LIBS)
	$(CXX) $^ -o $@ $(LD_LIBRARIES)

$(BUILD_DIR)/divecan_test: $(DIVECAN_OBJS) | $(CPPUTEST_LIBS)
	$(CXX) $^ -o $@ $(LD_LIBRARIES)

$(BUILD_DIR)/leds_test: $(LEDS_OBJS) | $(CPPUTEST_LIBS)
	$(CXX) $^ -o $@ $(LD_LIBRARIES)

$(BUILD_DIR)/pwr_management_test: $(PWR_MANAGEMENT_OBJS) | $(CPPUTEST_LIBS)
	$(CXX) $^ -o $@ $(LD_LIBRARIES)

$(BUILD_DIR)/printer_test: $(PRINTER_OBJS) | $(CPPUTEST_LIBS)
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

$(BUILD_DIR)/MockPower_hud.o: $(MOCKS_DIR)/MockPower.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/flash.o: $(FLASH_SRC) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/FlashTest.o: $(FLASH_TEST_SRC) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/MockFlash.o: $(MOCKS_DIR)/MockFlash.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/MockErrors.o: $(MOCKS_DIR)/MockErrors.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/Transciever.o: $(TRANSCIEVER_SRC) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DTESTING_CAN -c $< -o $@

$(BUILD_DIR)/TranscieverTest.o: $(TRANSCIEVER_TEST_SRC) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DTESTING_CAN -c $< -o $@

$(BUILD_DIR)/MockCAN.o: $(MOCKS_DIR)/MockCAN.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/queue.o: $(MOCKS_DIR)/queue.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/DiveCAN.o: $(CORE_SRC)/DiveCAN/DiveCAN.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DTESTING -DTESTING_CAN -c $< -o $@

$(BUILD_DIR)/Transciever_divecan.o: $(CORE_SRC)/DiveCAN/Transciever.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DTESTING -DTESTING_CAN -c $< -o $@

$(BUILD_DIR)/DiveCANTest.o: $(DIVECAN_TEST_SRC) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DTESTING -DTESTING_CAN -c $< -o $@

$(BUILD_DIR)/MockPower.o: $(MOCKS_DIR)/MockPower.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/printer.o: $(MOCKS_DIR)/printer.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/leds.o: $(LEDS_SRC) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/LEDsTest.o: $(LEDS_TEST_SRC) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/pwr_management.o: $(PWR_MANAGEMENT_SRC) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/PwrManagementTest.o: $(PWR_MANAGEMENT_TEST_SRC) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/flash_pwr.o: $(CORE_SRC)/Hardware/flash.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/MockPower_pwr.o: $(MOCKS_DIR)/MockPower.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DTESTING_PWR_MANAGEMENT -c $< -o $@

$(BUILD_DIR)/MockDelay.o: $(MOCKS_DIR)/MockDelay.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/printer_real.o: $(PRINTER_SRC) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/PrinterTest.o: $(PRINTER_TEST_SRC) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

test: $(TESTS)
	@echo "Running menu_state_machine tests..."
	@$(BUILD_DIR)/menu_state_machine_test -c
	@echo ""
	@echo "Running HUDControl tests..."
	@$(BUILD_DIR)/hudcontrol_test -c
	@echo ""
	@echo "Running flash tests..."
	@$(BUILD_DIR)/flash_test -c
	@echo ""
	@echo "Running Transciever tests..."
	@$(BUILD_DIR)/transciever_test -c
	@echo ""
	@echo "Running DiveCAN tests..."
	@$(BUILD_DIR)/divecan_test -c
	@echo ""
	@echo "Running LEDs tests..."
	@$(BUILD_DIR)/leds_test -c
	@echo ""
	@echo "Running power management tests..."
	@$(BUILD_DIR)/pwr_management_test -c
	@echo ""
	@echo "Running printer tests..."
	@$(BUILD_DIR)/printer_test -c

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
	@echo ""
	@echo "Running flash tests with verbose output..."
	@$(BUILD_DIR)/flash_test -v
	@echo ""
	@echo "Running Transciever tests with verbose output..."
	@$(BUILD_DIR)/transciever_test -v
	@echo ""
	@echo "Running DiveCAN tests with verbose output..."
	@$(BUILD_DIR)/divecan_test -v

list_tests: $(TESTS)
	@echo "=== menu_state_machine test groups ==="
	@$(BUILD_DIR)/menu_state_machine_test -lg
	@echo ""
	@echo "=== HUDControl test groups ==="
	@$(BUILD_DIR)/hudcontrol_test -lg
	@echo ""
	@echo "=== flash test groups ==="
	@$(BUILD_DIR)/flash_test -lg
	@echo ""
	@echo "=== Transciever test groups ==="
	@$(BUILD_DIR)/transciever_test -lg
	@echo ""
	@echo "=== DiveCAN test groups ==="
	@$(BUILD_DIR)/divecan_test -lg
