#!/bin/bash

emcc src/ray_tracer.c \
	-o src/ray_tracer.js \
	-sMODULARIZE=1 \
	-sEXPORT_ES6=1 \
	--emit-tsd ray_tracer.d.ts \
	-sEXPORTED_RUNTIME_METHODS=cwrap,HEAPU8 \
	-sEXPORTED_FUNCTIONS=_render_full,_render_fast \
	-O3
