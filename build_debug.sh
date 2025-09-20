#!/bin/bash

# HEAPU8: expose memory to JS to read image data
# -g: Preserve debug information.
# -O1: Simple optimizations that make the code way faster (~6x) while preserving debug information
#      If debugging breaks, try removing it

emcc src/ray_tracer.c \
	-o src/ray_tracer.js \
	-sMODULARIZE=1 \
	-sEXPORT_ES6=1 \
	--emit-tsd ray_tracer.d.ts \
	-sEXPORTED_RUNTIME_METHODS=HEAPU8 \
	-g \
	-gsource-map \
	-O1
