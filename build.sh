#!/bin/bash

emcc ray_tracer.c \
	-o ray_tracer.js \
	-O1 \
	-sEXPORTED_FUNCTIONS=_render \
	-sEXPORTED_RUNTIME_METHODS=cwrap,HEAPU8
