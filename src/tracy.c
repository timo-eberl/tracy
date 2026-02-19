#include "tracy.h"
#include "pcg_variants.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// Conditionally include emscripten.h and define EMSCRIPTEN_KEEPALIVE
#ifdef __EMSCRIPTEN__
// if VS Code says it can't find emscripten, you need to add its path to includePath
// something like /home/user/emsdk/upstream/emscripten/system/include
#include <emscripten.h>
#else
// For non-Emscripten builds (like GCC), define this as an empty macro so the compiler ignores it.
#define EMSCRIPTEN_KEEPALIVE
#endif

// Filter configuration constants
#define GAUSS_SIGMA 0.5
#define GAUSS_RADIUS 1.5 // 3 * Sigma, captures >99% of gaussian curves influence
#define MITCHELL_RADIUS 2.0
#define BOX_RADIUS 0.5

#define MAX_DEPTH 4

// If true, reinhard tonemapping and srgb conversion will be used. Otherwise raw (clamped) data will
// be written.
#define TONE_MAP true

// offset used for shadow rays. may need to be adjusted depending on scene scale
#define SELF_OCCLUSION_DELTA 0.00000001

// clang-format off
// allow one line typedefs
typedef struct { double x, y, z; } Vec;
typedef struct { Vec origin; Vec dir; } Ray;

// color is treated differently depending on the material type
// DIFFUSE: perfect lambertian diffuse, color=albedo
// EMISSIVE: only emission, color=radiosity (W/m^2)
// MIRROR: perfect reflection, color=rho, rho describes the ratio of reflected radiance
// REFRACTIVE: reflection and refraction, color.x=ior
typedef enum { DIFFUSE, EMISSIVE, MIRROR, REFRACTIVE } MaterialType;
// Geometry Data Structures
typedef struct { Vec center; double radius; } Sphere;
typedef struct { Vec v0, v1, v2; bool two_sided } Triangle;
typedef enum { SHAPE_SPHERE, SHAPE_TRIANGLE } ShapeType;
// The generic Scene Object
typedef struct {
	ShapeType type; Vec color; MaterialType material;
	union { Sphere sphere; Triangle triangle; } geo;
} Primitive;

// t: distance, p: point, n: normal, inside: flag
typedef struct { double t; Vec p; Vec n; bool inside; } HitInfo;
typedef enum { FILTER_BOX = 0, FILTER_GAUSSIAN = 1, FILTER_MITCHELL = 2 } FilterType;
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

// Emissive light: visible surface is 1 m^2, so 21.5 W flux correspond to
// 21.5 W/m^2 radiosity

// room dimensions: (3,2.4,4)

