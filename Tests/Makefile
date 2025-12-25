# CppUTest Makefile for STM32 Menu State Machine Tests

# Project paths
PROJECT_ROOT = ..
CORE_SRC = $(PROJECT_ROOT)/Core/Src
MOCKS_DIR = Mocks

# CppUTest configuration
CPPUTEST_HOME = cpputest
CPPUTEST_BUILD = $(CPPUTEST_HOME)/build

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

CXXFLAGS += -Wall -Wextra -std=c++11 -g
CFLAGS += -Wall -Wextra -std=c11 -g

# Linker flags
LD_LIBRARIES = $(CPPUTEST_BUILD)/src/CppUTest/libCppUTest.a $(CPPUTEST_BUILD)/src/CppUTestExt/libCppUTestExt.a -lstdc++ -lpthread

# Test targets
TESTS = menu_state_machine_test

# Source files
MENU_STATE_MACHINE_SRC = $(CORE_SRC)/menu_state_machine.c
MENU_STATE_MACHINE_TEST_SRC = menu_state_machine/MenuStateMachineTest.cpp
MENU_STATE_MACHINE_MOCK_SRC = $(MOCKS_DIR)/MockHAL.cpp

# Object files
MENU_STATE_MACHINE_OBJS = menu_state_machine.o MenuStateMachineTest.o MockHAL.o

.PHONY: all clean test

all: $(TESTS)

menu_state_machine_test: $(MENU_STATE_MACHINE_OBJS)
	$(CXX) $^ -o $@ $(LD_LIBRARIES)

menu_state_machine.o: $(MENU_STATE_MACHINE_SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

MenuStateMachineTest.o: $(MENU_STATE_MACHINE_TEST_SRC)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

MockHAL.o: $(MENU_STATE_MACHINE_MOCK_SRC)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

test: $(TESTS)
	@echo "Running menu_state_machine tests..."
	@./menu_state_machine_test -c

clean:
	rm -f $(TESTS) *.o

# Additional helpful targets
verbose_test: $(TESTS)
	@echo "Running menu_state_machine tests with verbose output..."
	@./menu_state_machine_test -v

list_tests: $(TESTS)
	@./menu_state_machine_test -lg
