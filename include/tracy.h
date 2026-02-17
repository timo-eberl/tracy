#ifndef TRACY_H_
#define TRACY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * Initializes or reconfigures the renderer with new scene parameters.
 * Must be called at least once before any rendering. Calling it again will reset the render
 * progress with the new settings.
 */
void render_init(int width, int height, int filter_type, double cam_angle_x, double cam_angle_y,
				 double cam_dist, double focus_x, double focus_y, double focus_z);

/**
 * Progressively refines the image by adding more samples.
 * Call this repeatedly to reduce noise and improve image quality.
 * To get the result call `refresh_image_ldr` or `refresh_image_hdr`.
 * This function does not update the image buffers retrieved by those functions.
 * @param n_samples The number of samples to add per pixel in this step.
 */
void render_refine(unsigned int n_samples);

/**
 * Processes the current rendered state into 8-bit LDR (RGBA).
 * Call this each time after `render_refine` to get the current image data.
 * @return Pointer to the LDR buffer.
 */
uint8_t* update_image_ldr();

/**
 * Processes the current rendered state into 32-bit Linear Float HDR (RGB).
 * Call this each time after `render_refine` to get the current image data.
 * @return Pointer to the HDR buffer.
 */
float* update_image_hdr();

#ifdef __cplusplus
}
#endif

#endif // TRACY_H_