// clang-format off
Primitive scene[] = {
	// Left Wall (x = -1.5)
	{.type = SHAPE_TRIANGLE, .color = {0.75, 0.25, 0.25}, .material = DIFFUSE, .geo.triangle = {{-1.5, 0, 2.0}, {-1.5, 0, -2.0}, {-1.5, 2.4, -2.0}}},
	{.type = SHAPE_TRIANGLE, .color = {0.75, 0.25, 0.25}, .material = DIFFUSE, .geo.triangle = {{-1.5, 0, 2.0}, {-1.5, 2.4, -2.0}, {-1.5, 2.4, 2.0}}},
	// Right Wall (x = 1.5)
	{.type = SHAPE_TRIANGLE, .color = {0.25, 0.25, 0.75}, .material = DIFFUSE, .geo.triangle = {{1.5, 0, 2.0}, {1.5, 2.4, -2.0}, {1.5, 0, -2.0}}},
	{.type = SHAPE_TRIANGLE, .color = {0.25, 0.25, 0.75}, .material = DIFFUSE, .geo.triangle = {{1.5, 0, 2.0}, {1.5, 2.4, 2.0}, {1.5, 2.4, -2.0}}},
	// Back Wall (z = -2.0)
	{.type = SHAPE_TRIANGLE, .color = {0.75, 0.75, 0.75}, .material = DIFFUSE, .geo.triangle = {{-1.5, 0, -2.0}, {1.5, 0, -2.0}, {1.5, 2.4, -2.0}}},
	{.type = SHAPE_TRIANGLE, .color = {0.75, 0.75, 0.75}, .material = DIFFUSE, .geo.triangle = {{-1.5, 0, -2.0}, {1.5, 2.4, -2.0}, {-1.5, 2.4, -2.0}}},
	// Bottom (y = 0.0)
	{.type = SHAPE_TRIANGLE, .color = {0.75, 0.75, 0.75}, .material = DIFFUSE, .geo.triangle = {{-1.5, 0, 2.0}, {1.5, 0, 2.0}, {1.5, 0, -2.0}}},
	{.type = SHAPE_TRIANGLE, .color = {0.75, 0.75, 0.75}, .material = DIFFUSE, .geo.triangle = {{-1.5, 0, 2.0}, {1.5, 0, -2.0}, {-1.5, 0, -2.0}}},
	// Top (y = 2.4)
	{.type = SHAPE_TRIANGLE, .color = {0.75, 0.75, 0.75}, .material = DIFFUSE, .geo.triangle = {{-1.5, 2.4, 2.0}, {1.5, 2.4, -2.0}, {1.5, 2.4, 2.0}}},
	{.type = SHAPE_TRIANGLE, .color = {0.75, 0.75, 0.75}, .material = DIFFUSE, .geo.triangle = {{-1.5, 2.4, 2.0}, {-1.5, 2.4, -2.0}, {1.5, 2.4, -2.0}}},
	// Mirror Sphere
	{.type = SHAPE_SPHERE,   .color = {1.00, 1.00, 1.00}, .material = MIRROR, .geo.sphere = {.center = {-0.7, 0.5, -0.6}, .radius = 0.5}},
	// Glass Sphere
	{.type = SHAPE_SPHERE,   .color = {1.50, 0.00, 0.00}, .material = REFRACTIVE,  .geo.sphere = {.center = {0.7, 0.5, 0.6}, .radius = 0.5}},
	// Area Light (1x1m Rect)
	{.type = SHAPE_TRIANGLE, .color = {5 * 21.5, 5 * 21.5, 5 * 21.5}, .material = EMISSIVE, .geo.triangle = {{-0.5, 2.399, 0.5}, {0.5, 2.399, -0.5}, {0.5, 2.399, 0.5}}},
	{.type = SHAPE_TRIANGLE, .color = {5 * 21.5, 5 * 21.5, 5 * 21.5}, .material = EMISSIVE, .geo.triangle = {{-0.5, 2.399, 0.5}, {-0.5, 2.399, -0.5}, {0.5, 2.399, -0.5}}},
	// Light Shield - 4 Sides Angled 45 Degree
	// Side 1: Front
	{.type = SHAPE_TRIANGLE, .color = {0.1, 0.1, 0.1}, .material = DIFFUSE, .geo.triangle = {{-0.5, 2.4, 0.5}, {0.7, 2.2, 0.7}, {0.5, 2.4, 0.5}, true}},
	{.type = SHAPE_TRIANGLE, .color = {0.1, 0.1, 0.1}, .material = DIFFUSE, .geo.triangle = {{-0.5, 2.4, 0.5}, {-0.7, 2.2, 0.7}, {0.7, 2.2, 0.7}, true}},
	// Side 2: Right
	{.type = SHAPE_TRIANGLE, .color = {0.1, 0.1, 0.1}, .material = DIFFUSE, .geo.triangle = {{0.5, 2.4, 0.5}, {0.7, 2.2, -0.7}, {0.5, 2.4, -0.5}, true}},
	{.type = SHAPE_TRIANGLE, .color = {0.1, 0.1, 0.1}, .material = DIFFUSE, .geo.triangle = {{0.5, 2.4, 0.5}, {0.7, 2.2, 0.7}, {0.7, 2.2, -0.7}, true}},
	// Side 3: Back
	{.type = SHAPE_TRIANGLE, .color = {0.1, 0.1, 0.1}, .material = DIFFUSE, .geo.triangle = {{0.5, 2.4, -0.5}, {-0.7, 2.2, -0.7}, {-0.5, 2.4, -0.5}, true}},
	{.type = SHAPE_TRIANGLE, .color = {0.1, 0.1, 0.1}, .material = DIFFUSE, .geo.triangle = {{0.5, 2.4, -0.5}, {0.7, 2.2, -0.7}, {-0.7, 2.2, -0.7}, true}},
	// Side 4: Left
	{.type = SHAPE_TRIANGLE, .color = {0.1, 0.1, 0.1}, .material = DIFFUSE, .geo.triangle = {{-0.5, 2.4, -0.5}, {-0.7, 2.2, 0.7}, {-0.5, 2.4, 0.5}, true}},
	{.type = SHAPE_TRIANGLE, .color = {0.1, 0.1, 0.1}, .material = DIFFUSE, .geo.triangle = {{-0.5, 2.4, -0.5}, {-0.7, 2.2, -0.7}, {-0.7, 2.2, 0.7}, true}},
};
int num_primitives = sizeof(scene) / sizeof(Primitive);
// clang-format on

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
uint8_t* image_buffer_ldr = NULL;			 // stores tone mapped gamma corrected colors (rgba)
float* image_buffer_hdr = NULL;				 // stores linear averaged floats (rgb)
Vec* summed_weighted_radiance_buffer = NULL; // stores summed raw radiance
double* summed_weights_buffer = NULL;		 // stores the summed weights of the samples
int buffer_width = 0;
int buffer_height = 0;

