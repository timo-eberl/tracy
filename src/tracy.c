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
#define GAUSS_SIGMA 0.5f
#define GAUSS_RADIUS 1.5f // 3 * Sigma, captures >99% of gaussian curves influence
#define MITCHELL_RADIUS 2.0f
#define BOX_RADIUS 0.5f

#define RR_START_DEPTH 2 // Roussian Roulette starts after some samples

// If true, reinhard tonemapping and srgb conversion will be used. Otherwise raw (clamped) data will
// be written.
#define TONE_MAP true

// offset used for rays. may need to be adjusted depending on scene scale
#define SELF_OCCLUSION_DELTA 0.00001f
#define EPSILON 0.0001f

// clang-format off
// allow one line typedefs
typedef struct { float x, y, z; } Vec;
typedef struct { double x, y, z; } DVec;
typedef struct { Vec origin; Vec dir; } Ray;

typedef enum { SPHERE, TRIANGLE } ShapeType;
typedef struct { Vec center; float radius; } Sphere;
// one_sided triangles are only hit from their front side (CCW ordering)
typedef struct { Vec v0, v1, v2; Vec edge1, edge2; bool one_sided; } Triangle;
typedef struct {
	union { Sphere sphere; Triangle triangle; } data;
	ShapeType type;
} Shape;

typedef enum { DIFFUSE, EMISSIVE, MIRROR, REFRACTIVE } MaterialType;
typedef struct { Vec albedo; } DiffuseMaterial;
typedef struct { Vec radiosity; /* in W/m^2 */ } EmissiveMaterial;
typedef struct { Vec rho; /* describes the ratio of reflected radiance */ } MirrorMaterial;
typedef struct { float ior; /* index of refraction */ } RefractiveMaterial;
typedef struct {
	union {
		DiffuseMaterial diffuse; EmissiveMaterial emissive; MirrorMaterial mirror;
		RefractiveMaterial refractive;
	} data;
	MaterialType type;
	bool thin_wall; // thin geometry, backfaces are treated like frontfaces.
} Material;

// The generic Scene Object
typedef struct {
	Shape shape;
	Material material;
} Primitive;
typedef struct { Primitive* primitives; int size; } Scene;

// t: distance, p: point, n: normal, inside: flag
typedef struct { float t; Vec p; Vec n; bool inside; } HitInfo;
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
#define MAT_RED    (Material){.type = DIFFUSE, .data.diffuse.albedo = {0.75, 0.25, 0.25}}
#define MAT_BLUE   (Material){.type = DIFFUSE, .data.diffuse.albedo = {0.25, 0.25, 0.75}}
#define MAT_WHITE  (Material){.type = DIFFUSE, .data.diffuse.albedo = {0.75, 0.75, 0.75}}
#define MAT_MIRROR (Material){.type = MIRROR, .data.mirror.rho = {1,1,1}}
#define MAT_GLASS  (Material){.type = REFRACTIVE, .data.refractive.ior = 1.5}
#define MAT_LIGHT  (Material){.type = EMISSIVE, .data.emissive.radiosity = {5*21.5, 5*21.5, 5*21.5}}
#define MAT_SHIELD (Material){.type = DIFFUSE, .data.diffuse.albedo={0.1,0.1,0.1}, .thin_wall=true}

