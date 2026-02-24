// render to an image file

#include "tracy.h"
#include <stdio.h>

#define STEPS 10
#define SAMPLES_PER_STEP 50

void save_image_as_tga(const char* file_path, unsigned char* buffer, int width, int height) {
	FILE* fp = fopen(file_path, "wb"); // Must be "wb" for binary
	if (!fp) {
		fprintf(stderr, "Error: Could not open file %s for writing.\n", file_path);
		return;
	}

	// 18-byte TGA Header (Uncompressed 32-bit RGBA)
	unsigned char header[18] = {0};
	header[2] = 2;					   // Uncompressed true-color
	header[12] = width & 0xFF;		   // Width LSB
	header[13] = (width >> 8) & 0xFF;  // Width MSB
	header[14] = height & 0xFF;		   // Height LSB
	header[15] = (height >> 8) & 0xFF; // Height MSB
	header[16] = 32;				   // 32 bits per pixel (RGBA)
	header[17] = 0x20;				   // Origin: Top-Left

	fwrite(header, 1, 18, fp);

	// TGA expects BGRA order. We swap R and B from the RGBA buffer while writing.
	for (int i = 0; i < width * height; ++i) {
		fputc(buffer[i * 4 + 2], fp); // B
		fputc(buffer[i * 4 + 1], fp); // G
		fputc(buffer[i * 4 + 0], fp); // R
		fputc(buffer[i * 4 + 3], fp); // A
	}

	fclose(fp);
}

int main() {
	const int scene = 1;

	const int max_depth = 6;
	const int width = 280;
	const int height = 320;

	const int filter_type = 0;

	const double cam_angle_x = 0.4;
	const double cam_angle_y = 0.0;
	const double cam_dist = 4.0;
	const double focus_x = 0.0;
	const double focus_y = 1.05;
	const double focus_z = 0.0;

	printf("Rendering scene %d at %dx%d...\n", scene, width, height);

	render_init(scene, max_depth, width, height, filter_type, cam_angle_x, cam_angle_y, cam_dist,
				focus_x, focus_y, focus_z);

	// do incremental updates and update image each time for live preview
	for (size_t i = 0; i < STEPS; i++) {
		render_refine(SAMPLES_PER_STEP);
		unsigned char* image_buffer = update_image_ldr();
		printf("Step %d/%d: Saving to 'render_c.tga'...\n", ((int)i + 1), STEPS);
		save_image_as_tga("render_c.tga", image_buffer, width, height);
	}

	printf("Done.\n");

	return 0;
}
