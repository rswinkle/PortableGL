#include "rsw_math.h"
#include "rtweekend.h"

#define PGL_PREFIX_TYPES
#define PORTABLEGL_IMPLEMENTATION
#include "gltools.h"
#include "GLObjects.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <iostream>
#include <memory>
#include <vector>

#include <stdio.h>


#define SDL_MAIN_HANDLED
#include <SDL.h>

#define WIDTH 320
#define HEIGHT 240

using namespace std;

using std::shared_ptr;
using std::make_shared;

using rsw::vec4;
using rsw::vec3;
using rsw::vec2;
using rsw::mat2;
using rsw::mat3;
using rsw::mat4;
using rsw::dot;
using rsw::randf;

vec4 Red(1.0f, 0.0f, 0.0f, 0.0f);
vec4 Green(0.0f, 1.0f, 0.0f, 0.0f);
vec4 Blue(0.0f, 0.0f, 1.0f, 0.0f);

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;

u32* bbufpix;

glContext the_Context;

float iGlobalTime;

// Type aliases for vec3
using point3f = vec3;   // 3D point
using color3f = vec3;    // RGB color3f


// Image

float aspect_ratio = WIDTH/(float)HEIGHT;

// World

hittable_list world;

// Camera
vec3 eye(13,2,3);
vec3 lookat(0);
vec3 vup(0,1,0);
float vfov = 20.0f;
float dist_to_focus = 10.0f;
float aperture = 0.1f;
camera cam(eye, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus);

// raytracing constants
#define USE_AA

int samples_per_pixel = 25;
int max_depth = 12;

typedef struct My_Uniforms
{
	mat4 mvp_mat;
	vec4 v_color3f;
	float globaltime;
	GLuint tex0;
	GLuint tex1;
	GLuint tex2;
	GLuint tex6;
	GLuint tex9;
} My_Uniforms;

#define NUM_TEXTURES 5
GLuint textures[NUM_TEXTURES];


void cleanup();
void setup_context();
void setup_gl_data();


void normal_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void raytracer_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);


void random_scene();


int main(int argc, char** argv)
{
	if (argc != 3) {
		printf("Usage: %s [samples=25] [max_depth=12]\n", argv[0]);
	}
	if (argc >= 2) {
		if (!(samples_per_pixel = atoi(argv[1]))) {
			return 0;
		}
	}

	if (argc >= 3) {
		if (!(max_depth = atoi(argv[2]))) {
			return 0;
		}
	}

	setup_context();

	My_Uniforms the_uniforms;
	mat4 identity;

	//can't turn off C++ destructors
	{

	/*
	float points[] = { -1.0,  1.0, 0,
	                   -1.0, -1.0, 0,
	                    1.0,  1.0, 0,
	                    1.0, -1.0, 0
	};

	Buffer triangle(1);
	triangle.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*12, points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	*/

		/*
	auto material_ground = make_shared<lambertian>(vec3(0.8,0.8,0.0));

	auto material_center = make_shared<lambertian>(vec3(0.1,0.2,0.5));
	//auto material_left   = make_shared<metal>(vec3(0.8, 0.8, 0.8), 0.3);
	auto material_left   = make_shared<dielectric>(1.5);

	auto material_right  = make_shared<metal>(vec3(0.8, 0.6, 0.2), 0.0);



	world.add(make_shared<sphere>(vec3(0,-100.5,-1), 100, material_ground));
	world.add(make_shared<sphere>(vec3(0,0,-1), 0.5, material_center));
	world.add(make_shared<sphere>(vec3(-1,0,-1), 0.5, material_left));
	world.add(make_shared<sphere>(vec3(-1,0,-1), -0.45, material_left));
	world.add(make_shared<sphere>(vec3(1,0,-1), 0.5, material_right));
	*/

	random_scene();

	GLuint shader = pglCreateProgram(normal_vs, raytracer_fs, 0, NULL, GL_FALSE);
	glUseProgram(shader);
	pglSetUniform(&the_uniforms);

	the_uniforms.v_color3f = Red;
	the_uniforms.mvp_mat = identity; //only necessary in C of course but that's what I want, to have it work as both

	SDL_Event event;
	SDL_Keysym keysym;
	bool quit = GL_FALSE;

	unsigned int old_time = 0, new_time=0, counter = 0, start_time = 0;

	while (!quit) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				quit = true;
			} else if (event.type == SDL_KEYDOWN) {
				keysym = event.key.keysym;

				switch (keysym.sym) {
				case SDLK_ESCAPE:
					quit = true;
					break;
				}
			}
		}

		++counter;
		new_time = SDL_GetTicks();
		if (new_time - old_time >= 3000) {
			printf("%f FPS\n", counter*1000.0f/((float)(new_time-old_time)));
			old_time = new_time;
			counter = 0;
		}

		iGlobalTime = (new_time-start_time) / 1000.0f;
		the_uniforms.globaltime = (new_time-start_time) / 1000.0f;

		//glClearColor(0, 0, 0, 1);
		//glClear(GL_COLOR_BUFFER_BIT);
		//glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		pglDrawFrame();

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		//Render the scene
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);

		//break;

	}

	/*
	char filename[] = "raytracing.png";
	if(!stbi_write_png(filename, WIDTH, HEIGHT, 4, bbufpix, WIDTH*4)) {
		printf("Failed to write %s\n", filename);
	}
	*/



	}

	cleanup();

	return 0;
}

