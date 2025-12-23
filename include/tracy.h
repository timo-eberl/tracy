#ifndef TRACY_H_
#define TRACY_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * Initializes or reconfigures the renderer with new scene parameters.
 * Must be called at least once before any rendering. Calling it again will
 * reset the render progress with the new settings.
 */
void render_init(int p_width, int p_height, int p_filter_type, double p_cam_angle_x,
				 double p_cam_angle_y, double p_cam_dist, double p_focus_x, double p_focus_y,
				 double p_focus_z);

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