// state variables
int width, height;
Vec camera_origin;
Vec forward, right, up;
int sample_count;		// Global counter of total samples processed (used for seeding)
FilterType filter_type; // Current selected filter

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

// ray-triangle intersection using Möller–Trumbore algorithm
bool intersect_triangle(Ray r, Triangle tri, HitInfo* hit) {
	const double EPSILON = 0.0000001;
	Vec edge1 = vec_sub(tri.v1, tri.v0);
	Vec edge2 = vec_sub(tri.v2, tri.v0);
	Vec h = vec_cross(r.dir, edge2);
	double a = vec_dot(edge1, h);

	// Check if ray is parallel to the triangle
	// Note: We perform double-sided intersection here.
	if (a > -EPSILON && a < EPSILON) { return false; }

	double f = 1.0 / a;
	Vec s = vec_sub(r.origin, tri.v0);
	double u = f * vec_dot(s, h);

	if (u < 0.0 || u > 1.0) { return false; }

	Vec q = vec_cross(s, edge1);
	double v = f * vec_dot(r.dir, q);

	if (v < 0.0 || u + v > 1.0) { return false; }

	double t = f * vec_dot(edge2, q);

	// Check if intersection is in front of the camera
	if (t > EPSILON) {
		hit->t = t;
		hit->p = vec_add(r.origin, vec_scale(r.dir, t));

		// Calculate geometric normal
		Vec n = vec_normalize(vec_cross(edge1, edge2));

		// Check orientation to set 'inside' flag correctly
		// If normal and ray point in the same direction, we are exiting the object (inside)
		if (vec_dot(r.dir, n) > 0.0) {
			if (tri.two_sided) {
				hit->inside = false;
				hit->n = vec_scale(n, -1.0);
			} else {
				hit->inside = true;
				hit->n = n;
			}
		} else {
			hit->inside = false;
			hit->n = n;
		}
		return true;
	}

	return false;
}

bool intersect_scene(Ray r, HitInfo* closest_hit, Primitive** hit_primitive) {
	closest_hit->t = INFINITY;
	*hit_primitive = NULL;

	for (int i = 0; i < num_primitives; ++i) {
		HitInfo current_hit;
		bool hit = false;

		switch (scene[i].type) {
		case SHAPE_SPHERE: hit = intersect_sphere(r, scene[i].geo.sphere, &current_hit); break;
		case SHAPE_TRIANGLE:
			hit = intersect_triangle(r, scene[i].geo.triangle, &current_hit);
			break;
		}

		if (hit && current_hit.t < closest_hit->t) {
			*closest_hit = current_hit;
			*hit_primitive = &scene[i];
		}
	}

	return (*hit_primitive != NULL);
}

// srgb response curve (4.1.9)
double linear_to_srgb(double v) {
	return (v <= 0.0031308) ? (12.92 * v) : (1.055 * pow(v, 0.416666667) - 0.055);
}
Vec vec_linear_to_srgb(Vec v) {
	return (Vec){linear_to_srgb(v.x), linear_to_srgb(v.y), linear_to_srgb(v.z)};
}

