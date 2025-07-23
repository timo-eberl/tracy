#!/bin/bash

emcc ray_tracer.c \
	-o ray_tracer.js \
	-g \
	-gsource-map \
	-sEXPORTED_FUNCTIONS=_render_full,_render_fast,_malloc,_free \
	-sEXPORTED_RUNTIME_METHODS=cwrap,HEAPU8
