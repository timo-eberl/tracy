#!/bin/bash

emcc ray_tracer.c \
	-o ray_tracer.js \
	-sMODULARIZE=1 \
	-sEXPORT_ES6=1 \
	-sEXPORTED_RUNTIME_METHODS=cwrap,HEAPU8 \
	-sEXPORTED_FUNCTIONS=_render_full,_render_fast,_malloc,_free \
	-g \
	-gsource-map