// for tone mapping we convert our color values to a single luminance value to combat
// the problem of washing out our image described in 4.2.5
double luminance(Vec rgb) {
	return 0.2126 * rgb.x + 0.7152 * rgb.y + 0.0722 * rgb.z;
}
// simple reinhard tone mapping operator based on luminance
Vec reinhard_luminance(Vec rgb_hdr) {
	double l_hdr = luminance(rgb_hdr);
	if (l_hdr <= 0.0) return (Vec){0, 0, 0}; // Handle black so we don't divide by 0
	double l_ldr = l_hdr / (1.0 + l_hdr);
	Vec v = vec_scale(rgb_hdr, l_ldr / l_hdr);
	return (Vec){fmin(v.x, 1.0), fmin(v.y, 1.0), fmin(v.z, 1.0)};
}
// 0-1 to 0-255
uint8_t quantize(double v) {
	v = (v < 0.0f) ? 0.0f : (v > 1.0f) ? 1.0f : v; // clamp 0 1
	return (uint8_t)(v * 255.999);
}

void initialize_buffers() {
	// (Re)allocate buffer if dimensions change or not allocated yet
	if (image_buffer_ldr == NULL || image_buffer_hdr == NULL ||
		summed_weighted_radiance_buffer == NULL || summed_weights_buffer == NULL ||
		width != buffer_width || height != buffer_height) {

		if (image_buffer_ldr != NULL) { free(image_buffer_ldr); }
		if (image_buffer_hdr != NULL) { free(image_buffer_hdr); }
		if (summed_weighted_radiance_buffer != NULL) { free(summed_weighted_radiance_buffer); }
		if (summed_weights_buffer != NULL) { free(summed_weights_buffer); }
		summed_weighted_radiance_buffer = malloc(width * height * sizeof(Vec));
		summed_weights_buffer = malloc(width * height * sizeof(double));
		image_buffer_ldr = malloc(width * height * 4 * sizeof(uint8_t));
		image_buffer_hdr = malloc(width * height * 3 * sizeof(float));
		buffer_width = width;
		buffer_height = height;
	}
	// write zeros in radiance buffers
	// no need to clear image_buffers as they are overwritten every time they are requested
	memset(summed_weighted_radiance_buffer, 0, width * height * sizeof(Vec));
	memset(summed_weights_buffer, 0, width * height * sizeof(double));
}

// 1D Box Filter
double box_1d(double x) {
	// half-open interval [-radius, radius). Ensures pixels on a boundary are not used for two
	// pixels, although in praxis this doesn't make any difference.
	return (x >= -BOX_RADIUS && x < BOX_RADIUS) ? 1.0 : 0.0;
}

// 1D Mitchell-Netravali filter function with B=1/3, C=1/3.
// This filter is separable: W(x,y) = w(x) * w(y)
// The support radius is strictly 2.0.
double mitchell_1d(double x) {
	x = fabs(x);
	if (x >= 2.0) return 0.0;

	// Hardcoded coefficients for B=1/3, C=1/3
	// 0 <= x < 1: 1/6 * (7x^3 - 12x^2 + 16/3)
	if (x < 1.0) {
		return (7.0 * x * x * x - 12.0 * x * x + 16.0 / 3.0) * (1.0 / 6.0);
	}
	// 1 <= x < 2: 1/6 * (-7/3x^3 + 12x^2 - 20x + 32/3)
	else {
		return ((-7.0 / 3.0) * x * x * x + 12.0 * x * x - 20.0 * x + 32.0 / 3.0) * (1.0 / 6.0);
	}
}

// Calculates a weight based on the 2D Gaussian PDF.
// This implements the formula: (1 / (2πσ²)) e^(-(x² + y²) / (2σ²))
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
	bool is_in_shadow = intersect_scene(shadow_ray, &shadow_ray_hit, &(Primitive*){NULL});
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
	double cos_i = -vec_dot(incident, normal);
	assert(cos_i > 0.0);

	double k = 1 - eta * eta * (1 - cos_i * cos_i);
	*total_int_refl = k < 0;
	if (*total_int_refl) { // total internal reflection
		return reflect(incident, normal);
	} else {
		Vec refl_dir = vec_add(vec_scale(incident, eta), vec_scale(normal, eta * cos_i - sqrt(k)));
		return vec_normalize(refl_dir);
	}
}

