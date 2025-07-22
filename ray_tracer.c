#include <assert.h>
#include <emscripten.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct { double x, y, z; } Vec;
typedef struct { Vec origin; Vec dir; } Ray;
typedef struct { Vec position; double radiant_flux; Vec color; } PointLight;
typedef enum { DIFFUSE, EMISSIVE, MIRROR } MaterialType;
// color is treated differently depending on the material type
// DIFFUSE: color=albedo
// EMISSIVE: color=radiosity (W/m^2)
// MIRROR: color is ignored
typedef struct { Vec center; double radius; Vec color; MaterialType type; } Sphere;
typedef struct {
	double t;   // Distance to hit
	Vec p;	  // Hit point in world space
	Vec n;	  // Normal vector at the hit point
} HitInfo;

// The image buffer will be allocated on demand.
unsigned char* image_buffer = NULL; // stores tone mapped gamma corrected colors
Vec* radiance_buffer = NULL; // stores raw radiance
int buffer_width = 0;
int buffer_height = 0;

// Structured Super-Sampling Configuration
// The dimension of the grid within each pixel.
// 3 means a 3x3 grid, for a total of 9 samples per pixel.
#define SUPER_SAMPLE_GRID_DIM 1
// The standard deviation (sigma) of the Gaussian bell curve. A value of 0.5
// means the filter will be wider than a single pixel.
#define GAUSS_SIGMA 0.5
// The range of the filter in units of sigma. A value of 3.0 means we sample
// across +/- 3-sigma, capturing >99% of the curve's influence.
// This will scale our sample offsets to cover a wider area.
#define GAUSS_FILTER_RADIUS_IN_SIGMA 1.5

