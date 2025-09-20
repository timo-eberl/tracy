#!/bin/bash

emcc src/tracy.c \
	-o src/tracy_c.js \
	-sMODULARIZE=1 \
	-sEXPORT_ES6=1 \
	--emit-tsd tracy_c.d.ts \
	-sEXPORTED_RUNTIME_METHODS=HEAPU8 \
	-O3

# HEAPU8: expose memory to JS to read image data
# -O3: Max runtime optimizations