// random double between 0.0 (inclusive) and 1.0 (exclusive)
double random_double(pcg32_random_t* rng) {
	return (double)pcg32_random_r(rng) / 4294967296.0;
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
Vec sample_uniform_hemisphere(Vec normal, pcg32_random_t* rng) {
	// Generate a random point on a unit sphere
	double r1 = random_double(rng); // for z
	double r2 = random_double(rng); // for phi

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

// random direction on hemisphere proportional to cosine-weighted solid angle
Vec sample_cosine_hemisphere(Vec normal, pcg32_random_t* rng) {
	double r1 = random_double(rng);
	double r2 = random_double(rng);

	// Uniformly sample a disk
	double r = sqrt(r1);
	double phi = 2.0 * M_PI * r2;

	// Project disk to hemisphere (z = sqrt(1 - r^2))
	// In local space, z is the cosine of the angle with the normal
	Vec sample_local = {r * cos(phi), r * sin(phi), sqrt(fmax(0.0, 1.0 - r1))};
	Vec u, v, w; // local coordinate system
	create_orthonormal_basis(normal, &u, &v, &w);
	// local -> world
	Vec sample_world = vec_add(vec_add(vec_scale(u, sample_local.x), vec_scale(v, sample_local.y)),
							   vec_scale(w, sample_local.z));

	return sample_world;
}

Vec radiance_from_ray(Ray r, int depth, pcg32_random_t* rng); // forward declaration for recursion
Vec radiance_from_ray(Ray r, int depth, pcg32_random_t* rng) {
	if (depth > MAX_DEPTH) { return (Vec){0, 0, 0}; }

	HitInfo hit;
	Primitive* hit_prim = NULL;
	bool did_hit = intersect_scene(r, &hit, &hit_prim);

	if (!did_hit) { return (Vec){0, 0, 0}; }

	switch (hit_prim->material) {
	case EMISSIVE: {
		if (hit.inside) return (Vec){0}; // Only emit light in front facing direction

		Vec radiosity = hit_prim->color;
		Vec radiance = vec_scale(radiosity, 1.0 / M_PI);
		return radiance;
	}
	case DIFFUSE: {
		if (hit.inside) return (Vec){0}; // If inside, return 0

		Vec normal = hit.n;
		Vec next_direction = sample_cosine_hemisphere(normal, rng);

		Ray refl_ray = {hit.p, next_direction};
		refl_ray.origin = vec_add(refl_ray.origin, vec_scale(normal, SELF_OCCLUSION_DELTA));
		Vec incoming_radiance = radiance_from_ray(refl_ray, depth + 1, rng);

		// The PDF is (cos_theta / PI).
		// The estimator is: (Li * BRDF * cos_theta) / PDF. BRDF is (Color / PI).
		// Result: (Li * (Color / PI) * cos_theta) / (cos_theta / PI) == Li * Color
		Vec radiance = vec_hadamard_prod(hit_prim->color, incoming_radiance);
		return radiance;
	}
	case MIRROR: {
		Vec normal = hit.inside ? vec_scale(hit.n, -1.0) : hit.n; // if inside, flip normal
		// we don't implement a perfect mirror as a brdf
		// instead we describe perfect reflection as L_r = L_i * rho, where rho is just a ratio
		Ray refl_ray = {hit.p, reflect(r.dir, normal)};
		refl_ray.origin = vec_add(refl_ray.origin, vec_scale(normal, SELF_OCCLUSION_DELTA));
		Vec radiance =
			vec_hadamard_prod(radiance_from_ray(refl_ray, depth + 1, rng), hit_prim->color);
		return radiance;
	}
	case REFRACTIVE: {
		double ior = hit_prim->color.x;
		double eta = hit.inside ? ior : 1.0 / ior;
		// Calculate how much light reflects using the Fresnel term.
		double reflectance = fresnel(r.dir, hit.n, hit.inside, ior);
		Vec normal = hit.inside ? vec_scale(hit.n, -1.0) : hit.n; // if inside, flip normal

		if (reflectance > random_double(rng)) { // do either reflection or refraction
			// reflection
			Ray refl_ray = {hit.p, reflect(r.dir, normal)};
			refl_ray.origin = vec_add(refl_ray.origin, vec_scale(normal, SELF_OCCLUSION_DELTA));
			Vec reflection_radiance = radiance_from_ray(refl_ray, depth + 1, rng);
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
			Vec refraction_radiance = radiance_from_ray(refr_ray, depth + 1, rng);
			return refraction_radiance;
		}
	}
	default: assert(false); // material type not implemented
	}
}

void write_image(bool update_ldr, bool update_hdr) {
	// loop over pixels, do tone mapping and gamma correction
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int radiance_index = y * width + x;

			// Normalize the final color by dividing by the total sum of weights.
			// If this were a continuous integral, the sum of weights would be 1.0 and this
			// step unnecessary. But since we are doing a discrete sum, our total weight
			// will not be exactly 1.0, so we manually keep track of it.
			double weight = summed_weights_buffer[radiance_index];
			Vec radiance = (weight > 0) ? vec_scale(summed_weighted_radiance_buffer[radiance_index],
													1.0 / weight)
										: (Vec){0, 0, 0};

			if (update_hdr) {
				int image_index = radiance_index * 3; // HDR has 3 components (RGB)
				image_buffer_hdr[image_index + 0] = (float)radiance.x;
				image_buffer_hdr[image_index + 1] = (float)radiance.y;
				image_buffer_hdr[image_index + 2] = (float)radiance.z;
			}

			if (update_ldr) {
				int image_index = radiance_index * 4; // LDR has 4 components (RGBA)

				Vec ldr_color =
					TONE_MAP ? vec_linear_to_srgb(reinhard_luminance(radiance)) : radiance;

				image_buffer_ldr[image_index + 0] = quantize(ldr_color.x);
				image_buffer_ldr[image_index + 1] = quantize(ldr_color.y);
				image_buffer_ldr[image_index + 2] = quantize(ldr_color.z);
				image_buffer_ldr[image_index + 3] = quantize(1.0);
			}
		}
	}
}

