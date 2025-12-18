#ifndef TRACY_H_
#define TRACY_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * Initializes or reconfigures the renderer with new scene parameters.
 * Must be called at least once before any rendering. Calling it again will
 * reset the render progress with the new settings.
 */
void render_init(int width, int height, double cam_angle_x, double cam_angle_y, double cam_dist,
				 double focus_x, double focus_y, double focus_z);

/**
 * Generates a fast, low-quality preview of the scene.
 * Useful for interactive camera adjustments.
 * @return A pointer to the internal buffer containing the 32-bit RGBA image.
 */
uint8_t* render_fast();

/**
 * Progressively refines the image by adding more samples.
 * Call this repeatedly to reduce noise and improve image quality.
 * @param n_samples The number of samples to add per pixel in this step.
 * @return A pointer to the internal buffer with the updated RGBA image.
 */
uint8_t* render_refine(unsigned int n_samples);

#endif // TRACY_H_
