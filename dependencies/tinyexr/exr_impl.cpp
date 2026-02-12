#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

extern "C" {
// Wrapper to expose LoadEXR to C
int load_exr_rgba(const char* filename, float** out_rgba, int* width, int* height,
				  const char** err_msg) {
	return LoadEXR(out_rgba, width, height, filename, err_msg);
}

// Wrapper to free memory
void free_exr_rgba(float* rgba) {
	free(rgba);
}
}
