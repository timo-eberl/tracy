#!/bin/bash

emcc ray_tracer.c \
	-o ray_tracer.js \
	-O3 \
	-sEXPORTED_FUNCTIONS=_render_full,_render_fast \
	-sEXPORTED_RUNTIME_METHODS=cwrap,HEAPU8
