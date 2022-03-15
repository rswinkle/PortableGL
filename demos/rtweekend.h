#include "rsw_math.h"

#include <cmath>
#include <limits>
#include <memory>
#include <vector>

// I know I know, I shouldn't use these using directives/declarations
// I'll probably merge some modified version of some of this
// into rsw_math and crsw_math later and maybe make a proper raytracing
// header too
using std::shared_ptr;
using std::make_shared;
using std::sqrt;

using rsw::vec3;
using rsw::dot;


// Constants
const double infinity = std::numeric_limits<float>::infinity();
const double pi = 3.14159265f;

// forward declarations
struct material;

struct ray
{
	vec3 origin;
	vec3 dir;
	float tm;

	ray() {}
	ray(vec3 Origin, vec3 direction) : origin(Origin), dir(direction), tm(0) {}
	ray(vec3 Origin, vec3 direction, float time)
	    : origin(Origin), dir(direction), tm(time) {}

	vec3 at(float t) const {
		return origin + t * dir;
	}
};

struct hit_record
{
	vec3 p;
	vec3 normal;
	shared_ptr<material> mat_ptr;
	float t;
	bool front_face;

	void set_face_normal(const ray& r, const vec3& outward_normal)
	{
		front_face = dot(r.dir, outward_normal) < 0;
		normal = front_face ? outward_normal : -outward_normal;
	}
};

struct hittable
{
	virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const = 0;
};

struct sphere : public hittable
{
	vec3 center;
	float radius;
	shared_ptr<material> mat_ptr;

	sphere() {}
	sphere(vec3 cen, float r, shared_ptr<material> m) : center(cen), radius(r), mat_ptr(m) {};

	virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const override;
};

bool sphere::hit(const ray& r, float t_min, float t_max, hit_record& rec) const
{
	vec3 oc = r.origin - center;
	float a = r.dir.len_squared();
	float half_b = dot(oc, r.dir);
	float c = oc.len_squared() - radius*radius;

	float discriminant = half_b*half_b - a*c;
	if (discriminant < 0) return false;
	float sqrtd = sqrt(discriminant);

	// Find the nearest root that lies in the acceptable range.
	float root = (-half_b - sqrtd) / a;
	if (root < t_min || t_max < root) {
		root = (-half_b + sqrtd) / a;
		if (root < t_min || t_max < root)
			return false;
	}

	rec.t = root;
	rec.p = r.at(rec.t);
	vec3 outward_normal = (rec.p - center) / radius;
	rec.set_face_normal(r, outward_normal);
	rec.mat_ptr = mat_ptr;

	return true;
}

struct hittable_list : public hittable
{
	hittable_list() {}
	hittable_list(shared_ptr<hittable> object) { add(object); }

	void clear() { objects.clear(); }
	void add(shared_ptr<hittable> object) { objects.push_back(object); }

	virtual bool hit(
	    const ray& r, float t_min, float t_max, hit_record& rec) const override;

	std::vector<shared_ptr<hittable>> objects;
};

bool hittable_list::hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
	hit_record temp_rec;
	auto hit_anything = false;
	auto closest_so_far = t_max;

	for (const auto& object : objects) {
		if (object->hit(r, t_min, closest_so_far, temp_rec)) {
			hit_anything = true;
			closest_so_far = temp_rec.t;
			rec = temp_rec;
		}
	}

	return hit_anything;
}

struct camera
{
	vec3 origin;
	vec3 lower_left_corner;
	vec3 horizontal;
	vec3 vertical;
	vec3 u,v,w;
	float lens_radius;

	// vfov = vertical fov in degrees
	camera(
		vec3 eye,
		vec3 lookat,
		vec3 vup,
		float vfov,
		float aspect_ratio,
		float aperture,
		float focus_dist
	) {
		float theta = rsw::radians(vfov);
		float h = tan(theta/2);
		float viewport_height = 2.0f * h;
		float viewport_width = aspect_ratio * viewport_height;

		w = (eye - lookat).norm();
		u = cross(vup, w).norm();
		v = cross(w, u);

		//float focal_length = 1.0f;

		origin = eye;
		horizontal = focus_dist * viewport_width * u;
		vertical = focus_dist * viewport_height * v;
		lower_left_corner = origin - horizontal/2 - vertical/2 - focus_dist*w;

		lens_radius = aperture / 2;
	}

	ray get_ray(float s, float t) const {
		vec3 rd = lens_radius * rsw::random_in_unit_disk();
		vec3 offset = u * rd.x + v* rd.y;
		return ray(origin+offset, lower_left_corner + s*horizontal + t*vertical - origin - offset);
	}
};

struct material
{
	virtual bool scatter(
		const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered
		) const = 0;
};

struct lambertian : public material
{
	vec3 albedo;

	lambertian(const vec3& a) : albedo(a) {}

	virtual bool scatter(
		const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered
	) const override {
		vec3 scatter_direction = rec.normal + rsw::random_unit_vector();

		// catch degenerate scatter direction (unit vector was close to opposite normal)
		if (scatter_direction.near_zero())
			scatter_direction = rec.normal;

		scattered = ray(rec.p, scatter_direction);
		attenuation = albedo;
		return true;
	}

};

struct metal : public material
{
	vec3 albedo;
	float fuzz;

	metal(const vec3& a, float f) : albedo(a), fuzz(f < 1 ? f : 1) {}

	virtual bool scatter(
		const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered
	) const override {
		vec3 reflected = reflect(r_in.dir.norm(), rec.normal);
		scattered = ray(rec.p, reflected + fuzz*rsw::random_in_unit_sphere());
		attenuation = albedo;
		return (dot(scattered.dir, rec.normal) > 0);
	}

};

struct dielectric : public material
{
	double ir; // index of refraction

	dielectric(float index_of_refraction) : ir(index_of_refraction) {}

	virtual bool scatter(
		const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered
	) const override {
		attenuation = vec3(1);
		float refraction_ratio = rec.front_face ? (1.0f/ir) : ir;

		vec3 unit_dir = r_in.dir.norm();
		float cos_theta = fminf(dot(-unit_dir, rec.normal), 1.0);
		float sin_theta = sqrt(1.0f - cos_theta*cos_theta);

		bool cannot_refract = refraction_ratio * sin_theta > 1.0f;
		vec3 direction;
		if (cannot_refract || reflectance(cos_theta, refraction_ratio) > rsw::randf())
			direction = reflect(unit_dir, rec.normal);
		else
			direction = rsw::refract(unit_dir, rec.normal, refraction_ratio);

		scattered = ray(rec.p, direction);
		return true;
	}

	static float reflectance(float cosine, float ref_idx)
	{
		// Use Schlick's approimation for reflectance
		float r0 = (1-ref_idx) / (1+ref_idx);
		r0 *= r0;
		return r0 + (1-r0)*pow((1 - cosine), 5);
	}

};