Primitive scene_cornell[] = {
	// Left Wall
	{.shape.type=TRIANGLE, .shape.data.triangle={{-1.5, 0,   2.0},{-1.5, 0,  -2.0},{-1.5, 2.4,-2.0},.one_sided=true}, .material=MAT_RED},
	{.shape.type=TRIANGLE, .shape.data.triangle={{-1.5, 0,   2.0},{-1.5, 2.4,-2.0},{-1.5, 2.4, 2.0},.one_sided=true}, .material=MAT_RED},
	// Right Wall
	{.shape.type=TRIANGLE, .shape.data.triangle={{ 1.5, 0,   2.0},{ 1.5, 2.4,-2.0},{ 1.5, 0,  -2.0},.one_sided=true}, .material=MAT_BLUE},
	{.shape.type=TRIANGLE, .shape.data.triangle={{ 1.5, 0,   2.0},{ 1.5, 2.4, 2.0},{ 1.5, 2.4,-2.0},.one_sided=true}, .material=MAT_BLUE},
	// Back Wall
	{.shape.type=TRIANGLE, .shape.data.triangle={{-1.5, 0,  -2.0},{ 1.5, 0,  -2.0},{ 1.5, 2.4,-2.0},.one_sided=true}, .material=MAT_WHITE},
	{.shape.type=TRIANGLE, .shape.data.triangle={{-1.5, 0,  -2.0},{ 1.5, 2.4,-2.0},{-1.5, 2.4,-2.0},.one_sided=true}, .material=MAT_WHITE},
	// Bottom
	{.shape.type=TRIANGLE, .shape.data.triangle={{-1.5, 0,   2.0},{ 1.5, 0,   2.0},{ 1.5, 0,  -2.0},.one_sided=true}, .material=MAT_WHITE},
	{.shape.type=TRIANGLE, .shape.data.triangle={{-1.5, 0,   2.0},{ 1.5, 0,  -2.0},{-1.5, 0,  -2.0},.one_sided=true}, .material=MAT_WHITE},
	// Top
	{.shape.type=TRIANGLE, .shape.data.triangle={{-1.5, 2.4, 2.0},{ 1.5, 2.4,-2.0},{ 1.5, 2.4, 2.0},.one_sided=true}, .material=MAT_WHITE},
	{.shape.type=TRIANGLE, .shape.data.triangle={{-1.5, 2.4, 2.0},{-1.5, 2.4,-2.0},{ 1.5, 2.4,-2.0},.one_sided=true}, .material=MAT_WHITE},
	// Mirror Sphere
	{.shape.type=SPHERE, .shape.data.sphere={.center={-0.7, 0.5,-0.6}, .radius=0.5}, .material=MAT_MIRROR},
	// Glass Sphere
	{.shape.type=SPHERE, .shape.data.sphere={.center={ 0.7, 0.5, 0.6}, .radius=0.5}, .material=MAT_GLASS},
	// Area Light (1x1m Rect)
	{.shape.type=TRIANGLE, .shape.data.triangle={{-0.5, 2.399, 0.5},{ 0.5, 2.399,-0.5},{ 0.5, 2.399, 0.5},.one_sided=true}, .material=MAT_LIGHT},
	{.shape.type=TRIANGLE, .shape.data.triangle={{-0.5, 2.399, 0.5},{-0.5, 2.399,-0.5},{ 0.5, 2.399,-0.5},.one_sided=true}, .material=MAT_LIGHT},
	// Light Shield - 4 Sides Angled 45 Degree
	// Side 1: Front
	{.shape.type=TRIANGLE, .shape.data.triangle={{-0.5, 2.4, 0.5},{ 0.7, 2.2, 0.7},{ 0.5, 2.4, 0.5}}, .material=MAT_SHIELD},
	{.shape.type=TRIANGLE, .shape.data.triangle={{-0.5, 2.4, 0.5},{-0.7, 2.2, 0.7},{ 0.7, 2.2, 0.7}}, .material=MAT_SHIELD},
	// Side 2: Right
	{.shape.type=TRIANGLE, .shape.data.triangle={{ 0.5, 2.4, 0.5},{ 0.7, 2.2,-0.7},{ 0.5, 2.4,-0.5}}, .material=MAT_SHIELD},
	{.shape.type=TRIANGLE, .shape.data.triangle={{ 0.5, 2.4, 0.5},{ 0.7, 2.2, 0.7},{ 0.7, 2.2,-0.7}}, .material=MAT_SHIELD},
	// Side 3: Back
	{.shape.type=TRIANGLE, .shape.data.triangle={{ 0.5, 2.4,-0.5},{-0.7, 2.2,-0.7},{-0.5, 2.4,-0.5}}, .material=MAT_SHIELD},
	{.shape.type=TRIANGLE, .shape.data.triangle={{ 0.5, 2.4,-0.5},{ 0.7, 2.2,-0.7},{-0.7, 2.2,-0.7}}, .material=MAT_SHIELD},
	// Side 4: Left
	{.shape.type=TRIANGLE, .shape.data.triangle={{-0.5, 2.4,-0.5},{-0.7, 2.2, 0.7},{-0.5, 2.4, 0.5}}, .material=MAT_SHIELD},
	{.shape.type=TRIANGLE, .shape.data.triangle={{-0.5, 2.4,-0.5},{-0.7, 2.2,-0.7},{-0.7, 2.2, 0.7}}, .material=MAT_SHIELD},
};

#define MAT_GROUND       (Material){.type = DIFFUSE, .data.diffuse.albedo = {0.75, 0.75, 0.75}, .thin_wall = true}
#define MAT_LIGHT_GREEN  (Material){.type = EMISSIVE, .data.emissive.radiosity = {1*21.5,5*21.5,1*21.5}}
#define MAT_LIGHT_PURPLE (Material){.type = EMISSIVE, .data.emissive.radiosity = {1*21.5,1*21.5,5*21.5}}

