#ifndef TRACY_H_
#define TRACY_H_

unsigned char* render_fast(
	int width, int height, double cam_angle_x, double cam_angle_y, double cam_dist,
	double focus_x, double focus_y, double focus_z
);

unsigned char* render_full(
	int width, int height, double cam_angle_x, double cam_angle_y, double cam_dist,
	double focus_x, double focus_y, double focus_z
);

#endif // TRACY_H_
