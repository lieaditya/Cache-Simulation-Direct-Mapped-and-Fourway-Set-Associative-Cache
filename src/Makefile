# ---------------------------------------
# CONFIGURATION BEGIN
# ---------------------------------------

# Entry point for the program
C_SRCS = main.c
CPP_SRCS = simulation.cpp cache_base.cpp cache_module.cpp direct_mapped_cache.cpp four_way_lru_cache.cpp main_memory.cpp

# Object files located in the output directory outside src
C_OBJS = $(patsubst %.c,../out/%.o,$(C_SRCS))
CPP_OBJS = $(patsubst %.cpp,../out/%.o,$(CPP_SRCS))

# Header files directory
HEADERS_DIR := ../includes/

# Target name
TARGET := ../out/simulation

# Path to your systemc installation (adjust as needed)
SCPATH = ../../systemc

# Additional flags for the compiler
CXXFLAGS := -std=c++14 -I$(SCPATH)/include -I$(HEADERS_DIR) -L$(SCPATH)/lib -lsystemc -lm -Wall

# ---------------------------------------
# CONFIGURATION END
# ---------------------------------------

# Determine if clang or gcc as well as clang++ or g++ are available
CXX := $(shell command -v g++ || command -v clang++)
ifeq ($(strip $(CXX)),)
    $(error Neither clang++ nor g++ is available. Exiting.)
endif

CC := $(shell command -v gcc || command -v clang)
ifeq ($(strip $(CC)),)
    $(error Neither clang nor gcc is available. Exiting.)
endif

# Add rpath according to OS
UNAME_S := $(shell uname -s)

ifneq ($(UNAME_S), Darwin)
    CXXFLAGS += -Wl,-rpath=$(SCPATH)/lib
endif

ifeq ($(UNAME_S), Darwin)
    CXXFLAGS += -Wl,-rpath $(SCPATH)/lib
endif

# Default to release build for both app and library
all: debug

# Ensure the output directory exists
$(shell mkdir -p ../out)

# Rule to compile .c files to .o files
../out/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to compile .cpp files to .o files
../out/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Debug build
debug: CXXFLAGS += -g
debug: $(TARGET)

# Release build
release: CXXFLAGS += -O2
release: $(TARGET)

# Rule to link object files to executable
$(TARGET): $(C_OBJS) $(CPP_OBJS)
	$(CXX) $(CXXFLAGS) $(C_OBJS) $(CPP_OBJS) $(LDFLAGS) -o $(TARGET)

# Clean up
clean:
	rm -f $(TARGET)
	rm -rf ../out/*.o
	rm -rf ../out/*.vcd

.PHONY: all debug release clean