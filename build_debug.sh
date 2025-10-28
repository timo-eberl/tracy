#!/bin/bash

emcc src/tracy.c \
	-o src/tracy_c.js \
	-sMODULARIZE=1 \
	-sEXPORT_ES6=1 \
	--emit-tsd tracy_c.d.ts \
	-sSHARED_MEMORY=1 \
	-sIMPORTED_MEMORY=1 \
	-Wall \
	-g \
	-gsource-map \
	-O0

# -g: Preserve debug information.
# -O0: No optimizations, but debugging will be limited (e.g. some local variables are removed)
# -O1: Simple optimizations that make the code way faster (~6x) while preserving some debug information
#      If debugging breaks, use O0
