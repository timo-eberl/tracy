#!/bin/bash

emcc src/tracy.c \
	-o src/tracy_c.js \
	-sMODULARIZE=1 \
	-sEXPORT_ES6=1 \
	--emit-tsd tracy_c.d.ts \
	-sEXPORTED_RUNTIME_METHODS=HEAPU8 \
	-g \
	-gsource-map \
	-O1

# HEAPU8: expose memory to JS to read image data
# -g: Preserve debug information.
# -O1: Simple optimizations that make the code way faster (~6x) while preserving debug information
#      If debugging breaks, try removing it