// offset used for shadow rays. may need to be adjusted depending on scene scale
#define SELF_OCCLUSION_DELTA 0.00000001

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
Vec vec_add(Vec a, Vec b) { return (Vec){a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec vec_sub(Vec a, Vec b) { return (Vec){a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec vec_scale(Vec v, double s) { return (Vec){v.x * s, v.y * s, v.z * s}; }
Vec vec_hadamard_prod(Vec a, Vec b) { return (Vec){a.x * b.x, a.y * b.y, a.z * b.z}; }
double vec_dot(Vec a, Vec b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
double vec_length(Vec v) { return sqrt(vec_dot(v, v)); }
double vec_length_squared(Vec v) { return vec_dot(v, v); }
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

// light radiant energy calculation:
// typical kitchen: assume 4x3m (12m^2) floor, 2.4m height, target illuminance is ~200 lux
// this means a total of 2400 lumen must reach the floor
// assume the light is 2.4m centered above the floor, the solid angle to the floor is ~1.38 sr
// -> the luminous intensity of the point light should be 3312 lm/sr
// in reality we might have a fluorescent tube or multiple individual smaller LEDs for this, which
// typically are not isotropic (10.1.5), instead their intensity is higher downwards
// however, since we use an isotropic point light, our total luminous flux must be higher to
// achieve the same luminance downwards. typical fluorescent tube has 21.5W.
// luminous flux: (3312 lm/sr) * (4pi sr) = ~41612
// convert to radiant flux (173W/lm for a fluorescent tube): ~240W

// PointLight should be removed once indirect lighting is implemented.
// Emissive light: probably around 1m^2, so 21.5W flux correspond to 21.5W/m^2 radiosity

// room dimensions: (3,2.4,4)

// position, radiant flux (W), color
PointLight light = { {0,2.38,0 }, 100, {1,1,1} };
Sphere scene[] = { // center, radius, color, type
	{{ 1e4-1.5,	 1.2,	 0},  1.0e4, {0.75, 0.25, 0.25}, DIFFUSE}, // Left
	{{-1e4+1.5,	 1.2,	 0},  1.0e4, {0.25, 0.25, 0.75}, DIFFUSE}, // Right
	{{	   0,	 1.2, 1e4-2},  1.0e4, {0.75, 0.75, 0.75}, DIFFUSE}, // Back
	{{	   0,	 1e4,	 0},  1.0e4, {0.75, 0.75, 0.75}, DIFFUSE}, // Bottom
	{{	   0,-1e4+2.4,	 0},  1.0e4, {0.75, 0.75, 0.75}, DIFFUSE}, // Top
	{{	-0.7,	 0.5,  -0.6},	0.5, {0.75, 0.75, 0.75}, MIRROR}, // Mirror Sphere
	{{	 0.7,	 0.5,   0.6},	0.5, {0.75, 0.75, 0.75}, DIFFUSE}, // Glass Sphere
	{{	   0,  62.397,	 0},   60.0, {21.5, 21.5, 21.5}, EMISSIVE} // Area Light
};
int num_spheres = sizeof(scene) / sizeof(Sphere);
// ray-sphere intersection (6.2.4)
bool intersect_sphere(Ray r, Sphere s, HitInfo* hit) {
	Vec oc = vec_sub(r.origin, s.center);
	double a = vec_dot(r.dir, r.dir);
	double b = 2.0 * vec_dot(oc, r.dir);
	double c = vec_dot(oc, oc) - s.radius * s.radius;
	// quadratic formula
	double discriminant = b * b - 4 * a * c;
	if (discriminant < 0) {
		// cant take the square root of negative number
		// the quadratic formula has no solution -> no intersection
		return false;
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
		if (t1 <= 0) {
			return false; // both intersections are behind ray
		}
		hit->t = t0 > 0 ? t0 : t1; // if the first intersection is behind ray, use the other
		hit->p = vec_add(r.origin, vec_scale(r.dir, hit->t));
		hit->n = vec_normalize(vec_sub(hit->p, s.center));
		if (!(t0 > 0.0)) { // hit form inside -> flip normal
			hit->n = vec_scale(hit->n, -1.0);
		}
		return true;
	}
}

bool intersect_scene(Ray r, HitInfo* closest_hit, Sphere** hit_sphere) {
	closest_hit->t = INFINITY;
	*hit_sphere = NULL;

	for (int i = 0; i < num_spheres; ++i) {
		HitInfo current_hit;
		if (intersect_sphere(r, scene[i], &current_hit)) {
			if (current_hit.t < closest_hit->t) {
				*closest_hit = current_hit;
				*hit_sphere = &scene[i];
			}
		}
	}

	return (*hit_sphere);
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

bool is_in_shadow(Vec surf_pos, Vec surf_normal, Vec light_pos) {
	Vec light_direction = vec_normalize(vec_sub(light_pos, surf_pos));
	Ray shadow_ray = {surf_pos, light_direction};
	shadow_ray.origin = vec_add(shadow_ray.origin, vec_scale(surf_normal, SELF_OCCLUSION_DELTA));
	HitInfo shadow_ray_hit;
	bool is_in_shadow = intersect_scene(shadow_ray, &shadow_ray_hit, &(Sphere*){NULL});
	if (is_in_shadow) { // check if occluder is further away than light source
		double hit_dist_sq = shadow_ray_hit.t * shadow_ray_hit.t;
		double light_dist_sq = vec_length_squared(vec_sub(light.position,shadow_ray.origin));
		if (hit_dist_sq > light_dist_sq) { // risk of self occlusion?
			is_in_shadow = false;
		}
	}
	return is_in_shadow;
}

Vec radiance_from_ray(Ray r) {
	HitInfo hit;
	Sphere* hit_sphere = NULL;
	bool did_hit = intersect_scene(r, &hit, &hit_sphere);

	if (!did_hit) {
		return (Vec){0,0,0};
	}
	else if (hit_sphere->type == EMISSIVE) {
		Vec radiosity = hit_sphere->color;
		Vec radiance = vec_scale(radiosity, 1.0/(4.0*M_PI));
		return radiance;
	}
	else if (hit_sphere->type == DIFFUSE) {
		if (is_in_shadow(hit.p, hit.n, light.position)) return (Vec){0,0,0};

		Vec light_direction = vec_normalize(vec_sub(light.position, hit.p));
		double cos_theta = vec_dot(hit.n, light_direction);
		if (cos_theta < 0.0) cos_theta = 0.0; // only upper hemisphere

		double dist_sq = vec_length_squared(vec_sub(light.position, hit.p));
		double irradiance = (cos_theta * light.radiant_flux) / (dist_sq * 4 * M_PI); // 10.1.5
		Vec colored_irradiance = vec_scale(light.color, irradiance);
		Vec lambertian_brdf = vec_scale(hit_sphere->color, 1.0/M_PI);
		return vec_hadamard_prod(lambertian_brdf, colored_irradiance);
	}
	return (Vec){0,0,0};
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

		if (x == 560 && y == 90) {
			// use for setting breakpoint
			char* dummy = "dummy";
		}

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
			Vec radiance_s = radiance_from_ray(r);

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