EMSCRIPTEN_KEEPALIVE
uint8_t* update_image_ldr() {
	write_image(true, false); // update only LDR buffer
	return image_buffer_ldr;
}

EMSCRIPTEN_KEEPALIVE
float* update_image_hdr() {
	write_image(false, true); // update only HDR buffer
	return image_buffer_hdr;
}

EMSCRIPTEN_KEEPALIVE
void render_init(int p_width, int p_height, int p_filter_type, double p_cam_angle_x,
				 double p_cam_angle_y, double p_cam_dist, double p_focus_x, double p_focus_y,
				 double p_focus_z) {
	width = p_width;
	height = p_height;
	filter_type = (FilterType)p_filter_type;
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
void render_refine(unsigned int n_samples) {

	const double aspect_ratio = (double)width / height;
	const double fov_y = 30 * 3.141 / 180.0;
	const double fov_scale = tan(fov_y / 2.0); // 5.1.4

	double filter_radius;
	if (filter_type == FILTER_BOX) {
		filter_radius = BOX_RADIUS;
	} else if (filter_type == FILTER_GAUSSIAN) {
		filter_radius = GAUSS_RADIUS;
	} else if (filter_type == FILTER_MITCHELL) {
		filter_radius = MITCHELL_RADIUS;
	} else {
		assert(false); // filter not implemented
	}

	for (size_t sample_index = 0; sample_index < n_samples; ++sample_index) {
		// We do Sample Splatting: A single ray distributes weighted radiance to all neighboring
		// pixels within the filter radius (e.g. 2x2 block).

		// Optimization: To make this thread-safe without slow floating-point atomics, accumulate
		// into thread-local tile buffers (with padding/ghost zones) and merge them once the tile is
		// done.

		// For multi-threading, we need a stable base sample count for this pass to ensure
		// deterministic seeding independent of execution order.
		// Note: We use sample_count as "total pixels rendered" to seed.
		uint64_t base_seed = (uint64_t)sample_count;

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
		// loop over pixels, calculate radiance
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				if (x == 560 && y == 90) {
					// use for setting breakpoint
					// volatile tells the compiler not to remove it
					__asm__ __volatile__("nop");
				}

				// Thread-local RNG state
				pcg32_random_t rng_state;
				// Seed uniquely per pixel and per sample pass
				uint64_t seed = base_seed + (uint64_t)y * width + x;
				pcg32_srandom_r(&rng_state, seed, 54u);

				// Sample splatting strategy:
				// Pick a specific point on the continuous film plane within this pixel.
				// We jitter by [-0.5, 0.5] to cover the pixel area evenly.
				// TODO: Use a better more uniform distribution
				double jitter_x = random_double(&rng_state) - 0.5;
				double jitter_y = random_double(&rng_state) - 0.5;

				double film_x = x + 0.5 + jitter_x;
				double film_y = y + 0.5 + jitter_y;

				// 5.2.2
				// Map coordinates to the view plane (-1;1)
				double world_x = (2.0 * film_x / width - 1.0);
				double world_y = 1.0 - 2.0 * film_y / height;

				// Calculate the direction for the ray for this sample
				Vec right_comp = vec_scale(right, world_x * fov_scale * aspect_ratio);
				Vec up_comp = vec_scale(up, world_y * fov_scale);
				Vec dir = vec_normalize(vec_add(forward, vec_add(right_comp, up_comp)));

				Ray r = {camera_origin, dir};
				Vec radiance = radiance_from_ray(r, 0, &rng_state);

				// Distribute (Splat) the radiance to all neighboring pixels within filter range.
				// Determine the integer range of pixels where the pixel center (x + 0.5) falls
				// within the filter radius of the sample point (film_x, film_y).
				int min_nx = (int)ceil(film_x - 0.5 - filter_radius);
				int max_nx = (int)floor(film_x - 0.5 + filter_radius) + 1;
				int min_ny = (int)ceil(film_y - 0.5 - filter_radius);
				int max_ny = (int)floor(film_y - 0.5 + filter_radius) + 1;

				// with box filtering only the original pixel should be covered (at least with
				// radius 0.5 or lower)
				if (filter_type == FILTER_BOX && BOX_RADIUS <= 0.5) {
					assert(min_nx == x && max_nx == x + 1 && min_ny == y && max_ny == y + 1);
				}

				for (int ny = min_ny; ny < max_ny; ++ny) {
					for (int nx = min_nx; nx < max_nx; ++nx) {
						// Boundary check: ensure we don't write outside valid memory.
						// Note: Pixels at the very edge will receive less weight (fewer samples),
						// resulting in higher variance/noise at borders, but correct average.
						if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
							// Calculate weight based on distance from sample to neighbor pixel
							// center
							double dist_x = film_x - (nx + 0.5);
							double dist_y = film_y - (ny + 0.5);

							double weight;
							if (filter_type == FILTER_BOX) {
								weight = box_1d(dist_x) * box_1d(dist_y);
							} else if (filter_type == FILTER_GAUSSIAN) {
								weight = gaussian_weight_2d(dist_x, dist_y, GAUSS_SIGMA);
							} else if (filter_type == FILTER_MITCHELL) {
								weight = mitchell_1d(dist_x) * mitchell_1d(dist_y);
							} else {
								assert(false); // filter not implemented
							}

							int index = ny * width + nx;
							Vec weighted_rad = vec_scale(radiance, weight);

							// clang-format off
							#ifdef _OPENMP
							// Atomics are required here because multiple threads may splat
							// to the same neighbor pixel simultaneously.
							#pragma omp atomic
							summed_weighted_radiance_buffer[index].x += weighted_rad.x;
							#pragma omp atomic
							summed_weighted_radiance_buffer[index].y += weighted_rad.y;
							#pragma omp atomic
							summed_weighted_radiance_buffer[index].z += weighted_rad.z;

							#pragma omp atomic
							summed_weights_buffer[index] += weight;
							#else
							summed_weighted_radiance_buffer[index] =
								vec_add(summed_weighted_radiance_buffer[index], weighted_rad);
							summed_weights_buffer[index] += weight;
							#endif
							// clang-format on
						}
					}
				}
				// We can't safely increment the global sample_count here in parallel
			}
		}
		// Increment sample count by the number of pixels processed in this pass
		sample_count += width * height;
	}
}
