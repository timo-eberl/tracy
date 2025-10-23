// render to an image file - without web

#include <stdio.h>
#include <stdlib.h>
#include "tracy.h"

void save_image_as_ppm(const char* file_path, unsigned char* image_buffer, int width, int height) {
	FILE *fp = fopen(file_path, "w");
	if (!fp) {
		fprintf(stderr, "Error: Could not open file %s for writing.\n", file_path);
		return;
	}

	// Write the PPM header (P3 = ASCII RGB)
	fprintf(fp, "P3\n%d %d\n255\n", width, height);

	// Write the pixel data
	for (int i = 0; i < width * height; ++i) {
		fprintf(fp, "%d %d %d\n",
			image_buffer[i * 4 + 0],
			image_buffer[i * 4 + 1],
			image_buffer[i * 4 + 2]);
	}

	fclose(fp);
}

int main() {
	// Define image dimensions
	const int width = 640;
	const int height = 480;

	const double cam_angle_x = 0.04258603374866164;
	const double cam_angle_y = 0.0;
	const double cam_dist = 5.5;
	const double focus_x = 0.0;
	const double focus_y = 1.25;
	const double focus_z = 0.0;

	printf("Rendering scene at %dx%d...\n", width, height);

	unsigned char* image_buffer = render_full(
		width, height, cam_angle_x, cam_angle_y, cam_dist, focus_x, focus_y, focus_z
	);

	printf("Saving image to 'render.ppm'...\n");

	// Save the rendered image
	save_image_as_ppm("render.ppm", image_buffer, width, height);

	printf("Done.\n");

	return 0;
}