void normal_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_Position = vertex_attribs[0];
}



// not sure section 6.2 simplification is actually faster
float hit_sphere(const vec3& center, float radius, const ray& r)
{
	vec3 oc = r.origin - center;
	float a = r.dir.len_squared();
	float half_b = dot(oc, r.dir);
	float c = oc.len_squared() - radius*radius;
	float discriminant = half_b*half_b - a*c;
	if (discriminant < 0) {
		return -1.0f;
	} else {
		return (-half_b - sqrt(discriminant)) / a;
	}
}

vec3 ray_color3f(const ray& r, const hittable& world, int depth)
{
	hit_record rec;

	if (depth <= 0)
		return vec3(0);

	if (world.hit(r, 0.001, infinity, rec)) {
		ray scattered;
		vec3 attenuation;
		if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
			return attenuation * ray_color3f(scattered, world, --depth);
		return vec3(0);
		/*
		// hack
		//vec3 target = rec.p + rec.normal + rsw::random_in_unit_sphere();

		// true Lambertian brighter, less pronounced shadows
		vec3 target = rec.p + rec.normal + rsw::random_unit_vector();

		// same hemisphere as normal method
		//vec3 target = rec.p + rsw::random_in_hemisphere(rec.normal);

		return 0.5 * ray_color3f(ray(rec.p, target-rec.p), world, --depth);
		*/

		// no bouncing
		//return 0.5 * (rec.normal + vec3(1));
	}
	vec3 unit_dir = r.dir.norm();
	float t = 0.5*(unit_dir.y + 1.0);
	return (1.0-t)*vec3(1.0) + t*vec3(0.5, 0.7, 1.0);
}

void raytracer_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec2 frag = *(vec2*)(&builtins->gl_FragCoord); //only want xy;

	// for single ray centered in the pixel
#ifndef USE_AA
	frag.x /= WIDTH;
	frag.y /= HEIGHT;
	// u = frag.x, v = frag.y
	
	vec3 color3f = sqrt(ray_color3f(cam.get_ray(frag.x, frag.y), world, MAX_DEPTH));
	*(vec4*)&builtins->gl_FragColor = vec4(color3f, 1);
#else
	// so we can add [0,1.0) and stay within our pixel.  single ray
	frag -= 0.5f;
	
	float u,v;
	vec3 color3f;
	for (int s=0; s<samples_per_pixel; s++) {
		u = (frag.x + randf()) / WIDTH;
		v = (frag.y + randf()) / HEIGHT;
		ray r = cam.get_ray(u, v);
		color3f += ray_color3f(r, world, max_depth);
	}
	float scale = 1.0f/samples_per_pixel;
	//color3f *= scale;
	
	// gamma corrections, gamma = 2.0
	color3f = rsw::sqrt(color3f*scale);
	
	*(vec4*)&builtins->gl_FragColor = vec4(color3f, 1);
#endif
}


void random_scene()
{
    auto ground_material = make_shared<lambertian>(color3f(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3f(0,-1000,0), 1000, ground_material));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = randf();
            point3f center(a + 0.9*randf(), 0.2, b + 0.9*randf());

            if ((center - point3f(4, 0.2, 0)).len() > 0.9) {
                shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    auto albedo = color3f::random() * color3f::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    // metal
                    auto albedo = color3f::random(0.5, 1);
                    auto fuzz = rsw::randf_range(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = make_shared<dielectric>(1.5);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3f(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(color3f(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3f(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<metal>(color3f(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3f(4, 1, 0), 1.0, material3));
}



void setup_context()
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO)) {
		cout << "SDL_Init error: " << SDL_GetError() << "\n";
		exit(0);
	}

	window = SDL_CreateWindow("raytracing_1weekend", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (!window) {
		cerr << "Failed to create window\n";
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT)) {
		puts("Failed to initialize glContext");
		exit(0);
	}
}

void cleanup()
{
	free_glContext(&the_Context);

	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(window);

	SDL_Quit();
}


