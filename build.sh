#!/bin/bash

emcc src/tracy.c \
	-o src/tracy_c.js \
	-sMODULARIZE=1 \
	-sEXPORT_ES6=1 \
	--emit-tsd tracy_c.d.ts \
	-sSHARED_MEMORY=1 \
	-sIMPORTED_MEMORY=1 \
	-Wall \
	-Wextra \
	-O3

# -O3: Max runtime optimizations

# -sSHARED_MEMORY=1 -sIMPORTED_MEMORY=1
# 	required so shared memory can be used
