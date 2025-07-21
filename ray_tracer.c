#include <emscripten.h>
#include <stdlib.h>
#include <math.h>

// The image buffer will be allocated on demand.
unsigned char* image_buffer = NULL;
int buffer_width = 0;
int buffer_height = 0;

// Structured Super-Sampling Configuration
// The dimension of the grid within each pixel.
// 3 means a 3x3 grid, for a total of 9 samples per pixel.
#define SUPER_SAMPLE_GRID_DIM 3

typedef struct { double x, y, z; } Vec;
Vec vec_add(Vec a, Vec b) { return (Vec){a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec vec_sub(Vec a, Vec b) { return (Vec){a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec vec_scale(Vec v, double s) { return (Vec){v.x * s, v.y * s, v.z * s}; }
double vec_dot(Vec a, Vec b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
double vec_length(Vec v) { return sqrt(vec_dot(v, v)); }
Vec vec_normalize(Vec v) {
	double l = vec_length(v);
	if (l == 0) return (Vec){0,0,0};
	return vec_scale(v, 1.0 / l);
}
Vec vec_cross(Vec a, Vec b) {
	return (Vec){
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	};
}

typedef struct { Vec origin; Vec dir; } Ray;
typedef struct { Vec center; double radius; Vec color; } Sphere;
Sphere scene[] = {
	{{ 1e5+1,40.8,81.6},  1.0e5, {255 * .75, 255 * .25, 255 * .25}},//Left
	{{-1e5+99,40.8,81.6}, 1.0e5, {255 * .25, 255 * .25, 255 * .75}},//Rght
	{{50,40.8, 1e5},      1.0e5, {255 * .75, 255 * .75, 255 * .75}},//Back
	// {{50,40.8,-1e5+170},  1.0e5, {255 * .0, 255 * .0, 255 * .0}},//Frnt
	{{50, 1e5, 81.6},     1.0e5, {255 * .75, 255 * .75, 255 * .75}},//Botm
	{{50,-1e5+81.6,81.6}, 1.0e5, {255 * .75, 255 * .75, 255 * .75}},//Top
	{{27,16.5,47},         16.5, {255 * .25, 255 * .25, 255 * .25}},//Mirr
	{{73,16.5,78},         16.5, {255 * .999, 255 * .999, 255 * .999}},//Glas
	{{50,681.6-.27,81.6}, 600.0, {255 * .999, 255 * .999, 255 * .999}}//Lite 
};
int num_spheres = sizeof(scene) / sizeof(Sphere);
// ray-sphere intersection (6.2.4)
double intersect(Ray r, Sphere s) {
	Vec oc = vec_sub(r.origin, s.center);
	double a = vec_dot(r.dir, r.dir);
	double b = 2.0 * vec_dot(oc, r.dir);
	double c = vec_dot(oc, oc) - s.radius * s.radius;
	// quadratic formula
	double discriminant = b * b - 4 * a * c;
	if (discriminant < 0) {
		// cant take the square root of negative number
		// the quadratic formula has no solution -> no intersection
		return -1.0;
	} else {
		double sqrtDiscriminant = sqrt(discriminant);
		// we may have either one solution (t0==t1) or two solutions (t0 < t1)
		double t0 = (-b - sqrtDiscriminant) / (2.0 * a);
		double t1 = (-b + sqrtDiscriminant) / (2.0 * a);
		// we want to know about the closest intersection in front of the camera
		// -> smallest t value
		// however a negative t value would mean an intersection behind the camera
		// -> ignore negative t value
		// therefore we return the smallest positive t value
		return (t0 > 0.0) ? t0 : t1;
	}
}

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

Vec radiance(Ray r) {
	double min_t = INFINITY;
	Sphere* hit_sphere = NULL;

	for (int i = 0; i < num_spheres; ++i) {
		double t = intersect(r, scene[i]);
		if (t > 0 && t < min_t) {
			min_t = t;
			hit_sphere = &scene[i];
		}
	}

	return hit_sphere ? hit_sphere->color : (Vec){0,0,0};
}

EMSCRIPTEN_KEEPALIVE
unsigned char* render(
	int width, int height, double cam_angle_x, double cam_angle_y, double cam_dist,
	double focus_x, double focus_y, double focus_z
) {
	unsigned char* buffer = get_image_buffer(width, height);
	
	Vec focus_point = {focus_x, focus_y, focus_z};

	// Calculate Camera Position using spherical coordinates around the focus point
	double cam_x = focus_point.x + cam_dist * sin(cam_angle_y) * cos(cam_angle_x);
	double cam_y = focus_point.y + cam_dist * sin(cam_angle_x);
	double cam_z = focus_point.z + cam_dist * cos(cam_angle_y) * cos(cam_angle_x);
	Vec camera_origin = {cam_x, cam_y, cam_z};

	// Create the cameras coordinate system (basis vectors)
	Vec forward = vec_normalize(vec_sub(focus_point, camera_origin));
	Vec world_up = {0, 1, 0};
	Vec right = vec_normalize(vec_cross(forward, world_up));
	Vec up = vec_normalize(vec_cross(right, forward));

	double aspect_ratio = (double)width / height;

	double fov_y = 30 * 3.141 / 180.0;
	double fov_scale = tan(fov_y / 2.0); // 5.1.4

	// loop over image pixels
	for (int y = 0; y < height; ++y)
	for (int x = 0; x < width; ++x) {

		Vec accumulated_color = {0, 0, 0};

		// loop over super-sampling grid
		// super-sampling uses a finite grid like described in 2.6.2
		for (int sy = 0; sy < SUPER_SAMPLE_GRID_DIM; ++sy)
		for (int sx = 0; sx < SUPER_SAMPLE_GRID_DIM; ++sx) {
			// evenly spaced sample offsets from the pixel center with range (-0.5;0.5)
			double sample_offset_x = (double)(sx + 0.5) / (double)SUPER_SAMPLE_GRID_DIM - 0.5;
			double sample_offset_y = (double)(sy + 0.5) / (double)SUPER_SAMPLE_GRID_DIM - 0.5;

			// Map pixel coordinates to the view plane (-1;1)
			double world_x = (2.0 * (x + 0.5 + sample_offset_x) / width - 1.0);
			double world_y = 1.0 - 2.0 * (y + 0.5 + sample_offset_y) / height;
			// scale to account for fov and aspect ratio
			world_x *= aspect_ratio * fov_scale;
			world_y *= fov_scale;

			// Calculate the direction for the ray for this pixel
			Vec right_comp = vec_scale(right, world_x);
			Vec up_comp = vec_scale(up, world_y);
			Vec dir = vec_normalize(vec_add(forward, vec_add(right_comp, up_comp)));

			Ray r = {camera_origin, dir};

			accumulated_color = vec_add(accumulated_color, radiance(r));
		}

		double total_samples = SUPER_SAMPLE_GRID_DIM * SUPER_SAMPLE_GRID_DIM;
		Vec final_color = vec_scale(accumulated_color, 1.0 / total_samples);

		int index = (y * width + x) * 4;
		buffer[index + 0] = final_color.x;
		buffer[index + 1] = final_color.y;
		buffer[index + 2] = final_color.z;
		buffer[index + 3] = 255;
	}

	return buffer;
}