Primitive scene_caustics[] = {
	// Floor
	{.shape.type=TRIANGLE, .shape.data.triangle={{-2, 0, -2},{ 0, 0, 2},{ 2, 0, -2}}, .material=MAT_GROUND},
	// Glass
	{.shape.type=SPHERE, .shape.data.sphere={.center={ 0.0, 1.3, 0.0}, .radius=0.75}, .material=MAT_GLASS},
	{.shape.type=SPHERE, .shape.data.sphere={.center={ 0.3, 0.3, 0.0}, .radius=0.20}, .material=MAT_GLASS},
	{.shape.type=SPHERE, .shape.data.sphere={.center={-0.3, 0.3, 0.0}, .radius=0.20}, .material=MAT_GLASS},
	// Light
	{.shape.type=TRIANGLE, .shape.data.triangle={{-0.5, 5.0, 0.5},{ 0.5, 5.0,-0.5},{ 0.5, 5.0, 0.5},.one_sided=true}, .material=MAT_LIGHT_GREEN},
	{.shape.type=TRIANGLE, .shape.data.triangle={{-0.5, 5.0, 0.5},{-0.5, 5.0,-0.5},{ 0.5, 5.0,-0.5},.one_sided=true}, .material=MAT_LIGHT_PURPLE},
};
// clang-format on

