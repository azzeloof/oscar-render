# Compiler
CXX = g++

# Compiler flags
# -std=c++20: Use C++20 standard
# -Wall: Enable all common warnings
# -Wextra: Enable extra warnings
# -Wpedantic: Issue all warnings demanded by strict ISO C++
# -O2: Optimization level 2 (common for release)
# -g: Include debugging symbols (useful for development)
# -MMD -MP: Generate dependency files (for header changes)
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -O2 -g -MMD -MP

# SFML specific include flags (e.g., -I/path/to/SFML/include or -I./include for local headers)
CPPFLAGS =

# Linker flags
LDFLAGS =

# SFML libraries to link against
SFML_LIBS = -lsfml-graphics -lsfml-window -lsfml-audio -lsfml-system

# Other libraries you might need
OTHER_LIBS = 

# Source files (assuming they are in the current directory, e.g., ./main.cpp)
SRCS = main.cpp oscilloscope.cpp

# Build directory name
TARGET_DIR = build

# Executable target path (e.g., build/oscilloscope_emulator)
TARGET = $(TARGET_DIR)/oscilloscope_emulator

# Object files: Prepend TARGET_DIR to the .o version of SRCS
# Results in paths like build/main.o build/oscilloscope.o
OBJS = $(addprefix $(TARGET_DIR)/, $(SRCS:.cpp=.o))

# Dependency files: Derived from OBJS paths, so they also go into TARGET_DIR
# Results in paths like build/main.d build/oscilloscope.d
DEPS = $(OBJS:.o=.d)

# Default target: build the executable
all: $(TARGET)

# Rule to link the executable from object files in TARGET_DIR
# $(TARGET) will be build/oscilloscope_emulator
# $(OBJS) will be build/main.o build/oscilloscope.o
$(TARGET): $(OBJS)
	@echo "Linking: $@"
	@mkdir -p $(@D) # Ensure directory for the TARGET executable itself exists (e.g., build/)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(SFML_LIBS) $(OTHER_LIBS)

# Rule to compile .cpp files (from project root) into .o files in TARGET_DIR
# $< is the source file (e.g., main.cpp)
# $@ is the target object file (e.g., build/main.o)
$(TARGET_DIR)/%.o: %.cpp
	@echo "Compiling: $<  ->  $@"
	@mkdir -p $(@D) # Ensure directory for the .o file exists (e.g., build/)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# Clean up: Remove the entire build directory
clean:
	@echo "Cleaning up build directory: $(TARGET_DIR)"
	rm -rf $(TARGET_DIR)

# Include dependency files (e.g., build/main.d) if they exist.
# The compiler (with -MMD -MP) will generate these in the same directory as the .o files.
-include $(DEPS)

# Phony targets (targets that don't represent actual files)
.PHONY: all clean