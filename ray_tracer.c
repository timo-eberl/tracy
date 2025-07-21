#include <emscripten.h>
#include <stdlib.h>
#include <math.h>

typedef struct { double x, y, z; } Vec;
typedef struct { Vec origin; Vec dir; } Ray;
typedef struct { Vec center; double radius; Vec color; } Sphere;

// The image buffer will be allocated on demand.
unsigned char* image_buffer = NULL; // stores tone mapped gamma corrected colors
Vec* radiance_buffer = NULL; // stores raw radiance
int buffer_width = 0;
int buffer_height = 0;

// Structured Super-Sampling Configuration
// The dimension of the grid within each pixel.
// 3 means a 3x3 grid, for a total of 9 samples per pixel.
#define SUPER_SAMPLE_GRID_DIM 3
// The standard deviation (sigma) of the Gaussian bell curve. A value of 0.5
// means the filter will be wider than a single pixel.
#define GAUSS_SIGMA 0.5
// The range of the filter in units of sigma. A value of 3.0 means we sample
// across +/- 3-sigma, capturing >99% of the curve's influence.
// This will scale our sample offsets to cover a wider area.
#define GAUSS_FILTER_RADIUS_IN_SIGMA 1.5

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
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

Sphere scene[] = {
	{{ 1e5+1,40.8,81.6},  1.0e5, {0.75, 0.25, 0.25}},//Left
	{{-1e5+99,40.8,81.6}, 1.0e5, {0.25, 0.25, 0.75}},//Rght
	{{50,40.8, 1e5},      1.0e5, {0.75, 0.75, 0.75}},//Back
	// {{50,40.8,-1e5+170},  1.0e5, {0.00, 0.00, 0.00}},//Frnt
	{{50, 1e5, 81.6},     1.0e5, {0.75, 0.75, 0.75}},//Botm
	{{50,-1e5+81.6,81.6}, 1.0e5, {0.75, 0.75, 0.75}},//Top
	{{27,16.5,47},         16.5, {0.25, 0.25, 0.25}},//Mirr
	{{73,16.5,78},         16.5, {1.00, 1.00, 1.00}},//Glas
	{{50,681.6-.27,81.6}, 600.0, {15.0, 15.0, 15.0}}//Lite
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

// srgb response curve (4.1.9)
double linear_to_srgb(double v) {
	return (v <= 0.0031308) ? (12.92 * v) : (1.055 * pow(v, 0.416666667) - 0.055);
}

// for tone mapping we convert our color values to a single luminance value to combat
// the problem of washing out our image described in 4.2.5
double luminance(Vec rgb) {
	return 0.2126*rgb.x + 0.7152*rgb.y + 0.0722*rgb.z;
}
// simple reinhard tone mapping operator based on luminance
Vec reinhard_luminance(Vec rgb_hdr) {
	double l_hdr = luminance(rgb_hdr);
	double l_ldr = l_hdr / (1.0 + l_hdr);
	return vec_scale(rgb_hdr, l_ldr/l_hdr);
}

void set_buffer_size(int width, int height) {
	// (Re)allocate buffer if dimensions change or not allocated yet
	if (
		image_buffer == NULL || radiance_buffer == NULL
		|| width != buffer_width || height != buffer_height)
	{
		if (image_buffer != NULL) {
			free(image_buffer);
		}
		if (radiance_buffer != NULL) {
			free(radiance_buffer);
		}
		radiance_buffer = malloc(width * height * sizeof(Vec));
		image_buffer = malloc(width * height * 4 * sizeof(unsigned char));
		buffer_width = width;
		buffer_height = height;
	}
}

// Calculates a weight based on the 2D Gaussian PDF.
// This implements the formula: (1 / (2πσ²)) e^(-(x² + y²) / (2σ²))
// We are doing a normalization in the render loop, so the normalization
// constant could be omitted.
double gaussian_weight_2d(double offset_x, double offset_y, double sigma) {
	double two_sigma_squared = 2.0 * sigma * sigma;
	// normalization constant: (1 / (2πσ²))
	double norm_const = 1.0 / (M_PI * two_sigma_squared);
	double r_squared = offset_x * offset_x + offset_y * offset_y;
	return norm_const * exp(-r_squared / two_sigma_squared);
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
	set_buffer_size(width, height);

	Vec focus_point = {focus_x, focus_y, focus_z};

	// Calculate Camera Position using spherical coordinates around the focus point
	double cam_x = focus_point.x + cam_dist * sin(cam_angle_y) * cos(cam_angle_x);
	double cam_y = focus_point.y + cam_dist * sin(cam_angle_x);
	double cam_z = focus_point.z + cam_dist * cos(cam_angle_y) * cos(cam_angle_x);
	Vec camera_origin = {cam_x, cam_y, cam_z};

	// Create the cameras coordinate system (basis vectors) 5.1.6
	Vec forward = vec_normalize(vec_sub(focus_point, camera_origin));
	Vec world_up = {0, 1, 0};
	Vec right = vec_normalize(vec_cross(forward, world_up));
	Vec up = vec_normalize(vec_cross(right, forward));

	double aspect_ratio = (double)width / height;

	double fov_y = 30 * 3.141 / 180.0;
	double fov_scale = tan(fov_y / 2.0); // 5.1.4

	// loop over pixels, calculate radiance
	for (int y = 0; y < height; ++y)
	for (int x = 0; x < width; ++x) {
		Vec weighted_radiance_sum = {0, 0, 0};
		double total_weight_sum = 0.0;

		// This factor scales our sample offsets to cover the desired range of the filter.
		// For sigma=0.5, radius=3 samples will range from -1.5 to 1.5
		const double sample_range_scale = GAUSS_SIGMA * GAUSS_FILTER_RADIUS_IN_SIGMA * 2.0;
		// loop over super-sampling grid
		// super-sampling uses a finite grid like described in 2.6.2
		for (int sy = 0; sy < SUPER_SAMPLE_GRID_DIM; ++sy)
		for (int sx = 0; sx < SUPER_SAMPLE_GRID_DIM; ++sx) {
			// evenly spaced sample offsets from the pixel center with range (-0.5;0.5)
			double sample_offset_x = (double)(sx + 0.5) / (double)SUPER_SAMPLE_GRID_DIM - 0.5;
			double sample_offset_y = (double)(sy + 0.5) / (double)SUPER_SAMPLE_GRID_DIM - 0.5;
			// scale to match the filters radius
			sample_offset_x *= sample_range_scale;
			sample_offset_y *= sample_range_scale;

			// 5.2.2
			// Map pixel coordinates to the view plane (-1;1)
			double world_x = (2.0 * (x + 0.5 + sample_offset_x) / width - 1.0);
			double world_y = 1.0 - 2.0 * (y + 0.5 + sample_offset_y) / height;

			// Calculate the direction for the ray for this pixel
			Vec right_comp = vec_scale(right, world_x * fov_scale * aspect_ratio);
			Vec up_comp = vec_scale(up, world_y * fov_scale);
			Vec dir = vec_normalize(vec_add(forward, vec_add(right_comp, up_comp)));

			Ray r = {camera_origin, dir};
			Vec radiance_s = radiance(r);

			double weight = gaussian_weight_2d(sample_offset_x, sample_offset_y, GAUSS_SIGMA);
			weighted_radiance_sum = vec_add(weighted_radiance_sum, vec_scale(radiance_s, weight));
			total_weight_sum += weight;
		}

		// Normalize the final color by dividing by the total sum of weights.
		// If this were a continuous integral, the sum of weights would be 1.0 and this
		// step unnecessary. But since we are doing a discrete sum, our total weight
		// will not be exactly 1.0, so we manually keep track of it.
		Vec weighted_radiance = vec_scale(weighted_radiance_sum, 1.0 / total_weight_sum);

		radiance_buffer[y * width + x] = weighted_radiance;
	}

	// loop over pixels, do tone mapping and gamma correction
	for (int y = 0; y < height; ++y)
	for (int x = 0; x < width; ++x) {
		Vec ldr_color = reinhard_luminance(radiance_buffer[y * width + x]);

		int index = (y * width + x) * 4;
		image_buffer[index + 0] = linear_to_srgb(ldr_color.x) * 255;
		image_buffer[index + 1] = linear_to_srgb(ldr_color.y) * 255;
		image_buffer[index + 2] = linear_to_srgb(ldr_color.z) * 255;
		image_buffer[index + 3] = 255;
	}

	return image_buffer;
}
