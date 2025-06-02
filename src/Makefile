# Define your compiler
CXX = g++
# CXX = clang++ # Uncomment for clang

# Define compiler flags
CXXFLAGS = -Wall -Wextra -std=c++17 -I./oscar/src/include
# Add SFML include directories
CXXFLAGS += -I/usr/local/include # Adjust if SFML headers are in a different location

# Define linker flags
LDFLAGS =
# Add SFML linker flags and libraries
LDFLAGS += -L/usr/local/lib -lsfml-graphics -lsfml-audio -lsfml-system # Adjust if SFML libs are in a different location

# --- OSCPACK INTEGRATION ---
OSCPACK_DIR = libs/oscpack

# OSCPACK include directories
# These paths are relative to where your Makefile is. Adjust if necessary.
OSCPACK_INC_DIRS = \
    -I$(OSCPACK_DIR)/src \
    -I$(OSCPACK_DIR)/src/ip \
    -I$(OSCPACK_DIR)/src/osc

# Add OSCPACK includes to CXXFLAGS
CXXFLAGS += $(OSCPACK_INC_DIRS)

# OSCPACK source files
# List the .cpp files you need from oscpack.
# This is a common set, but you might need more or fewer depending on your usage.
OSCPACK_SRCS = \
    $(OSCPACK_DIR)/src/ip/IpEndpointName.cpp \
    $(OSCPACK_DIR)/src/ip/NetworkingUtils.cpp \
    $(OSCPACK_DIR)/src/ip/PacketListener.cpp \
    $(OSCPACK_DIR)/src/ip/Timer.cpp \
    $(OSCPACK_DIR)/src/ip/UdpSocket.cpp \
    $(OSCPACK_DIR)/src/osc/OscOutboundPacketStream.cpp \
    $(OSCPACK_DIR)/src/osc/OscReceivedElement.cpp \
    $(OSCPACK_DIR)/src/osc/OscPrintReceivedElements.cpp \
    $(OSCPACK_DIR)/src/osc/OscHostEndianness.cpp

# Convert OSCPACK source files to object files
OSCPACK_OBJS = $(OSCPACK_SRCS:.cpp=.o)

# --- Your Project Source Files ---
# Your project's source files
PROJECT_SRCS = \
    oscar/src/main.cpp \
    oscar/src/oscilloscope.cpp

# Convert your project's source files to object files
PROJECT_OBJS = $(PROJECT_SRCS:.cpp=.o)

# All object files for the final executable
ALL_OBJS = $(PROJECT_OBJS) $(OSCPACK_OBJS)

# Executable name
TARGET = oscar_oscilloscope

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(ALL_OBJS)
	$(CXX) $(ALL_OBJS) $(LDFLAGS) -o $@

# Rule to compile .cpp files into .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(ALL_OBJS) $(TARGET)