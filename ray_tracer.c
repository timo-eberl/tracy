#include <stdlib.h>
#include <emscripten.h>

unsigned char* image_buffer;
int buffer_width = 0;
int buffer_height = 0;

EMSCRIPTEN_KEEPALIVE
unsigned char* get_image_buffer(int width, int height) {
	// Re-allocate buffer only if dimensions change
	if (image_buffer == NULL || width != buffer_width || height != buffer_height) {
		if (image_buffer != NULL) {
			free(image_buffer);
		}
		image_buffer = malloc(width * height * 4 * sizeof(unsigned char));
		buffer_width = width;
		buffer_height = height;
	}
	return image_buffer;
}

EMSCRIPTEN_KEEPALIVE
void render(int width, int height) {
	unsigned char* buffer = get_image_buffer(width, height);
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int index = (y * width + x) * 4;
			// Background color (light blue)
			buffer[index + 0] = 173;
			buffer[index + 1] = 216;
			buffer[index + 2] = 230;
			buffer[index + 3] = 255;
		}
	}
}
