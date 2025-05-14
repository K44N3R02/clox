CC = clang
CFLAGS = -Wall -pedantic-errors -O0 -std=c99 -pedantic
OPT_CFLAGS = -O2
DEBUG_CFLAGS = -g
TARGET = clox
SOURCES = main.c chunk.c compiler.c debug.c object.c memory.c scanner.c value.c vm.c
OBJECT_DIR = bin
OBJECTS = $(addprefix $(OBJECT_DIR)/,$(SOURCES:.c=.o))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

# Default build (non-optimized, no debug info, pedantic errors)
$(OBJECT_DIR)/%.o: %.c
	@mkdir -p $(OBJECT_DIR) # Create the directory if it doesn't exist
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build
debug: CFLAGS += $(DEBUG_CFLAGS)
debug: all # Build the 'all' target, but with modified CFLAGS

# Release build (optimized)
release: CFLAGS := $(CFLAGS)
release: CFLAGS += $(OPT_CFLAGS)
release: all # Build the 'all' target, but with modified CFLAGS

clean:
	rm -f $(OBJECTS) $(TARGET)

run: all
	./$(TARGET) test.in > test.out

.PHONY: all debug release clean run # Declare these as phony targets

