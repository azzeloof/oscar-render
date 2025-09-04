# Compiler
CXX = g++

# Compiler flags
# --- ADDED: -Wno-vla-cxx-extension to silence warnings from RtAudio ---
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -O2 -g -MMD -MP -Wno-vla-cxx-extension

# Define OS_UNAME once to be used throughout the Makefile
OS_UNAME := $(shell uname -s)

# Include paths
# --- CORRECTED: Switched from ALSA to JACK for better PipeWire compatibility ---
CPPFLAGS = \
	-I./oscar/src/include \
	-I./libs/oscpack \
	-I./libs/oscpack/ip \
	-I./libs/oscpack/osc \
	-I./libs/rtaudio \
	-D__UNIX_JACK__

# Linker flags
LDFLAGS =

# --- Apple Silicon Mac-specific paths ---
ifeq ($(OS_UNAME), Darwin)
	ifeq ($(shell uname -m), arm64)
		# Add include paths for sfml, asio, and jack
		CPPFLAGS += -I/opt/homebrew/opt/sfml/include
		CPPFLAGS += -I/opt/homebrew/opt/asio/include
		CPPFLAGS += -I/opt/homebrew/opt/jack/include

		# Add library paths for sfml and jack
		LDFLAGS += -L/opt/homebrew/opt/sfml/lib
		LDFLAGS += -L/opt/homebrew/opt/jack/lib
	endif
endif

# SFML libraries
SFML_LIBS = -lsfml-graphics -lsfml-window -lsfml-system

# Other libraries
# --- CORRECTED: Added -ljack and made libraries OS-specific ---
ifeq ($(OS_UNAME), Darwin)
    # macOS doesn't have ALSA (-lasound)
    OTHER_LIBS = -lpthread -ljack
else
    # Default to Linux setup
    OTHER_LIBS = -lasound -lpthread -ljack
endif


# --- OSCPACK INTEGRATION ---
OSCPACK_DIR = libs/oscpack
OSCPACK_IP_SYSTEM_PATH_DEFAULT = $(OSCPACK_DIR)/ip/posix
OSCPACK_IP_SYSTEM_PATH = $(OSCPACK_IP_SYSTEM_PATH_DEFAULT)
ifeq ($(OS_UNAME),Darwin)
	OSCPACK_IP_SYSTEM_PATH = $(OSCPACK_DIR)/ip/posix
else ifeq ($(OS_UNAME),Linux)
	OSCPACK_IP_SYSTEM_PATH = $(OSCPACK_DIR)/ip/posix
else ifneq (,$(findstring MINGW,$(OS_UNAME)))
	OSCPACK_IP_SYSTEM_PATH = $(OSCPACK_DIR)/ip/win32
else ifneq (,$(findstring MSYS,$(OS_UNAME)))
	OSCPACK_IP_SYSTEM_PATH = $(OSCPACK_DIR)/ip/win32
else
  ifdef COMSPEC
	OSCPACK_IP_SYSTEM_PATH = $(OSCPACK_DIR)/ip/win32
  else ifdef WINDIR
	OSCPACK_IP_SYSTEM_PATH = $(OSCPACK_DIR)/ip/win32
  endif
endif

# --- FIXED: Added missing OscTypes.cpp to the source list ---
OSCPACK_SRCS = \
	$(OSCPACK_DIR)/ip/IpEndpointName.cpp \
	$(OSCPACK_IP_SYSTEM_PATH)/NetworkingUtils.cpp \
	$(OSCPACK_IP_SYSTEM_PATH)/UdpSocket.cpp \
	$(OSCPACK_DIR)/osc/OscOutboundPacketStream.cpp \
	$(OSCPACK_DIR)/osc/OscReceivedElements.cpp \
	$(OSCPACK_DIR)/osc/OscPrintReceivedElements.cpp \
	$(OSCPACK_DIR)/osc/OscTypes.cpp


# --- RtAudio Source Files ---
RTAUDIO_DIR = libs/rtaudio
RTAUDIO_SRCS = $(wildcard $(RTAUDIO_DIR)/*.cpp)

# --- Project Source Files ---
SRCS = oscar/src/main.cpp oscar/src/oscilloscope.cpp oscar/src/osc.cpp

# Combine all source files
ALL_SRCS = $(SRCS) $(OSCPACK_SRCS) $(RTAUDIO_SRCS)

# Build directory name
TARGET_DIR = build
TARGET = $(TARGET_DIR)/oscar_render
OBJS = $(addprefix $(TARGET_DIR)/, $(notdir $(ALL_SRCS:.cpp=.o)))
DEPS = $(OBJS:.o=.d)
VPATH = . oscar/src $(OSCPACK_DIR) $(OSCPACK_DIR)/ip $(OSCPACK_DIR)/osc $(OSCPACK_DIR)/ip/posix $(OSCPACK_DIR)/ip/win32 $(RTAUDIO_DIR)

# Default target
all: $(TARGET)

# Rule to link the executable
$(TARGET): $(OBJS)
	@echo "Linking: $@"
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(SFML_LIBS) $(OTHER_LIBS)

# Generic rule to compile .cpp files from VPATH into TARGET_DIR
$(TARGET_DIR)/%.o: %.cpp
	@echo "Compiling (generic): $<  ->  $@"
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# --- Explicit Rules for OSCPACK Sources ---
define compile_oscpack_src
_CURRENT_OSCPACK_SRC := $(1)
_CURRENT_OSCPACK_OBJ := $(TARGET_DIR)/$(notdir $(1:.cpp=.o))
$(_CURRENT_OSCPACK_OBJ): $(_CURRENT_OSCPACK_SRC)
	@echo "Compiling OSCPACK: $(_CURRENT_OSCPACK_SRC)  ->  $(_CURRENT_OSCPACK_OBJ)"
	@mkdir -p $(dir $(_CURRENT_OSCPACK_OBJ))
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $(_CURRENT_OSCPACK_SRC) -o $(_CURRENT_OSCPACK_OBJ)
endef

$(foreach s,$(OSCPACK_SRCS),\
	$(eval $(call compile_oscpack_src,$(s))))

# Clean up
clean:
	@echo "Cleaning up build directory: $(TARGET_DIR)"
	rm -rf $(TARGET_DIR)

# Include dependency files
-include $(DEPS)

# Phony targets
.PHONY: all clean


