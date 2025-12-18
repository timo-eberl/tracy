#include "tracy.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Conditionally include emscripten.h and define EMSCRIPTEN_KEEPALIVE
#ifdef __EMSCRIPTEN__
// if VS Code says it can't find emscripten, you need to add its path to includePath
// something like /home/user/emsdk/upstream/emscripten/system/include
#include <emscripten.h>
#else
// For non-Emscripten builds (like GCC), define this as an empty macro so the compiler ignores it.
#define EMSCRIPTEN_KEEPALIVE
#endif

// The standard deviation (sigma) of the Gaussian bell curve. A value of 0.5 means the filter will
// be wider than a single pixel.
#define GAUSS_SIGMA 0.5
// The range of the filter in units of sigma. A value of 3.0 means we sample across +/- 3-sigma,
// capturing >99% of the curve's influence.
#define GAUSS_FILTER_RADIUS_IN_SIGMA 2.0

#define MAX_DEPTH 4

// offset used for shadow rays. may need to be adjusted depending on scene scale
#define SELF_OCCLUSION_DELTA 0.00000001

// clang-format off
// allow one line typedefs
typedef struct { double x, y, z; } Vec;
typedef struct { Vec origin; Vec dir; } Ray;
typedef struct { Vec position; double radiant_flux; Vec color; } PointLight;
typedef enum { DIFFUSE, EMISSIVE, MIRROR, REFRACTIVE } MaterialType;
// color is treated differently depending on the material type
// DIFFUSE: perfect lambertian diffuse, color=albedo
// EMISSIVE: only emission, color=radiosity (W/m^2)
// MIRROR: perfect reflection, color=rho, rho describes the ratio of reflected radiance
// REFRACTIVE: reflection and refraction, color.x=ior
typedef struct { Vec center; double radius; Vec color; MaterialType type; } Sphere;
// t: distance, p: point, n: normal, inside: flag
typedef struct { double t; Vec p; Vec n; bool inside; } HitInfo;
// clang-format on

// light radiant energy calculation:
// typical kitchen: assume 4x3 m (12 m^2) floor, 2.4 m height, target illuminance is ~200 lux
// this means a total of 2400 lumen must reach the floor
// assume the light is 2.4 m centered above the floor, the solid angle to the floor is ~1.38 sr
// -> the luminous intensity of the point light should be 1739 lm/sr
// in reality we might have a fluorescent tube or multiple individual smaller LEDs for this, which
// typically are not isotropic (10.1.5), instead their intensity is higher downwards
// however, since we use an isotropic point light, our total luminous flux must be higher to
// achieve the same luminance downwards. typical fluorescent tube has 21.5 W.
// luminous flux: (1739 lm/sr) * (4pi sr) = ~21850 lm
// convert to radiant flux (172 lm/W for a fluorescent tube): ~127 W

// PointLight should be removed once indirect lighting is implemented.
// Emissive light: visible surface is probably around 1 m^2, so 21.5 W flux correspond to
// 21.5 W/m^2 radiosity

// room dimensions: (3,2.4,4)

