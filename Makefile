CC = clang
CFLAGS = -Wall -pedantic-errors -std=c99 -pedantic
OPT_CFLAGS = -O2
DEBUG_CFLAGS = -g -DDEBUG_TRACE_EXECUTION -DDEBUG_DUMP_CODE -DDEBUG_CONST_TABLE_EXTRA

# Define the names of the executables
DEFAULT_TARGET = clox
DEBUG_TARGET = clox-dbg
RELEASE_TARGET = clox-release

SOURCES = $(notdir $(wildcard *.c))

# Define distinct object file names for each build type
OBJECT_DIR = bin
OBJECTS_DEFAULT = $(addprefix $(OBJECT_DIR)/default_,$(SOURCES:.c=.o))
OBJECTS_DEBUG = $(addprefix $(OBJECT_DIR)/debug_,$(SOURCES:.c=.o))
OBJECTS_RELEASE = $(addprefix $(OBJECT_DIR)/release_,$(SOURCES:.c=.o))

.PHONY: all debug release clean run test

# --- Main Build Targets ---

# 'all' target builds the default (non-optimized) and debug versions
all: $(DEFAULT_TARGET) $(DEBUG_TARGET)

# Rule for the default build (clox), using only CFLAGS
$(DEFAULT_TARGET): $(OBJECTS_DEFAULT)
	@echo "Linking $(DEFAULT_TARGET) (default)..."
	$(CC) $(CFLAGS) $(OBJECTS_DEFAULT) -o $@

# Rule for the debug build (clox-dbg), using CFLAGS + DEBUG_CFLAGS
$(DEBUG_TARGET): $(OBJECTS_DEBUG)
	@echo "Linking $(DEBUG_TARGET) (debug)..."
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) $(OBJECTS_DEBUG) -o $@

# Rule for the release build (clox-release), using CFLAGS + OPT_CFLAGS
$(RELEASE_TARGET): $(OBJECTS_RELEASE)
	@echo "Linking $(RELEASE_TARGET) (optimized release)..."
	$(CC) $(CFLAGS) $(OPT_CFLAGS) $(OBJECTS_RELEASE) -o $@

# --- Compilation Rules for Object Files ---

# Rule to compile source files into default object files (no optimization)
$(OBJECT_DIR)/default_%.o: %.c
	@mkdir -p $(OBJECT_DIR)
	@echo "Compiling $< for default build..."
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to compile source files into debug object files
$(OBJECT_DIR)/debug_%.o: %.c
	@mkdir -p $(OBJECT_DIR)
	@echo "Compiling $< for debug build..."
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -c $< -o $@

# Rule to compile source files into release object files (with optimization)
$(OBJECT_DIR)/release_%.o: %.c
	@mkdir -p $(OBJECT_DIR)
	@echo "Compiling $< for optimized release build..."
	$(CC) $(CFLAGS) $(OPT_CFLAGS) -c $< -o $@

# --- Utility Targets ---

# 'debug' target explicitly builds only the debug version
debug: $(DEBUG_TARGET)

# 'release' target explicitly builds only the optimized release version
release: $(RELEASE_TARGET)

# 'run' target builds and executes the debug version
run: $(DEBUG_TARGET)
	@echo "Running $(DEBUG_TARGET)..."
	./$(DEBUG_TARGET) test.in > test.out

test: all
	rm -rf test-result/
	./test.sh

# 'clean' target removes all generated files and the object directory
clean:
	@echo "Cleaning up..."
	rm -f $(OBJECT_DIR)/*.o $(DEFAULT_TARGET) $(DEBUG_TARGET) $(RELEASE_TARGET)
	rmdir $(OBJECT_DIR) 2>/dev/null || true # Remove directory if empty, suppress error if not
