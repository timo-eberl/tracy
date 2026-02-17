#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

// Wrapper to expose tinyexr to C

extern "C" {

int load_exr_rgba(const char* filename, float** out_rgba, int* width, int* height,
				  const char** err_msg) {
	return LoadEXR(out_rgba, width, height, filename, err_msg);
}

// Wrapper to free memory
void free_exr_rgba(float* rgba) {
	free(rgba);
}

int save_exr_rgb_fp16(const char* filename, const float* rgb_data, int width, int height,
					  const char** err_msg) {
	// components=3 (RGB), save_as_fp16=1
	return SaveEXR(rgb_data, width, height, 3, 1, filename, err_msg);
}
}
