#!/bin/bash

emcc ray_tracer.c \
	-o ray_tracer.js \
	-s EXPORTED_FUNCTIONS="['_render', '_get_image_buffer']" \
	-s EXPORTED_RUNTIME_METHODS="['cwrap']"
