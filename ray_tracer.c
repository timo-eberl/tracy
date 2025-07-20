#include <emscripten.h>
#include <stdlib.h>
#include <math.h>

// The image buffer will be allocated on demand.
unsigned char* image_buffer = NULL;
int buffer_width = 0;
int buffer_height = 0;

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
	{{0, 0, 0}, 1, {255, 0, 0}},
	{{-2, 0, 2}, 1, {0, 255, 0}},
	{{2, 0, 2}, 1, {0, 0, 255}}
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

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			// Map pixel coordinates to the view plane
			// (-aspect_ratio, -1) to (aspect_ratio, 1)
			double world_x = (2.0 * (x + 0.5) / width - 1.0) * aspect_ratio;
			double world_y = 1.0 - 2.0 * (y + 0.5) / height;

			// Calculate the direction for the ray for this pixel
			Vec right_comp = vec_scale(right, world_x);
			Vec up_comp = vec_scale(up, world_y);
			Vec dir = vec_normalize(vec_add(forward, vec_add(right_comp, up_comp)));

			Ray r = {camera_origin, dir};

			double min_t = INFINITY;
			Sphere* hit_sphere = NULL;

			for (int i = 0; i < num_spheres; ++i) {
				double t = intersect(r, scene[i]);
				if (t > 0 && t < min_t) {
					min_t = t;
					hit_sphere = &scene[i];
				}
			}

			int index = (y * width + x) * 4;
			if (hit_sphere) {
				buffer[index + 0] = hit_sphere->color.x;
				buffer[index + 1] = hit_sphere->color.y;
				buffer[index + 2] = hit_sphere->color.z;
				buffer[index + 3] = 255;
			} else {
				buffer[index + 0] = 173;
				buffer[index + 1] = 216;
				buffer[index + 2] = 230;
				buffer[index + 3] = 255;
			}
		}
	}

	return buffer;
}
