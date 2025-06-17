# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -O2 -g -MMD -MP

# Include paths
CPPFLAGS = \
	-I./oscar/src/include \
	-I./libs/oscpack \
	-I./libs/oscpack/ip \
	-I./libs/oscpack/osc

# Linker flags
LDFLAGS =

# SFML libraries
SFML_LIBS = -lsfml-graphics -lsfml-window -lsfml-audio -lsfml-system

# Other libraries
OTHER_LIBS =

# --- OSCPACK INTEGRATION ---
OSCPACK_DIR = libs/oscpack

# OS detection for platform-specific oscpack files
# Default to POSIX, then check for specific OS types
OSCPACK_IP_SYSTEM_PATH_DEFAULT = $(OSCPACK_DIR)/ip/posix
OSCPACK_IP_SYSTEM_PATH = $(OSCPACK_IP_SYSTEM_PATH_DEFAULT) # Default

# Attempt to get OS using uname
OS_UNAME := $(shell uname -s)

ifeq ($(OS_UNAME),Darwin) # macOS
	OSCPACK_IP_SYSTEM_PATH = $(OSCPACK_DIR)/ip/posix
else ifeq ($(OS_UNAME),Linux)
	OSCPACK_IP_SYSTEM_PATH = $(OSCPACK_DIR)/ip/posix
else ifneq (,$(findstring MINGW,$(OS_UNAME))) # MinGW (e.g., MINGW32_NT, MINGW64_NT)
	OSCPACK_IP_SYSTEM_PATH = $(OSCPACK_DIR)/ip/win32
else ifneq (,$(findstring MSYS,$(OS_UNAME))) # MSYS (e.g., MSYS_NT)
	OSCPACK_IP_SYSTEM_PATH = $(OSCPACK_DIR)/ip/win32
else ifdef COMSPEC # Generic Windows check (for cmd.exe)
	OSCPACK_IP_SYSTEM_PATH = $(OSCPACK_DIR)/ip/win32
else ifdef WINDIR # Another generic Windows check
	OSCPACK_IP_SYSTEM_PATH = $(OSCPACK_DIR)/ip/win32
endif

$(info --- OSCPACK Platform Detection ---)
$(info OS_UNAME: [$(OS_UNAME)])
$(info OSCPACK_IP_SYSTEM_PATH set to: [$(OSCPACK_IP_SYSTEM_PATH)])
$(info --- End OSCPACK Platform Detection ---)

OSCPACK_SRCS = \
	$(OSCPACK_DIR)/ip/IpEndpointName.cpp \
	$(OSCPACK_IP_SYSTEM_PATH)/NetworkingUtils.cpp \
	$(OSCPACK_IP_SYSTEM_PATH)/UdpSocket.cpp \
	$(OSCPACK_DIR)/osc/OscOutboundPacketStream.cpp \
	$(OSCPACK_DIR)/osc/OscReceivedElements.cpp \
	$(OSCPACK_DIR)/osc/OscPrintReceivedElements.cpp 
# Note: Check if OscHostEndianness.cpp is also platform-specific from CMakeLists.txt if issues persist.
# CMakeLists.txt shows: osc/OscHostEndianness.h, osc/OscHostEndianness.cpp (seems common)

$(info --- OSCPACK_SRCS ---)
$(foreach s,$(OSCPACK_SRCS),$(info Source: $(s)))
$(info --- End OSCPACK_SRCS ---)

# --- Project Source Files ---
SRCS = oscar/src/main.cpp oscar/src/oscilloscope.cpp oscar/src/osc.cpp

# Combine all source files
ALL_SRCS = $(SRCS) $(OSCPACK_SRCS)

# Build directory name
TARGET_DIR = build

# Executable target path
TARGET = $(TARGET_DIR)/oscar_render

# Object files
OBJS = $(addprefix $(TARGET_DIR)/, $(notdir $(ALL_SRCS:.cpp=.o)))

# Dependency files
DEPS = $(OBJS:.o=.d)

# VPATH: Search paths for prerequisites
# Add platform specific paths to VPATH if make needs to find sources there via generic rules,
# but our explicit rules for OSCPACK should use the full calculated paths.
VPATH = . oscar/src $(OSCPACK_DIR) $(OSCPACK_DIR)/ip $(OSCPACK_DIR)/osc $(OSCPACK_DIR)/ip/posix $(OSCPACK_DIR)/ip/win32

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

#$(info --- Defining rule for OSCPACK ---)
#$(info Source (param 1): $(_CURRENT_OSCPACK_SRC))
#$(info Object (calculated): $(_CURRENT_OSCPACK_OBJ))
#$(info Target Dir (for mkdir): $(dir $(_CURRENT_OSCPACK_OBJ)))
#$(info --- End rule definition info ---)

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
