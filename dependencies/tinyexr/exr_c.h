#ifndef EXR_C_H
#define EXR_C_H

int load_exr_rgba(const char* filename, float** out_rgba, int* width, int* height,
				  const char** err_msg);
void free_exr_rgba(float* rgba);

#endif
