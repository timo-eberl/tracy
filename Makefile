# Compilers
CC = clang
EMCC = emcc

# Output names
CLI_TARGET = tracy
WEB_TARGET = web/src/tracy_c.js

# CLI Flags
CFLAGS = -std=c11 -Wall -Wextra -Iinclude -O3 -march=native -flto

# Emscripten Flags (Shared across debug/release)
#  -sSHARED_MEMORY=1 -sIMPORTED_MEMORY=1
#    required so shared memory can be used 
EMFLAGS = -Iinclude \
		  -sMODULARIZE=1 \
		  -sEXPORT_ES6=1 \
		  -sSHARED_MEMORY=1 \
		  -sIMPORTED_MEMORY=1 \
		  --emit-tsd tracy_c.d.ts \
		  -Wall -Wextra
# Release Flags
EM_RELEASE = -O3
# Debug Flags
#  -O0: No optimizations, but debugging will be limited (e.g. some local variables are removed)
#  -O1: Simple optimizations that make the code way faster (~6x) while preserving some debug information
EM_DEBUG = -g \
		   -gsource-map \
		   -O0

# --- Rules ---

.PHONY: all clean run web web-debug

# Default: build the C example
all: $(CLI_TARGET)

# Build the CLI application
$(CLI_TARGET): src/tracy.c examples/c_render/main.c
	$(CC) $(CFLAGS) $^ -o $@ -lm

# Convenience to build and run
run: $(CLI_TARGET)
	./$(CLI_TARGET)

# Web: Release build (-O3)
web: src/tracy.c
	$(EMCC) $(EMFLAGS) $(EM_RELEASE) $< -o $(WEB_TARGET)

# Web: Debug build (-O0, maps, debug symbols)
web-debug: src/tracy.c
	$(EMCC) $(EMFLAGS) $(EM_DEBUG) $< -o $(WEB_TARGET)

# Clean up build artifacts
clean:
	rm -f $(CLI_TARGET)
	rm -f $(WEB_SRC_DIR)/tracy_c.js
	rm -f $(WEB_SRC_DIR)/tracy_c.wasm
	rm -f $(WEB_SRC_DIR)/tracy_c.d.ts
	rm -f $(WEB_SRC_DIR)/tracy_c.wasm.map
