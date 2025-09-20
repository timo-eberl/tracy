#!/bin/bash

# HEAPU8: expose memory to JS to read image data
# -O3: Max runtime optimizations

emcc src/ray_tracer.c \
	-o src/ray_tracer.js \
	-sMODULARIZE=1 \
	-sEXPORT_ES6=1 \
	--emit-tsd ray_tracer.d.ts \
	-sEXPORTED_RUNTIME_METHODS=HEAPU8 \
	-O3