// position, radiant flux (W), color
PointLight simple_light = {{0, 2.38, 0}, 127, {1, 1, 1}}; // only for render_fast
// clang-format off
// KEEP ALIGNED: Tabular data for scene definition
Sphere scene[] = { // center, radius, color, type
	{{ 1e4-1.5,	 1.2,	 0},  1.0e4, {0.75, 0.25, 0.25}, DIFFUSE}, // Left
	{{-1e4+1.5,	 1.2,	 0},  1.0e4, {0.25, 0.25, 0.75}, DIFFUSE}, // Right
	{{	   0,	 1.2, 1e4-2}, 1.0e4, {0.75, 0.75, 0.75}, DIFFUSE}, // Back
	{{	   0,	 1e4,	 0},  1.0e4, {0.75, 0.75, 0.75}, DIFFUSE}, // Bottom
	{{	   0,-1e4+2.4,	 0},  1.0e4, {0.75, 0.75, 0.75}, DIFFUSE}, // Top
	{{	-0.7,	 0.5,  -0.6},	0.5, {1.00, 1.00, 1.00}, MIRROR}, // Mirror Sphere
	{{	 0.7,	 0.5,   0.6},	0.5, {1.50, 0.00, 0.00}, REFRACTIVE}, // Glass Sphere
	{{	   0,62.3979,	 0},   60.0, {2*21.5, 2*21.5, 2*21.5}, EMISSIVE}, // Area Light
};
// clang-format on
int num_spheres = sizeof(scene) / sizeof(Sphere);

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// clang-format off
// KEEP COMPACT: Vector intrinsics are more readable as one-liners
Vec vec_add(Vec a, Vec b) { return (Vec){a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec vec_sub(Vec a, Vec b) { return (Vec){a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec vec_scale(Vec v, double s) { return (Vec){v.x * s, v.y * s, v.z * s}; }
Vec vec_hadamard_prod(Vec a, Vec b) { return (Vec){a.x * b.x, a.y * b.y, a.z * b.z}; }
double vec_dot(Vec a, Vec b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
double vec_length(Vec v) { return sqrt(vec_dot(v, v)); }
double vec_length_squared(Vec v) { return vec_dot(v, v); }
// clang-format on
Vec vec_normalize(Vec v) {
	double l = vec_length(v);
	if (l == 0) return (Vec){0, 0, 0};
	return vec_scale(v, 1.0 / l);
}
Vec vec_cross(Vec a, Vec b) {
	return (Vec){
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x,
	};
}

// The image buffer will be allocated on demand.
uint8_t* image_buffer = NULL;				 // stores tone mapped gamma corrected colors
Vec* summed_weighted_radiance_buffer = NULL; // stores summed raw radiance
double* summed_weights_buffer = NULL;		 // stores the summed weights of the samples
int buffer_width = 0;
int buffer_height = 0;

// state variables
int width, height;
Vec camera_origin;
Vec forward, right, up;
int sample_count;

// 10.3.16
Vec reflect(Vec incident, Vec normal) {
	return vec_sub(incident, vec_scale(normal, 2.0 * vec_dot(normal, incident)));
}

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
		hit->inside = !(t0 > 0.0);
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
	return 0.2126 * rgb.x + 0.7152 * rgb.y + 0.0722 * rgb.z;
}
// simple reinhard tone mapping operator based on luminance
Vec reinhard_luminance(Vec rgb_hdr) {
	double l_hdr = luminance(rgb_hdr);
	double l_ldr = l_hdr / (1.0 + l_hdr);
	return vec_scale(rgb_hdr, l_ldr / l_hdr);
}

void initialize_buffers() {
	// (Re)allocate buffer if dimensions change or not allocated yet
	if (image_buffer == NULL || summed_weighted_radiance_buffer == NULL ||
		summed_weights_buffer == NULL || width != buffer_width || height != buffer_height) {

		if (image_buffer != NULL) { free(image_buffer); }
		if (summed_weighted_radiance_buffer != NULL) { free(summed_weighted_radiance_buffer); }
		if (summed_weights_buffer != NULL) { free(summed_weights_buffer); }
		summed_weighted_radiance_buffer = malloc(width * height * sizeof(Vec));
		summed_weights_buffer = malloc(width * height * sizeof(double));
		image_buffer = malloc(width * height * 4 * sizeof(uint8_t));
		buffer_width = width;
		buffer_height = height;
	}
	// write zeros in radiance buffers
	// no need to clear image_buffer as it's overwritten every time it's requeted
	memset(summed_weighted_radiance_buffer, 0, width * height * sizeof(Vec));
	memset(summed_weights_buffer, 0, width * height * sizeof(double));
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

// Calculates the Fresnel reflectance amount using Schlick's approximation.
// Determines how much light reflects vs. refracts.
double fresnel(Vec incident, Vec normal, bool is_inside, double ior) {
	double R0 = ((1.0 - ior) / (1.0 + ior)) * ((1.0 - ior) / (1.0 + ior));
	double cos_i = -vec_dot(incident, normal);
	// If we're inside the medium, use the correct IOR for the calculation
	if (is_inside) cos_i = -cos_i; // This can happen if ray is inside the sphere

	return R0 + (1 - R0) * pow(1 - cos_i, 5);
}

bool is_in_shadow(Vec surf_pos, Vec surf_normal, Vec light_pos) {
	Vec light_direction = vec_normalize(vec_sub(light_pos, surf_pos));
	Ray shadow_ray = {surf_pos, light_direction};
	shadow_ray.origin = vec_add(shadow_ray.origin, vec_scale(surf_normal, SELF_OCCLUSION_DELTA));
	HitInfo shadow_ray_hit;
	bool is_in_shadow = intersect_scene(shadow_ray, &shadow_ray_hit, &(Sphere*){NULL});
	if (is_in_shadow) { // check if occluder is further away than light source
		double hit_dist_sq = shadow_ray_hit.t * shadow_ray_hit.t;
		double light_dist_sq = vec_length_squared(vec_sub(light_pos, shadow_ray.origin));
		if (hit_dist_sq > light_dist_sq) { // risk of self occlusion?
			is_in_shadow = false;
		}
	}
	return is_in_shadow;
}

// Calculates the refraction direction using Snell's Law from 11.2.9
// Also handles Total Internal Reflection.
Vec refract(Vec incident, Vec normal, double eta, bool* total_int_refl) {
	double cos_i = vec_dot(incident, normal);
	if (cos_i > 0.0) { printf("cos_i: %f\n", cos_i); }

	double k = 1 - eta * eta * (1 - cos_i * cos_i);
	*total_int_refl = k < 0;
	if (*total_int_refl) { // total internal reflection
		return reflect(incident, normal);
	} else {
		Vec refl_dir = vec_add(vec_scale(incident, eta), vec_scale(normal, eta * cos_i - sqrt(k)));
		return vec_normalize(refl_dir);
	}
}

Vec radiance_from_ray_simple(Ray r) {
	HitInfo hit;
	Sphere* hit_sphere = NULL;
	bool did_hit = intersect_scene(r, &hit, &hit_sphere);

	if (!did_hit) { return (Vec){0, 0, 0}; }

	Vec normal = hit.inside ? vec_scale(hit.n, -1.0) : hit.n; // if inside, flip normal

	Vec light_direction = vec_normalize(vec_sub(simple_light.position, hit.p));
	double cos_theta = vec_dot(normal, light_direction);
	if (cos_theta < 0.0) { cos_theta = 0.0; } // only upper hemisphere

	double dist_sq = vec_length_squared(vec_sub(simple_light.position, hit.p));
	double irradiance = (cos_theta * simple_light.radiant_flux) / (dist_sq * 4 * M_PI); // 10.1.5
	Vec colored_irradiance = vec_scale(simple_light.color, irradiance);
	// divide by pi for energy conservation 10.3.6
	// pi is the projected solid angle over hemisphere 3.1.9
	Vec sphere_color = hit_sphere->type == DIFFUSE ? hit_sphere->color : (Vec){1, 1, 1};
	Vec lambertian_brdf = vec_scale(sphere_color, 1.0 / M_PI);
	Vec radiance = vec_hadamard_prod(lambertian_brdf, colored_irradiance);
	return radiance;
}

// random double between 0.0 (inclusive) and 1.0 (exclusive)
double random_double() {
	return (double)rand() / (RAND_MAX + 1.0);
}

// create orthonormal basis (local coordinate system) from a vector
// 'n' is the normal vector, which will become the 'w' axis.
void create_orthonormal_basis(Vec n, Vec* u, Vec* v, Vec* w) {
	*w = n;
	Vec a = (fabs(w->x) > 0.9) ? (Vec){0, 1, 0} : (Vec){1, 0, 0};
	*v = vec_normalize(vec_cross(*w, a));
	*u = vec_cross(*w, *v);
}

// random direction on hemisphere with uniform distribution
Vec sample_uniform_hemisphere(Vec normal) {
	// Generate a random point on a unit sphere
	double r1 = random_double(); // for z
	double r2 = random_double(); // for phi

	double z = 1.0 - 2.0 * r1;
	double r = sqrt(fmax(0.0, 1.0 - z * z));
	double phi = 2.0 * M_PI * r2;

	Vec sample_local = {r * cos(phi), r * sin(phi), z};
	Vec u, v, w; // local coordinate system
	create_orthonormal_basis(normal, &u, &v, &w);
	// local -> world
	Vec sample_world = vec_add(vec_add(vec_scale(u, sample_local.x), vec_scale(v, sample_local.y)),
							   vec_scale(w, sample_local.z));

	// ensure the sample is in the correct hemisphere
	if (vec_dot(sample_world, normal) < 0.0) { return vec_scale(sample_world, -1.0); }
	return sample_world;
}

Vec radiance_from_ray(Ray r, int depth); // forward declaration for recursion
Vec radiance_from_ray(Ray r, int depth) {
	if (depth > MAX_DEPTH) { return (Vec){0, 0, 0}; }

	HitInfo hit;
	Sphere* hit_sphere = NULL;
	bool did_hit = intersect_scene(r, &hit, &hit_sphere);

	if (!did_hit) { return (Vec){0, 0, 0}; }

	switch (hit_sphere->type) {
	case EMISSIVE: {
		Vec radiosity = hit_sphere->color;
		Vec radiance = vec_scale(radiosity, 1.0 / M_PI);
		return radiance;
	}
	case DIFFUSE: {
		Vec normal = hit.inside ? vec_scale(hit.n, -1.0) : hit.n; // if inside, flip normal

		Vec next_direction = sample_uniform_hemisphere(normal);
		double cos_theta = vec_dot(normal, next_direction);
		assert(cos_theta >= 0.0); // only upper hemisphere

		Ray refl_ray = {hit.p, next_direction};
		refl_ray.origin = vec_add(refl_ray.origin, vec_scale(normal, SELF_OCCLUSION_DELTA));
		Vec incoming_radiance = radiance_from_ray(refl_ray, depth + 1);

		// divide by pi for energy conservation 10.3.6
		// pi is the projected solid angle over hemisphere 3.1.9
		Vec lambertian_brdf = vec_scale(hit_sphere->color, 1.0 / M_PI);
		// brdf * radiance * cos(theta)
		Vec radiance = vec_scale(vec_hadamard_prod(lambertian_brdf, incoming_radiance), cos_theta);
		// we now calculated the expected value, but because we integrate over the hemisphere
		// we still need to multiply with 2 pi (8.3.3)
		radiance = vec_scale(radiance, 2 * M_PI);
		return radiance;
	}
	case MIRROR: {
		Vec normal = hit.inside ? vec_scale(hit.n, -1.0) : hit.n; // if inside, flip normal
		// we don't implement a perfect mirror as a brdf
		// instead we describe perfect reflection as L_r = L_i * rho, where rho is just a ratio
		Ray refl_ray = {hit.p, reflect(r.dir, normal)};
		refl_ray.origin = vec_add(refl_ray.origin, vec_scale(normal, SELF_OCCLUSION_DELTA));
		Vec radiance = vec_hadamard_prod(radiance_from_ray(refl_ray, depth + 1), hit_sphere->color);
		return radiance;
	}
	case REFRACTIVE: {
		double ior = hit_sphere->color.x;
		double eta = hit.inside ? ior : 1.0 / ior;
		// Calculate how much light reflects using the Fresnel term.
		double reflectance = fresnel(r.dir, hit.n, hit.inside, ior);
		Vec normal = hit.inside ? vec_scale(hit.n, -1.0) : hit.n; // if inside, flip normal

		if (reflectance > random_double()) { // do either reflection or refraction
			// reflection
			Ray refl_ray = {hit.p, reflect(r.dir, normal)};
			refl_ray.origin = vec_add(refl_ray.origin, vec_scale(normal, SELF_OCCLUSION_DELTA));
			Vec reflection_radiance = radiance_from_ray(refl_ray, depth + 1);
			return reflection_radiance;
		} else {
			// refraction
			bool internal_refl;
			Vec refr_dir = refract(r.dir, normal, eta, &internal_refl);
			if (internal_refl) { // total internal reflection -> exit early
				return (Vec){0, 0, 0};
			}
			Ray refr_ray = {hit.p, refr_dir};
			refr_ray.origin = vec_add(refr_ray.origin, vec_scale(normal, -SELF_OCCLUSION_DELTA));
			Vec refraction_radiance = radiance_from_ray(refr_ray, depth + 1);
			return refraction_radiance;
		}
	}
	default: assert(false); // material type not implemented
	}
}

void write_image_tone_mapped() {
	// loop over pixels, do tone mapping and gamma correction
	for (int y = 0; y < height; ++y)
		for (int x = 0; x < width; ++x) {
			// Normalize the final color by dividing by the total sum of weights.
			// If this were a continuous integral, the sum of weights would be 1.0 and this
			// step unnecessary. But since we are doing a discrete sum, our total weight
			// will not be exactly 1.0, so we manually keep track of it.
			int radiance_index = y * width + x;
			Vec radiance = vec_scale(summed_weighted_radiance_buffer[radiance_index],
									 1.0 / summed_weights_buffer[radiance_index]);

			Vec ldr_color = reinhard_luminance(radiance); // tone mapping

			int image_index = radiance_index * 4;
			image_buffer[image_index + 0] = linear_to_srgb(ldr_color.x) * 255.999;
			image_buffer[image_index + 1] = linear_to_srgb(ldr_color.y) * 255.999;
			image_buffer[image_index + 2] = linear_to_srgb(ldr_color.z) * 255.999;
			image_buffer[image_index + 3] = (uint8_t)255.999;
		}
}

void write_image_raw() {
	for (int y = 0; y < height; ++y)
		for (int x = 0; x < width; ++x) {
			int radiance_index = y * width + x;
			Vec radiance = vec_scale(summed_weighted_radiance_buffer[radiance_index],
									 1.0 / summed_weights_buffer[radiance_index]);

			int image_index = radiance_index * 4;
			image_buffer[image_index + 0] = fmin(radiance.x, 1.0) * 255.999;
			image_buffer[image_index + 1] = fmin(radiance.y, 1.0) * 255.999;
			image_buffer[image_index + 2] = fmin(radiance.z, 1.0) * 255.999;
			image_buffer[image_index + 3] = (uint8_t)255.999;
		}
}

EMSCRIPTEN_KEEPALIVE
void render_init(int p_width, int p_height, double p_cam_angle_x, double p_cam_angle_y,
				 double p_cam_dist, double p_focus_x, double p_focus_y, double p_focus_z) {
	width = p_width;
	height = p_height;
	initialize_buffers();

	Vec focus_point = {p_focus_x, p_focus_y, p_focus_z};
	// Calculate Camera Position using spherical coordinates around the focus point
	double cam_x = focus_point.x + p_cam_dist * sin(p_cam_angle_y) * cos(p_cam_angle_x);
	double cam_y = focus_point.y + p_cam_dist * sin(p_cam_angle_x);
	double cam_z = focus_point.z + p_cam_dist * cos(p_cam_angle_y) * cos(p_cam_angle_x);
	camera_origin = (Vec){cam_x, cam_y, cam_z};

	// Create the cameras coordinate system (basis vectors) 5.1.6
	forward = vec_normalize(vec_sub(focus_point, camera_origin));
	Vec world_up = {0, 1, 0};
	right = vec_normalize(vec_cross(forward, world_up));
	up = vec_normalize(vec_cross(right, forward));

	sample_count = 0;
}

EMSCRIPTEN_KEEPALIVE
uint8_t* render_fast() {
	double aspect_ratio = (double)width / height;
	double fov_y = 30 * 3.141 / 180.0;
	double fov_scale = tan(fov_y / 2.0); // 5.1.4

	// loop over pixels, calculate radiance
	for (int y = 0; y < height; ++y)
		for (int x = 0; x < width; ++x) {
			// 5.2.2
			// Map pixel coordinates to the view plane (-1;1)
			double world_x = (2.0 * (x + 2.0) / width - 1.0);
			double world_y = 1.0 - 2.0 * (y + 2.0) / height;

			// Calculate the direction for the ray for this pixel
			Vec right_comp = vec_scale(right, world_x * fov_scale * aspect_ratio);
			Vec up_comp = vec_scale(up, world_y * fov_scale);
			Vec dir = vec_normalize(vec_add(forward, vec_add(right_comp, up_comp)));

			Ray r = {camera_origin, dir};
			Vec radiance = radiance_from_ray_simple(r);

			summed_weighted_radiance_buffer[(y)*width + (x)] = radiance;
			summed_weights_buffer[(y)*width + (x)] = 1.0;
		}

	write_image_tone_mapped();
	return image_buffer;
}

EMSCRIPTEN_KEEPALIVE
uint8_t* render_refine(unsigned int n_samples) {
	const double aspect_ratio = (double)width / height;
	const double fov_y = 30 * 3.141 / 180.0;
	const double fov_scale = tan(fov_y / 2.0); // 5.1.4

	// This factor scales our sample offsets to cover the desired range of the filter.
	// For sigma=0.5, radius=3 samples will range from -1.5 to 1.5
	const double sample_range_scale = GAUSS_SIGMA * GAUSS_FILTER_RADIUS_IN_SIGMA * 2.0;

	for (size_t sample_index = 0; sample_index < n_samples; ++sample_index) {
		// TODO samples should influence neighbouring pixels as well
		// will be a bit annoying to build, because we first have to collect samples
		// and then add them to the radiance buffer multiple times at different pixels.
		// also at the pixels at the edge will have less samples

		srand(sample_count); // seed random generator

		// loop over pixels, calculate radiance
		for (int y = 0; y < height; ++y)
			for (int x = 0; x < width; ++x) {
				if (x == 560 && y == 90) {
					// use for setting breakpoint
					// volatile tells the compiler not to remove it
					__asm__ __volatile__("nop");
				}

				// (-0.5;0.5) * sample_range_scale
				double sample_offset_x = (random_double() - 0.5) * sample_range_scale;
				double sample_offset_y = (random_double() - 0.5) * sample_range_scale;

				// 5.2.2
				// Map pixel coordinates to the view plane (-1;1)
				double world_x = (2.0 * (x + 0.5 + sample_offset_x) / width - 1.0);
				double world_y = 1.0 - 2.0 * (y + 0.5 + sample_offset_y) / height;

				// Calculate the direction for the ray for this pixel
				Vec right_comp = vec_scale(right, world_x * fov_scale * aspect_ratio);
				Vec up_comp = vec_scale(up, world_y * fov_scale);
				Vec dir = vec_normalize(vec_add(forward, vec_add(right_comp, up_comp)));

				Ray r = {camera_origin, dir};
				Vec radiance = radiance_from_ray(r, 0);

				double weight = gaussian_weight_2d(sample_offset_x, sample_offset_y, GAUSS_SIGMA);

				int index = y * width + x;
				summed_weighted_radiance_buffer[index] =
					vec_add(summed_weighted_radiance_buffer[index], vec_scale(radiance, weight));
				summed_weights_buffer[index] += weight;
				sample_count += 1;
			}
	}

	write_image_tone_mapped();
	return image_buffer;
}