Scene all_scenes[] = {
	{scene_cornell, sizeof(scene_cornell) / sizeof(Primitive)},
	{scene_caustics, sizeof(scene_caustics) / sizeof(Primitive)},
};

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// clang-format off
// KEEP COMPACT: Vector intrinsics are more readable as one-liners
Vec vec_add(Vec a, Vec b) { return (Vec){a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec vec_sub(Vec a, Vec b) { return (Vec){a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec vec_scale(Vec v, float s) { return (Vec){v.x * s, v.y * s, v.z * s}; }
Vec vec_hadamard_prod(Vec a, Vec b) { return (Vec){a.x * b.x, a.y * b.y, a.z * b.z}; }
float vec_dot(Vec a, Vec b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float vec_length(Vec v) { return sqrtf(vec_dot(v, v)); }
float vec_length_squared(Vec v) { return vec_dot(v, v); }
// clang-format on
Vec vec_normalize(Vec v) {
	float l = vec_length(v);
	if (l == 0.0f) return (Vec){0, 0, 0};
	return vec_scale(v, 1.0f / l);
}
Vec vec_cross(Vec a, Vec b) {
	return (Vec){
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x,
	};
}
float vec_max_component(Vec v) {
	return fmaxf(v.x, fmaxf(v.y, v.z));
}

// The image buffer will be allocated on demand.
uint8_t* image_buffer_ldr = NULL;			  // stores tone mapped gamma corrected colors (rgba)
float* image_buffer_hdr = NULL;				  // stores linear averaged floats (rgb)
DVec* summed_weighted_radiance_buffer = NULL; // stores summed raw radiance
double* summed_weights_buffer = NULL;		  // stores the summed weights of the samples
int buffer_width = 0;
int buffer_height = 0;

// state variables
Scene current_scene;
int max_depth;
int width, height;
Vec camera_origin;
Vec forward, right, up;
int sample_count;		// Global counter of total samples processed (used for seeding)
FilterType filter_type; // Current selected filter

void precompute_triangle(Triangle* tri) {
	tri->edge1 = vec_sub(tri->v1, tri->v0);
	tri->edge2 = vec_sub(tri->v2, tri->v0);
}

// 10.3.16
Vec reflect(Vec incident, Vec normal) {
	return vec_sub(incident, vec_scale(normal, 2.0f * vec_dot(normal, incident)));
}

// ray-sphere intersection (6.2.4)
bool intersect_sphere(const Ray* r, const Sphere* s, HitInfo* hit) {
	Vec oc = vec_sub(r->origin, s->center);
	float a = vec_dot(r->dir, r->dir);
	float b = 2.0f * vec_dot(oc, r->dir);
	float c = vec_dot(oc, oc) - s->radius * s->radius;
	// quadratic formula
	float discriminant = b * b - 4.0f * a * c;
	if (discriminant < 0.0f) {
		// cant take the square root of negative number
		// the quadratic formula has no solution -> no intersection
		return false;
	} else {
		float sqrtDiscriminant = sqrtf(discriminant);
		// we may have either one solution (t0==t1) or two solutions (t0 < t1)
		float t0 = (-b - sqrtDiscriminant) / (2.0f * a);
		float t1 = (-b + sqrtDiscriminant) / (2.0f * a);
		// we want to know about the closest intersection in front of the camera
		// -> smallest t value
		// however a negative t value would mean an intersection behind the camera
		// -> ignore negative t value
		// therefore we return the smallest positive t value
		if (t1 <= 0.0f) {
			return false; // both intersections are behind ray
		}
		hit->t = t0 > 0.0f ? t0 : t1; // if the first intersection is behind ray, use the other
		hit->p = vec_add(r->origin, vec_scale(r->dir, hit->t));
		hit->n = vec_normalize(vec_sub(hit->p, s->center));
		hit->inside = !(t0 > 0.0f);
		return true;
	}
}

// ray-triangle intersection using Möller–Trumbore algorithm
// ray-triangle intersection using Möller–Trumbore algorithm
bool intersect_triangle(const Ray* r, const Triangle* tri, HitInfo* hit) {
	Vec h = vec_cross(r->dir, tri->edge2);
	float a = vec_dot(tri->edge1, h);

	// If we only care about front-faces, we can exit early
	if (tri->one_sided && a < EPSILON) return false;

	// Check if ray is parallel to the triangle
	// We perform double-sided intersection here.
	if (a > -EPSILON && a < EPSILON) { return false; }

	float f = 1.0f / a;
	Vec s = vec_sub(r->origin, tri->v0);
	float u = f * vec_dot(s, h);

	if (u < 0.0f || u > 1.0f) { return false; }

	Vec q = vec_cross(s, tri->edge1);
	float v = f * vec_dot(r->dir, q);

	if (v < 0.0f || u + v > 1.0f) { return false; }

	float t = f * vec_dot(tri->edge2, q);

	// Check if intersection is in front of the camera
	if (t > EPSILON) {
		hit->t = t;
		hit->p = vec_add(r->origin, vec_scale(r->dir, t));

		// Calculate geometric normal
		Vec n = vec_normalize(vec_cross(tri->edge1, tri->edge2));

		// Check orientation to set 'inside' flag correctly
		// If normal and ray point in the same direction, we are exiting the object (inside)
		hit->inside = (vec_dot(r->dir, n) > 0.0f);
		hit->n = n;

		return true;
	}

	return false;
}

bool intersect_scene(const Ray* r, HitInfo* closest_hit, Primitive** hit_primitive) {
	closest_hit->t = INFINITY;
	*hit_primitive = NULL;

	for (int i = 0; i < current_scene.size; ++i) {
		HitInfo current_hit;
		bool hit = false;

		switch (current_scene.primitives[i].shape.type) {
		case SPHERE:
			hit = intersect_sphere(r, &current_scene.primitives[i].shape.data.sphere, &current_hit);
			break;
		case TRIANGLE:
			hit = intersect_triangle(r, &current_scene.primitives[i].shape.data.triangle,
									 &current_hit);
			break;
		}

		if (hit && current_hit.t < closest_hit->t) {
			*closest_hit = current_hit;
			*hit_primitive = &current_scene.primitives[i];
		}
	}

	return (*hit_primitive != NULL);
}

// srgb response curve (4.1.9)
float linear_to_srgb(float v) {
	return (v <= 0.0031308f) ? (12.92f * v) : (1.055f * powf(v, 0.416666667f) - 0.055f);
}
Vec vec_linear_to_srgb(Vec v) {
	return (Vec){linear_to_srgb(v.x), linear_to_srgb(v.y), linear_to_srgb(v.z)};
}

// for tone mapping we convert our color values to a single luminance value to combat
// the problem of washing out our image described in 4.2.5
float luminance(Vec rgb) {
	return 0.2126f * rgb.x + 0.7152f * rgb.y + 0.0722f * rgb.z;
}
// simple reinhard tone mapping operator based on luminance
Vec reinhard_luminance(Vec rgb_hdr) {
	float l_hdr = luminance(rgb_hdr);
	if (l_hdr <= 0.0f) return (Vec){0, 0, 0}; // Handle black so we don't divide by 0
	float l_ldr = l_hdr / (1.0f + l_hdr);
	Vec v = vec_scale(rgb_hdr, l_ldr / l_hdr);
	return (Vec){fminf(v.x, 1.0f), fminf(v.y, 1.0f), fminf(v.z, 1.0f)};
}
// 0-1 to 0-255
uint8_t quantize(float v) {
	v = (v < 0.0f) ? 0.0f : (v > 1.0f) ? 1.0f : v; // clamp 0 1
	return (uint8_t)(v * 255.999f);
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
		summed_weighted_radiance_buffer = malloc(width * height * sizeof(DVec));
		summed_weights_buffer = malloc(width * height * sizeof(double));
		image_buffer_ldr = malloc(width * height * 4 * sizeof(uint8_t));
		image_buffer_hdr = malloc(width * height * 3 * sizeof(float));
		buffer_width = width;
		buffer_height = height;
	}
	// write zeros in radiance buffers
	// no need to clear image_buffers as they are overwritten every time they are requested
	memset(summed_weighted_radiance_buffer, 0, width * height * sizeof(DVec));
	memset(summed_weights_buffer, 0, width * height * sizeof(double));
}

// 1D Box Filter
float box_1d(float x) {
	// half-open interval[-radius, radius). Ensures pixels on a boundary are not used for two
	// pixels, although in praxis this doesn't make any difference.
	return (x >= -BOX_RADIUS && x < BOX_RADIUS) ? 1.0f : 0.0f;
}

// 1D Mitchell-Netravali filter function with B=1/3, C=1/3.
// This filter is separable: W(x,y) = w(x) * w(y)
// The support radius is strictly 2.0.
float mitchell_1d(float x) {
	x = fabsf(x);
	if (x >= 2.0f) return 0.0f;

	// Hardcoded coefficients for B=1/3, C=1/3
	// 0 <= x < 1: 1/6 * (7x^3 - 12x^2 + 16/3)
	if (x < 1.0f) {
		return (7.0f * x * x * x - 12.0f * x * x + 16.0f / 3.0f) * (1.0f / 6.0f);
	}
	// 1 <= x < 2: 1/6 * (-7/3x^3 + 12x^2 - 20x + 32/3)
	else {
		return ((-7.0f / 3.0f) * x * x * x + 12.0f * x * x - 20.0f * x + 32.0f / 3.0f) *
			   (1.0f / 6.0f);
	}
}

// Calculates a weight based on the 2D Gaussian PDF.
// This implements the formula: (1 / (2πσ²)) e^(-(x² + y²) / (2σ²))
float gaussian_weight_2d(float offset_x, float offset_y, float sigma) {
	float two_sigma_squared = 2.0f * sigma * sigma;
	// normalization constant: (1 / (2πσ²))
	float norm_const = 1.0f / ((float)M_PI * two_sigma_squared);
	float r_squared = offset_x * offset_x + offset_y * offset_y;
	return norm_const * expf(-r_squared / two_sigma_squared);
}

// Calculates the Fresnel reflectance amount using Schlick's approximation.
// Determines how much light reflects vs. refracts.
float fresnel(Vec incident, Vec normal, bool is_inside, float ior) {
	float R0 = ((1.0f - ior) / (1.0f + ior)) * ((1.0f - ior) / (1.0f + ior));
	float cos_i = -vec_dot(incident, normal);
	// If we're inside the medium, use the correct IOR for the calculation
	if (is_inside) {
		cos_i = -cos_i; // This can happen if ray is inside the sphere
		// We must use the transmitted angle for Schlick when going from Denser -> Rarer
		float sin2_t = ior * ior * (1.0f - cos_i * cos_i);
		if (sin2_t > 1.0f) return 1.0f; // Total Internal Reflection
		cos_i = sqrtf(1.0f - sin2_t);
	}

	return R0 + (1.0f - R0) * powf(1.0f - cos_i, 5.0f);
}

// Calculates the refraction direction using Snell's Law from 11.2.9
// Also handles Total Internal Reflection.
Vec refract(Vec incident, Vec normal, float eta, bool* total_int_refl) {
	float cos_i = -vec_dot(incident, normal);
	assert(cos_i > 0.0f);

	float k = 1.0f - eta * eta * (1.0f - cos_i * cos_i);
	*total_int_refl = k < 0.0f;
	if (*total_int_refl) { // total internal reflection
		return reflect(incident, normal);
	} else {
		Vec refl_dir = vec_add(vec_scale(incident, eta), vec_scale(normal, eta * cos_i - sqrtf(k)));
		return vec_normalize(refl_dir);
	}
}

// random float between 0.0 (inclusive) and 1.0 (exclusive)
float random_float(pcg32_random_t* rng) {
	// Shift out the bottom 8 bits to get a perfect 24-bit integer
	// Multiply by 2^-24 (which is exactly 1.0f / 16777216.0f)
	return (pcg32_random_r(rng) >> 8) * 0x1.0p-24f;
}

// create orthonormal basis (local coordinate system) from a vector
// 'n' is the normal vector, which will become the 'w' axis.
void create_orthonormal_basis(Vec n, Vec* u, Vec* v, Vec* w) {
	*w = n;
	Vec a = (fabsf(w->x) > 0.9f) ? (Vec){0, 1, 0} : (Vec){1, 0, 0};
	*v = vec_normalize(vec_cross(*w, a));
	*u = vec_cross(*w, *v);
}

// random direction on hemisphere with uniform distribution
Vec sample_uniform_hemisphere(Vec normal, pcg32_random_t* rng) {
	// Generate a random point on a unit sphere
	float r1 = random_float(rng); // for z
	float r2 = random_float(rng); // for phi

	float z = 1.0f - 2.0f * r1;
	float r = sqrtf(fmaxf(0.0f, 1.0f - z * z));
	float phi = 2.0f * (float)M_PI * r2;

	Vec sample_local = {r * cosf(phi), r * sinf(phi), z};
	Vec u, v, w; // local coordinate system
	create_orthonormal_basis(normal, &u, &v, &w);
	// local -> world
	Vec sample_world = vec_add(vec_add(vec_scale(u, sample_local.x), vec_scale(v, sample_local.y)),
							   vec_scale(w, sample_local.z));

	// ensure the sample is in the correct hemisphere
	if (vec_dot(sample_world, normal) < 0.0f) { return vec_scale(sample_world, -1.0f); }
	return sample_world;
}

// random direction on hemisphere proportional to cosine-weighted solid angle
Vec sample_cosine_hemisphere(Vec normal, pcg32_random_t* rng) {
	float r1 = random_float(rng);
	float r2 = random_float(rng);

	// Uniformly sample a disk
	float r = sqrtf(r1);
	float phi = 2.0f * (float)M_PI * r2;

	// Project disk to hemisphere (z = sqrt(1 - r^2))
	// In local space, z is the cosine of the angle with the normal
	Vec sample_local = {r * cosf(phi), r * sinf(phi), sqrtf(fmaxf(0.0f, 1.0f - r1))};
	Vec u, v, w; // local coordinate system
	create_orthonormal_basis(normal, &u, &v, &w);
	// local -> world
	Vec sample_world = vec_add(vec_add(vec_scale(u, sample_local.x), vec_scale(v, sample_local.y)),
							   vec_scale(w, sample_local.z));

	return sample_world;
}

float clamp_survival_probability(float probability) {
	// Clamp probability to ensure we don't divide by zero or kill too aggressively
	if (probability < 0.1f) return 0.1f;
	if (probability > 0.95f) return 0.95f;
	return probability;
}

Vec radiance_from_ray(Ray r, int depth, pcg32_random_t* rng); // forward declaration for recursion
Vec radiance_from_ray(Ray r, int depth, pcg32_random_t* rng) {
	if (depth + 1 > max_depth) { return (Vec){0, 0, 0}; }

	HitInfo hit;
	Primitive* hit_prim = NULL;
	bool did_hit = intersect_scene(&r, &hit, &hit_prim);

	if (!did_hit) { return (Vec){0, 0, 0}; }

	// Handle thin walls (think of paper or leaves)
	// If we hit the backface of a thin-walled object, treat it as a frontface
	if (hit_prim->material.thin_wall && hit.inside) {
		hit.n = vec_scale(hit.n, -1.0f);
		hit.inside = false;
	}

	switch (hit_prim->material.type) {
	case EMISSIVE: {
		if (hit.inside) return (Vec){0}; // Only emit light in front facing direction

		Vec radiosity = hit_prim->material.data.emissive.radiosity;
		Vec radiance = vec_scale(radiosity, 1.0f / (float)M_PI);
		return radiance;
	}
	case DIFFUSE: {
		if (hit.inside) return (Vec){0}; // If inside, return 0

		Vec normal = hit.n;

		float survival_prob = 1.0f; // Default to 100% survival
#ifdef ENABLE_RUSSIAN_ROULETTE
		if (depth >= RR_START_DEPTH) {
			// Probability is based on how 'bright' the surface is.
			// Darker surfaces are more likely to terminate.
			survival_prob = clamp_survival_probability(vec_max_component(hit_prim->color));
			// Terminate based on survival probability
			if (random_float(rng) > survival_prob) { return (Vec){0, 0, 0}; }
		}
#endif

		Vec next_direction = sample_cosine_hemisphere(normal, rng);

		Ray refl_ray = {hit.p, next_direction};
		refl_ray.origin = vec_add(refl_ray.origin, vec_scale(normal, SELF_OCCLUSION_DELTA));
		Vec incoming_radiance = radiance_from_ray(refl_ray, depth + 1, rng);

		// russian roulette bias correction: scale the albedo by the inverse probability to
		// compensate for killed rays.
		Vec albedo = vec_scale(hit_prim->material.data.diffuse.albedo, 1.0f / survival_prob);

		// The PDF is (cos_theta / PI).
		// The estimator is: (Li * BRDF * cos_theta) / PDF. BRDF is (Color / PI).
		// Result: (Li * (Color / PI) * cos_theta) / (cos_theta / PI) == Li * Color
		return vec_hadamard_prod(albedo, incoming_radiance);
	}
	case MIRROR: {
		Vec normal = hit.inside ? vec_scale(hit.n, -1.0f) : hit.n; // if inside, flip normal

		float survival_prob = 1.0f;
#ifdef ENABLE_RUSSIAN_ROULETTE
		if (depth >= RR_START_DEPTH) {
			// Mirrors are usually bright, but if it's a dark mirror, we might kill it
			survival_prob = clamp_survival_probability(vec_max_component(hit_prim->color));
			// Terminate based on survival probability
			if (random_float(rng) > survival_prob) { return (Vec){0, 0, 0}; }
		}
#endif

		// we don't implement a perfect mirror as a brdf
		// instead we describe perfect reflection as L_r = L_i * rho, where rho is just a ratio
		Ray refl_ray = {hit.p, reflect(r.dir, normal)};
		refl_ray.origin = vec_add(refl_ray.origin, vec_scale(normal, SELF_OCCLUSION_DELTA));
		Vec incoming = radiance_from_ray(refl_ray, depth + 1, rng);
		// russian roulette bias correction
		Vec rho = vec_scale(hit_prim->material.data.mirror.rho, 1.0f / survival_prob);
		return vec_hadamard_prod(incoming, rho);
	}
	case REFRACTIVE: {
		float ior = hit_prim->material.data.refractive.ior;
		float eta = hit.inside ? ior : 1.0f / ior;
		// Calculate how much light reflects using the Fresnel term.
		float reflectance = fresnel(r.dir, hit.n, hit.inside, ior);
		Vec normal = hit.inside ? vec_scale(hit.n, -1.0f) : hit.n; // if inside, flip normal

		if (reflectance > random_float(rng)) { // do either reflection or refraction
			// reflection
			Ray refl_ray = {hit.p, reflect(r.dir, normal)};
			refl_ray.origin = vec_add(refl_ray.origin, vec_scale(normal, SELF_OCCLUSION_DELTA));
			Vec reflection_radiance = radiance_from_ray(refl_ray, depth + 1, rng);
			return reflection_radiance;
		} else {
			// refraction
			if (hit_prim->material.thin_wall) {
				// Ray passing through a thin wall bends twice, cancelling the angle out -> ray
				// passes straight through
				Ray refr_ray = {hit.p, r.dir};
				refr_ray.origin = vec_add(refr_ray.origin, vec_scale(r.dir, SELF_OCCLUSION_DELTA));
				return radiance_from_ray(refr_ray, depth + 1, rng);
			}

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
			Vec radiance =
				(weight > 0.0)
					? vec_scale((Vec){(float)summed_weighted_radiance_buffer[radiance_index].x,
									  (float)summed_weighted_radiance_buffer[radiance_index].y,
									  (float)summed_weighted_radiance_buffer[radiance_index].z},
								1.0f / (float)weight)
					: (Vec){0, 0, 0};

			if (update_hdr) {
				int image_index = radiance_index * 3; // HDR has 3 components (RGB)
				image_buffer_hdr[image_index + 0] = radiance.x;
				image_buffer_hdr[image_index + 1] = radiance.y;
				image_buffer_hdr[image_index + 2] = radiance.z;
			}

			if (update_ldr) {
				int image_index = radiance_index * 4; // LDR has 4 components (RGBA)

				Vec ldr_color =
					TONE_MAP ? vec_linear_to_srgb(reinhard_luminance(radiance)) : radiance;

				image_buffer_ldr[image_index + 0] = quantize(ldr_color.x);
				image_buffer_ldr[image_index + 1] = quantize(ldr_color.y);
				image_buffer_ldr[image_index + 2] = quantize(ldr_color.z);
				image_buffer_ldr[image_index + 3] = quantize(1.0f);
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
void render_init(int p_scene_id, int p_max_depth, int p_width, int p_height, int p_filter_type,
				 double p_cam_angle_x, double p_cam_angle_y, double p_cam_dist, double p_focus_x,
				 double p_focus_y, double p_focus_z) {

	int num_available_scenes = sizeof(all_scenes) / sizeof(Scene);
	current_scene = (p_scene_id >= 0 && p_scene_id < num_available_scenes) ? all_scenes[p_scene_id]
																		   : (Scene){0};
	// Precompute triangle edges
	for (int i = 0; i < current_scene.size; ++i) {
		if (current_scene.primitives[i].shape.type == TRIANGLE) {
			precompute_triangle(&current_scene.primitives[i].shape.data.triangle);
		}
	}

	max_depth = p_max_depth;
	width = p_width;
	height = p_height;
	filter_type = (FilterType)p_filter_type;
	initialize_buffers();

	Vec focus_point = {(float)p_focus_x, (float)p_focus_y, (float)p_focus_z};
	// Calculate Camera Position using spherical coordinates around the focus point
	float cam_x = (float)(p_focus_x + p_cam_dist * sin(p_cam_angle_y) * cos(p_cam_angle_x));
	float cam_y = (float)(p_focus_y + p_cam_dist * sin(p_cam_angle_x));
	float cam_z = (float)(p_focus_z + p_cam_dist * cos(p_cam_angle_y) * cos(p_cam_angle_x));
	camera_origin = (Vec){cam_x, cam_y, cam_z};

	// Create the cameras coordinate system (basis vectors) 5.1.6
	forward = vec_normalize(vec_sub(focus_point, camera_origin));
	Vec world_up = {0, 1.0f, 0};
	right = vec_normalize(vec_cross(forward, world_up));
	up = vec_normalize(vec_cross(right, forward));

	sample_count = 0;
}

EMSCRIPTEN_KEEPALIVE
void render_refine(unsigned int n_samples) {

	const float aspect_ratio = (float)width / height;
	const float fov_y = 30.0f * 3.141f / 180.0f;
	const float fov_scale = tanf(fov_y / 2.0f); // 5.1.4

	float filter_radius;
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
				// We jitter by[-0.5, 0.5) to cover the pixel area evenly.
				// TODO: Use a better more uniform distribution
				float jitter_x = random_float(&rng_state) - 0.5f;
				float jitter_y = random_float(&rng_state) - 0.5f;

				float film_x = x + (0.5f + jitter_x);
				float film_y = y + (0.5f + jitter_y);

				// 5.2.2
				// Map coordinates to the view plane (-1;1)
				float world_x = (2.0f * film_x / width - 1.0f);
				float world_y = 1.0f - 2.0f * film_y / height;

				// Calculate the direction for the ray for this sample
				Vec right_comp = vec_scale(right, world_x * fov_scale * aspect_ratio);
				Vec up_comp = vec_scale(up, world_y * fov_scale);
				Vec dir = vec_normalize(vec_add(forward, vec_add(right_comp, up_comp)));

				Ray r = {camera_origin, dir};
				Vec radiance = radiance_from_ray(r, 0, &rng_state);

				// Distribute (Splat) the radiance to all neighboring pixels within filter range.
				// Determine the integer range of pixels where the pixel center (x + 0.5) falls
				// within the filter radius of the sample point (film_x, film_y).
				int min_nx = x + (int)floorf(jitter_x - filter_radius) + 1;
				int max_nx = x + (int)floorf(jitter_x + filter_radius) + 1;
				int min_ny = y + (int)floorf(jitter_y - filter_radius) + 1;
				int max_ny = y + (int)floorf(jitter_y + filter_radius) + 1;

				// with box filtering only the original pixel should be covered (at least with
				// radius 0.5 or lower)
				if (filter_type == FILTER_BOX && BOX_RADIUS <= 0.5f) {
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
							float dist_x = (x - nx) + jitter_x;
							float dist_y = (y - ny) + jitter_y;

							float weight;
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
							summed_weighted_radiance_buffer[index].x += (double)weighted_rad.x;
							#pragma omp atomic
							summed_weighted_radiance_buffer[index].y += (double)weighted_rad.y;
							#pragma omp atomic
							summed_weighted_radiance_buffer[index].z += (double)weighted_rad.z;

							#pragma omp atomic
							summed_weights_buffer[index] += (double)weight;
							#else
							summed_weighted_radiance_buffer[index].x += (double)weighted_rad.x;
							summed_weighted_radiance_buffer[index].y += (double)weighted_rad.y;
							summed_weighted_radiance_buffer[index].z += (double)weighted_rad.z;
							summed_weights_buffer[index] += (double)weight;
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
