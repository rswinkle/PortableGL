
#include "rsw_math.h"

#define PGL_MANGLE_TYPES
#define EXCLUDE_GLSL
#define PORTABLEGL_IMPLEMENTATION
#include "gltools.h"
#include "GLObjects.h"


#include <iostream>
#include <stdio.h>


#define SDL_MAIN_HANDLED
#include <SDL.h>

#define WIDTH 320
#define HEIGHT 240

using namespace std;

using rsw::vec4;
using rsw::vec3;
using rsw::vec2;
using rsw::mat2;
using rsw::mat3;
using rsw::mat4;
using rsw::mix;
using rsw::radians;
using rsw::dot;
using rsw::clamp;
using rsw::smoothstep;

vec4 Red(1.0f, 0.0f, 0.0f, 0.0f);
vec4 Green(0.0f, 1.0f, 0.0f, 0.0f);
vec4 Blue(0.0f, 0.0f, 1.0f, 0.0f);

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;

u32* bbufpix;

glContext the_Context;

float iGlobalTime;

typedef struct My_Uniforms
{
	mat4 mvp_mat;
	vec4 v_color;
	float globaltime;
	GLuint tex0;
	GLuint tex1;
	GLuint tex2;
	GLuint tex6;
	GLuint tex9;
} My_Uniforms;

#define NUM_TEXTURES 5
GLuint textures[NUM_TEXTURES];


#define NUM_SHADERS 11
GLuint shaders[NUM_SHADERS];

void cleanup();
void setup_context();
void setup_gl_data();


void normal_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);


void graphing_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void graphing_lines_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void my_tunnel_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void square_tunnel_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void deform_tunnel_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void the_cave_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void tileable_water_caustic_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void flame_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void running_in_the_night_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void voronoise_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void iqs_eyeball(float* fs_input, Shader_Builtins* builtins, void* uniforms);

frag_func frag_funcs[NUM_SHADERS] =
{
	graphing_lines_fs,
	graphing_fs,
	my_tunnel_fs,
	running_in_the_night_fs,
	square_tunnel_fs,
	deform_tunnel_fs,
	tileable_water_caustic_fs,
	iqs_eyeball,
	voronoise_fs,
	the_cave_fs,
	flame_fs
};

int main(int argc, char** argv)
{

	setup_context();

	//can't turn off C++ destructors
	{

	float points[] = { -1.0,  1.0, 0,
	                   -1.0, -1.0, 0,
	                    1.0,  1.0, 0,
	                    1.0, -1.0, 0
	};


	My_Uniforms the_uniforms;
	mat4 identity;

	glGenTextures(NUM_TEXTURES, textures);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	if (!load_texture2D("../media/textures/tex00.jpg", GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_FALSE, GL_FALSE)) {
		printf("failed to load texture\n");
		return 0;
	}
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	if (!load_texture2D("../media/textures/tex02.jpg", GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_FALSE, GL_FALSE)) {
		printf("failed to load texture\n");
		return 0;
	}
	glBindTexture(GL_TEXTURE_2D, textures[2]);
	if (!load_texture2D("../media/textures/tex06.jpg", GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_FALSE, GL_FALSE)) {
		printf("failed to load texture\n");
		return 0;
	}
	glBindTexture(GL_TEXTURE_2D, textures[3]);
	if (!load_texture2D("../media/textures/tex01.jpg", GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_FALSE, GL_FALSE)) {
		printf("failed to load texture\n");
		return 0;
	}
	glBindTexture(GL_TEXTURE_2D, textures[4]);
	if (!load_texture2D("../media/textures/tex09.jpg", GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_FALSE, GL_FALSE)) {
		printf("failed to load texture\n");
		return 0;
	}

	the_uniforms.tex0 = textures[0];
	the_uniforms.tex2 = textures[1];
	the_uniforms.tex6 = textures[2];
	the_uniforms.tex1 = textures[3];
	the_uniforms.tex9 = textures[4];

	Buffer triangle(1);
	triangle.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*12, points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	for (int i=0; i<NUM_SHADERS; ++i) {
		shaders[i] = pglCreateProgram(normal_vs, frag_funcs[i], 0, NULL, GL_FALSE);
		glUseProgram(shaders[i]);

		pglSetUniform(&the_uniforms);
	}

	int cur_shader = 0;
	glUseProgram(shaders[cur_shader]);

	the_uniforms.v_color = Red;
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
				//printf("%c %c\n", event.key.keysym.scancode, event.key.keysym.sym);
				//printf("Physical %s key acting as %s key",
      	  	 	 // 	  SDL_GetScancodeName(keysym.scancode),
      	  	  	  //	  SDL_GetKeyName(keysym.sym));

				switch (keysym.sym) {
				case SDLK_ESCAPE:
					quit = true;
					break;
				case SDLK_LEFT:
					cur_shader = (cur_shader) ? cur_shader-1 : NUM_SHADERS-1;
					start_time = SDL_GetTicks();
					glUseProgram(shaders[cur_shader]);
					break;
				case SDLK_RIGHT:
					cur_shader = (cur_shader + 1) % NUM_SHADERS;
					start_time = SDL_GetTicks();
					glUseProgram(shaders[cur_shader]);
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
	}


	}

	cleanup();

	return 0;
}


void normal_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_Position = vertex_attribs[0];
}

void graphing_lines_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	//*(vec4*)fragcolor = ((My_Uniforms*)uniforms)->v_color;

	vec2 frag = *(vec2*)(&builtins->gl_FragCoord); //only want xy;
	frag.x /= WIDTH;
	frag.y /= HEIGHT;
	frag *= 2;
	frag -= 1;

	float fragx2 = frag.x * frag.x;
	float x2 = frag.y - fragx2;
	float x3 = frag.y - fragx2 * frag.x + 0.25*frag.x;
	float x4 = frag.y - fragx2 * fragx2;
	float incr = 2.0f / HEIGHT; //height per pixel

	if (x2 >= -incr && x2 <= incr) {
		*(vec4*)&builtins->gl_FragColor = vec4(1, 0, 0, 1);
	} else if (x3 >= -incr && x3 <= incr) {
		*(vec4*)&builtins->gl_FragColor = vec4(0, 1, 0, 1);
	} else if (x4 >= -incr && x4 <= incr) {
		*(vec4*)&builtins->gl_FragColor = vec4(0, 0, 1, 1);
	} else {
		*(vec4*)&builtins->gl_FragColor = vec4(0, 0, 0, 1);
	}
}

void graphing_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec2 frag = *(vec2*)(&builtins->gl_FragCoord); //only want xy;
	frag.x /= WIDTH;
	frag.y /= HEIGHT;
	frag *= 2;
	frag -= 1;

	float fragx2 = frag.x * frag.x;
	float x2 = frag.y - fragx2;
	//float x3 = frag.y - fragx2 * frag.x + 0.25*frag.x;
	//float x4 = frag.y - fragx2 * fragx2;

	if (x2 > 0)
		*(vec4*)&builtins->gl_FragColor = vec4(x2, 0, 0, 1);
	else
		*(vec4*)&builtins->gl_FragColor = vec4(0, 0, 0, 1);
}

void my_tunnel_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	//*(vec4*)fragcolor = ((My_Uniforms*)uniforms)->v_color;

	float globaltime = ((My_Uniforms*)uniforms)->globaltime;

	vec2 frag = *(vec2*)(&builtins->gl_FragCoord); //only want xy;
	frag.x /= WIDTH;
	frag.y /= HEIGHT;
	frag *= 2;
	frag -= 1;

	//float r = sqrt(frag.x * frag.x + frag.y * frag.y);
	float r = pow(pow(frag.x, 16.0) + pow(frag.y, 16.0), 1.0/16.0);
	//float sixpi = 4 * 3.1416;

	//float wave = (0.5*sin(sixpi*(1-r) + fmod(2*globaltime, sixpi)) + 0.5);
	float wave = (0.5*sin(10*globaltime*(1-r)) + 0.5);

	//*(vec4*)&builtins->gl_FragColor = r * (wave * vec4(1, 0, 0, 1) + (1-wave) * vec4(0, 0, 1, 1));
	*(vec4*)&builtins->gl_FragColor = vec4(r*wave, 0, r*(1-wave), 1);

	//*(vec4*)&builtins->gl_FragColor = vec4(r, 0, 0);
}

void square_tunnel_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
// ported from https://www.shadertoy.com/view/Ms2SWW
// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

	float globaltime = ((My_Uniforms*)uniforms)->globaltime;
	GLuint channel0 = ((My_Uniforms*)uniforms)->tex0;

	vec2 resolution(WIDTH, HEIGHT);
	vec2 frag = *(vec2*)(&builtins->gl_FragCoord); //only want xy;
	// normalized coordinates (-1 to 1 vertically)
	vec2 p = (-resolution + 2.0f*frag) / resolution.y;

	// angle of each pixel to the center of the screen
	float a = atan2(p.y, p.x);

	// modified distance metric. Usually distance = (x² + y²)^(1/2). By replacing all the "2" numbers
	// by 32 in that formula we can create distance metrics other than the euclidean. The higher the
	// exponent, then more square the metric becomes. More information here:

	// http://en.wikipedia.org/wiki/Minkowski_distance

	float r = pow(pow(p.x*p.x, 16.0f) + pow(p.y*p.y,16.0f), 1.0f/32.0f);

	// index texture by angle and radious, and animate along radius
	vec2 uv = vec2(0.5f/r + 0.5f*globaltime, a/3.1416f );

	// fetch color and darken in the center
	pgl_vec4 tmp = texture2D(channel0, uv.x, uv.y);
	*(vec4*)&builtins->gl_FragColor = vec4(tmp.x*r, tmp.y*r, tmp.z*r, 1.0);
}

void deform_tunnel_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
// ported from https://www.shadertoy.com/view/XdXGzn
// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

	float globaltime = ((My_Uniforms*)uniforms)->globaltime;
	GLuint channel0 = ((My_Uniforms*)uniforms)->tex2;
	GLuint channel1 = ((My_Uniforms*)uniforms)->tex0;

	vec2 resolution(WIDTH, HEIGHT);
	vec2 frag = *(vec2*)(&builtins->gl_FragCoord); //only want xy;

    vec2 p = -1.0 + 2.0 * frag / resolution;
    vec2 uv;

    float r = pow( pow(p.x*p.x,16.0) + pow(p.y*p.y,16.0), 1.0/32.0 );
    uv.x = .5*globaltime + 0.5/r;
    uv.y = atan2(p.y, p.x)/3.1416;

	float h = sin(32.0*uv.y);
    uv.x += 0.85*smoothstep(-0.1, 0.1, h);
    pgl_vec4 ch1_ = texture2D(channel1, 2.0*uv.x, 2.0*uv.y);
    pgl_vec4 ch0_ = texture2D(channel0, uv.x, uv.y);

    vec3 ch0(ch0_.x, ch0_.y, ch0_.z);
    vec3 ch1(ch1_.x, ch1_.y, ch1_.z);
	float a = smoothstep(0.9, 1.1, abs(p.x/p.y));
	vec3 col = mix(ch1, ch0, a);

    r *= 1.0 - 0.3*(smoothstep(0.0, 0.3, h) - smoothstep(0.3, 0.96, h));

    *(vec4*)&builtins->gl_FragColor = vec4(col*r*r*1.2, 1.0);
}

void tileable_water_caustic_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
//ported from https://www.shadertoy.com/view/MdlXz8
// Created by Dave_Hoskins
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

	// Found this on GLSL sandbox. I really liked it, changed a few things and made it tileable.
	// :)

	// -----------------------------------------------------------------------
	// Water turbulence effect by joltz0r 2013-07-04, improved 2013-07-07
	// Altered
	// -----------------------------------------------------------------------

#define TAU 6.28318530718
#define MAX_ITER 5

	//float iGlobalTime = ((My_Uniforms*)uniforms)->globaltime;

	vec2 iResolution(WIDTH, HEIGHT);
	vec2 gl_FragCoord = *(vec2*)(&builtins->gl_FragCoord); //only want xy;

	float time = iGlobalTime * .5+23.0;
	vec2 sp = gl_FragCoord / iResolution;

	vec2 p = sp*TAU-250.0;
	vec2 i = vec2(p);
	float c = 1.0;
	float inten = .005;

	for (int n = 0; n < MAX_ITER; n++)
	{
		float t = time * (1.0 - (3.5 / float(n+1)));
		i = p + vec2(cos(t - i.x) + sin(t + i.y), sin(t - i.y) + cos(t + i.x));
		c += 1.0/length(vec2(p.x / (sin(i.x+t)/inten),p.y / (cos(i.y+t)/inten)));
	}
	c /= float(MAX_ITER);
	c = 1.2-pow(c, 1.2);
	vec3 colour = vec3(pow(abs(c), 6.0));

	//gl_FragColor = vec4(clamp(colour + vec3(0.0, 0.35, 0.5), 0.0, 1.0), 1.0);
    *(vec4*)&builtins->gl_FragColor = vec4(clamp(colour + vec3(0.0, 0.35, 0.5), 0.0, 1.0), 1.0);
}

void running_in_the_night_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
//ported from https://www.shadertoy.com/view/ldl3DN
// Created by LaPatate64
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

	vec2 iResolution(WIDTH, HEIGHT);
	vec2 gl_FragCoord = *(vec2*)(&builtins->gl_FragCoord); //only want xy;
	GLuint iChannel0 = ((My_Uniforms*)uniforms)->tex2;

	float time = radians(45.0) + cos(iGlobalTime * 12.0) / 40.0;
	float time1 = radians(45.0) + cos(iGlobalTime * 22.0) / 30.0;
	vec2 uv = gl_FragCoord / iResolution * 2.0 - vec2(1.0, 1.0);
	if (uv.y < 0.0){
		vec2 tex;
		vec2 rot = vec2(cos(time), sin(time));
		vec2 mat;
		mat.x = (uv.x * rot.x + (uv.y - 1.0) * rot.y);
		mat.y = ((uv.y - 1.0) * rot.x - uv.x * rot.y);
		tex.x = mat.x * time1 / uv.y + iGlobalTime * 2.0;
		tex.y = mat.y * time1 / uv.y + iGlobalTime * 2.0;

		pgl_vec4 tmp = texture2D(iChannel0, tex.x * 2.0, tex.y * 2.0);
		*(vec4*)&builtins->gl_FragColor = vec4(tmp.x, tmp.y, tmp.z, tmp.w) * (-uv.y);
	} else {
		*(vec4*)&builtins->gl_FragColor = vec4(0);
	}
}


// ported from https://www.shadertoy.com/view/Xd23Dh
// Created by inigo quilez - iq/2014
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// This is a procedural pattern that has 2 parameters, that generalizes cell-noise,
// perlin-noise and voronoi, all of which can be written in terms of the former as:
//
// cellnoise(x) = pattern(0,0,x)
// perlin(x) = pattern(0,1,x)
// voronoi(x) = pattern(1,0,x)
//
// From this generalization of the three famouse patterns, a new one (which I call
// "Voronoise") emerges naturally. It's like perlin noise a bit, but within a jittered
// grid like voronoi):
//
// voronoise(x) = pattern(1,1,x)
//
// Not sure what one would use this generalization for, because it's slightly slower
// than perlin or voronoise (and certainly much slower than cell noise), and in the
// end as a shading TD you just want one or another depending of the type of visual
// features you are looking for, I can't see a blending being needed in real life.
// But well, if only for the math fun it was worth trying. And they say a bit of
// mathturbation can be healthy anyway!

// Use the mouse to blend between different patterns:

// ell noise    u=0,v=0
// voronoi      u=1,v=0
// perlin noise u=0,v1=
// voronoise    u=1,v=1

// More info here: http://iquilezles.org/www/articles/voronoise/voronoise.htm

vec3 hash3(vec2 p)
{
    vec3 q = vec3( dot(p,vec2(127.1,311.7)),
				   dot(p,vec2(269.5,183.3)),
				   dot(p,vec2(419.2,371.9)) );
	return fract(sin(q)*43758.5453);
}

float iqnoise(vec2 x, float u, float v)
{
    vec2 p = floor(x);
    vec2 f = fract(x);

	float k = 1.0+63.0*pow(1.0-v,4.0);

	float va = 0.0;
	float wt = 0.0;
    for( int j=-2; j<=2; j++ )
    for( int i=-2; i<=2; i++ )
    {
        vec2 g = vec2( float(i),float(j) );
		vec3 o = hash3( p + g )*vec3(u,u,1.0);
		vec2 r = g - f + o.xy();
		float d = dot(r,r);
		float ww = pow(1.0 - smoothstep(0.0,1.414,sqrt(d)), k);
		va += o.z*ww;
		wt += ww;
    }

    return va/wt;
}

void voronoise_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec2 iResolution(WIDTH, HEIGHT);
	vec2 gl_FragCoord = *(vec2*)(&builtins->gl_FragCoord); //only want xy;

	vec2 uv = gl_FragCoord / iResolution.xx();

    vec2 p = 0.5 - 0.5*sin( iGlobalTime*vec2(1.01,1.71) );

	//if (iMouse.w > 0.001) p = vec2(0.0, 1.0) + vec2(1.0, -1.0)*iMouse.xy/iResolution;

	p = p*p*(3.0-2.0*p);
	p = p*p*(3.0-2.0*p);
	p = p*p*(3.0-2.0*p);

	float f = iqnoise(24.0*uv, p.x, p.y);

	//gl_FragColor = vec4( f, f, f, 1.0 );
	*(vec4*)&builtins->gl_FragColor = vec4( f, f, f, 1.0 );
}




//ported from https://www.shadertoy.com/view/MdX3zr
// Created by XT95
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

float noise(vec3 p) //Thx to Las^Mercury
{
	vec3 i = floor(p);
	vec4 a = dot(i, vec3(1., 57., 21.)) + vec4(0., 57., 21., 78.);
	vec3 f = cos((p-i)*acos(-1.))*(-.5)+.5;
	a = mix(sin(cos(a)*a),sin(cos(1.+a)*(1.+a)), f.x);
	//a.xy = mix(a.xz(), a.yw(), f.y);
	a.xy(mix(a.xz(), a.yw(), f.y));
	return mix(a.x, a.y, f.z);
}

float sphere(vec3 p, vec4 spr)
{
	return length(spr.xyz()-p) - spr.w;
}

float flame(vec3 p)
{
	float d = sphere(p*vec3(1.,.5,1.), vec4(.0,-1.,.0,1.));
	return d + (noise(p+vec3(.0,iGlobalTime*2.,.0)) + noise(p*3.)*.5)*.25*(p.y) ;
}

float scene(vec3 p)
{
	return min(100.0f-length(p) , abs(flame(p)));
}

vec4 raymarch(vec3 org, vec3 dir)
{
	float d = 0.0, glow = 0.0, eps = 0.02;
	vec3  p = org;
	bool glowed = GL_FALSE;

	for(int i=0; i<64; i++)
	{
		d = scene(p) + eps;
		p += d * dir;
		if( d>eps )
		{
			if(flame(p) < .0)
				glowed=true;
			if(glowed)
       			glow = float(i)/64.;
		}
	}
	return vec4(p,glow);
}


void flame_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec2 iResolution(WIDTH, HEIGHT);
	vec2 gl_FragCoord = *(vec2*)(&builtins->gl_FragCoord); //only want xy;

	vec2 v = -1.0 + 2.0 * gl_FragCoord / iResolution;
	v.x *= iResolution.x/iResolution.y;

	vec3 org = vec3(0., -2., 4.);
	vec3 dir = normalize(vec3(v.x*1.6, -v.y, -1.5));

	vec4 p = raymarch(org, dir);
	float glow = p.w;

	vec4 col = mix(vec4(1.,.5,.1,1.), vec4(0.1,.5,1.,1.), p.y*.02+.4);

	//gl_FragColor = mix(vec4(1.), mix(vec4(1.,.5,.1,1.),vec4(0.1,.5,1.,1.),p.y*.02+.4), pow(glow*2.,4.));

	//gl_FragColor = mix(vec4(0.), col, pow(glow*2.,4.));
    *(vec4*)&builtins->gl_FragColor = mix(vec4(0.), col, pow(glow*2.,4.));
}




// ported from https://www.shadertoy.com/view/MsX3RH
// Created by BoyC
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// constants for the camera tunnel
const vec2 cama=vec2(-2.6943,3.0483);
const vec2 camb=vec2(0.2516,0.1749);
const vec2 camc=vec2(-3.7902,2.4478);
const vec2 camd=vec2(0.0865,-0.1664);

const vec2 lighta=vec2(1.4301,4.0985);
const vec2 lightb=vec2(-0.1276,0.2347);
const vec2 lightc=vec2(-2.2655,1.5066);
const vec2 lightd=vec2(-0.1284,0.0731);

// calculates the position of a single tunnel
inline vec2 Position(float z, vec2 a, vec2 b, vec2 c, vec2 d)
{
	return sin(z*a)*b + cos(z*c)*d;
}

// calculates 3D positon of a tunnel for a given time
inline vec3 Position3D(float time, vec2 a, vec2 b, vec2 c, vec2 d)
{
	return vec3(Position(time,a,b,c,d),time);
}

// 2d distance field for a slice of a single tunnel
inline float Distance(vec3 p, vec2 a, vec2 b, vec2 c, vec2 d, vec2 e, float r)
{
	vec2 pos = Position(p.z,a,b,c,d);
	float radius = max(5.0f, r + sin(p.z*e.x)*e.y) / 10000.0;
	return radius/dot(p.xy()-pos, p.xy()-pos);
}

// 2d distance field for a slice of the tunnel network
float Dist2D(vec3 pos)
{
	float d=0.0;

	d+=Distance(pos,cama,camb,camc,camd,vec2(2.1913,15.4634),70.0000);
	d+=Distance(pos,lighta,lightb,lightc,lightd,vec2(0.3814,12.7206),17.0590);
	d+=Distance(pos,vec2(2.7377,-1.2462),vec2(-0.1914,-0.2339),vec2(-1.3698,-0.6855),vec2(0.1049,-0.1347),vec2(-1.1157,13.6200),27.3718);
	d+=Distance(pos,vec2(-2.3815,0.2382),vec2(-0.1528,-0.1475),vec2(0.9996,-2.1459),vec2(-0.0566,-0.0854),vec2(0.3287,12.1713),21.8130);
	d+=Distance(pos,vec2(-2.7424,4.8901),vec2(-0.1257,0.2561),vec2(-0.4138,2.6706),vec2(-0.1355,0.1648),vec2(2.8162,14.8847),32.2235);
	d+=Distance(pos,vec2(-2.2158,4.5260),vec2(0.2834,0.2319),vec2(4.2578,-2.5997),vec2(-0.0391,-0.2070),vec2(2.2086,13.0546),30.9920);
	d+=Distance(pos,vec2(0.9824,4.4131),vec2(0.2281,-0.2955),vec2(-0.6033,0.4780),vec2(-0.1544,0.1360),vec2(3.2020,12.2138),29.1169);
	d+=Distance(pos,vec2(1.2733,-2.4752),vec2(-0.2821,-0.1180),vec2(3.4862,-0.7046),vec2(0.0224,0.2024),vec2(-2.2714,9.7317),6.3008);
	d+=Distance(pos,vec2(2.6860,2.3608),vec2(-0.1486,0.2376),vec2(2.0568,1.5440),vec2(0.0367,0.1594),vec2(-2.0396,10.2225),25.5348);
	d+=Distance(pos,vec2(0.5009,0.9612),vec2(0.1818,-0.1669),vec2(0.0698,-2.0880),vec2(0.1424,0.1063),vec2(1.7980,11.2733),35.7880);

	return d;
}

inline vec3 nmap(vec2 t, GLuint tx, float str)
{
	float d=1.0/1024.0;

	float xy = texture2D(tx, t.x, t.y).x;
	float x2 = texture2D(tx, t.x + d, t.y).x;
	float y2 = texture2D(tx, t.x, t.y + d).x;

	float s=(1.0-str)*1.2;
	s*=s;
	s*=s;

	return normalize(vec3(x2-xy, y2-xy, s/8.0));///2.0+0.5;
}


void the_cave_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	float globaltime = ((My_Uniforms*)uniforms)->globaltime;
	GLuint iChannel0 = ((My_Uniforms*)uniforms)->tex6;
	GLuint iChannel1 = ((My_Uniforms*)uniforms)->tex1;
	GLuint iChannel2 = ((My_Uniforms*)uniforms)->tex9;

	vec2 iResolution(WIDTH, HEIGHT);
	vec2 gl_FragCoord = *(vec2*)(&builtins->gl_FragCoord); //only want xy;

	// original used /3.0 but FPS is so slow it looks better moving slower
	float time = globaltime/12.0+291.0; //+43.63/3.0;

	//calculate camera by looking ahead in the tunnel

	vec2 p1=Position(time+0.05,cama,camb,camc,camd); //position ahead
	vec3 Pos=Position3D(time,cama,camb,camc,camd); //current position
	vec3 oPos=Pos;

	vec3 CamDir = normalize(vec3(p1.x-Pos.x,-p1.y+Pos.y,0.1));
	vec3 CamRight = normalize(rsw::cross(CamDir,vec3(0,1,0)));
	vec3 CamUp = normalize(rsw::cross(CamRight,CamDir));
	mat3 cam = mat3(CamRight,CamUp,CamDir);

	//ray calculation
	vec2 uv = 2.0*gl_FragCoord/iResolution-1.0;
	float aspect = iResolution.x/iResolution.y;

	vec3 Dir = normalize(vec3(uv*vec2(aspect,1.0),1.0)) * cam;

	//raymarching
	float fade=0.0;

	const float numit=75.0; //raymarch precision
	const float threshold=1.20; //defines the thickness of tunnels
	const float scale=1.5; //tunnel z depth

	vec3 Posm1=Pos;

	//calculate first hit
	for (float x=0.0; x<numit; x++)
	{
		if (Dist2D(Pos)<threshold)
		{
			fade=1.0-x/numit;
			break;
		}
		Posm1=Pos;
		Pos+=Dir/numit*scale;//*(1.0+x/numit);
	}

	//track back to get better resolution
	for (int x=0; x<6; x++)
	{
		vec3 p2=(Posm1+Pos)/2.0;
		if (Dist2D(p2)<threshold)
			Pos=p2;
		else
			Posm1=p2;
	}

	//lighting
	vec3 n=normalize(vec3(Dist2D(Pos+vec3(0.01,0,0))-Dist2D(Pos+vec3(-0.01,0,0)),
						  Dist2D(Pos+vec3(0,0.01,0))-Dist2D(Pos+vec3(0,-0.01,0)),
						  Dist2D(Pos+vec3(0,0,0.01))-Dist2D(Pos+vec3(0,0,-0.01))));

	//triplanar blend vector
	vec3 tpn = normalize(max(vec3(0.0), (abs(n)-vec3(0.2))*7.0))*0.5;

	//position of the light - uncomment the second line to get a more interesting path
	vec3 lp = Position3D(time+0.5,cama,camb,camc,camd); //current light position
	//lp = Position3D(time+0.3,lighta,lightb,lightc,lightd);

	vec3 ld = lp-Pos;	//light direction
	float lv=1.0;

	const float ShadowIT=15.0; //shadow precision

	//shadow calc
	for (float x=1.0; x<ShadowIT; x++) {
		if (Dist2D(Pos+ld*(x/ShadowIT))<threshold) {
			lv=0.0;
			break;
		}
	}

	vec3 tuv=Pos*vec3(3.0,3.0,1.5);	//texture coordinates

	/* normal mapping */
	float nms=0.19;
	vec3 nmx = nmap(tuv.yz(), iChannel0, nms) + nmap(-tuv.yz(), iChannel0, nms);
	vec3 nmy = nmap(tuv.xz(), iChannel1, nms) + nmap(-tuv.xz(), iChannel1, nms);
	vec3 nmz = nmap(tuv.xy(), iChannel2, nms) + nmap(-tuv.xy(), iChannel2, nms);

	vec3 nn=normalize(nmx*tpn.x+nmy*tpn.y+nmz*tpn.z);


	float dd;
	//normalmapped version:
	dd=max(0.0f ,dot(nn,normalize(ld*mat3(vec3(1,0,0),vec3(0,0,1),n))));
	//standard version:
	//dd=max(0.0f ,dot(n,normalize(ld)));

	vec4 diff=vec4(dd*1.2*lv)+vec4(0.2);

	//wisp
	float w=pow(dot(normalize(Pos-oPos),normalize(lp-oPos)),5000.0);
	//if (length(Pos-oPos) < length(lp-oPos)) w=0.0;
	if ((Pos-oPos).len() < (lp-oPos).len()) w=0.0;

	//texturing
	//double sampling to fix seams on texture edges
	pgl_vec4 tx_ = texture2D(iChannel0, tuv.y, tuv.z);
	pgl_vec4 ty_ = texture2D(iChannel1, tuv.x, tuv.z);
	pgl_vec4 tz_ = texture2D(iChannel2, tuv.x, tuv.y);
	vec4 tx(tx_.x, tx_.y, tx_.z, tx_.w);
	vec4 ty(ty_.x, ty_.y, ty_.z, ty_.w);
	vec4 tz(tz_.x, tz_.y, tz_.z, tz_.w);

	tx_ = texture2D(iChannel0, -tuv.y, -tuv.z);
	ty_ = texture2D(iChannel1, -tuv.x, -tuv.z);
	tz_ = texture2D(iChannel2, -tuv.x, -tuv.y);

	tx += vec4(tx_.x, tx_.y, tx_.z, tx_.w);
    ty += vec4(ty_.x, ty_.y, ty_.z, ty_.w);
    ty += vec4(tz_.x, tz_.y, tz_.z, tz_.w);

	vec4 col = tx*tpn.x+ty*tpn.y+tz*tpn.z;


	//gl_FragColor = col*diff*min(1.0,fade*10.0)+w;

    *(vec4*)&builtins->gl_FragColor = col*diff*min(1.0, fade*10.0) + w;
}


// iq's eyeball
// ported from
// https://www.shadertoy.com/view/XdyGz3 
//
// which in turn is taken from
// https://www.youtube.com/watch?v=emjuqqyq_qc
float eye_hash(float n)
{
	float x = sin(n)*43758.5453123;
	return x - floor(x);  // TODO use cast?
}

float eye_noise(vec2 x)
{
	vec2 p(floor(x.x), floor(x.y));
	vec2 f = x - p;

	// I think this is what that means
	//f = f * f * (3.0 - 2.0*f);
	f = f * f * (vec2(3.0) - 2.0*f);

	float n = p.x + p.y*57.0;

	return mix(mix(eye_hash(n+ 0.0), eye_hash(n+ 1.0), f.x), mix(eye_hash(n+ 57.0), eye_hash(n+ 58.0), f.x), f.y);
}

float fbm(vec2 p)
{
	mat2 m = mat2(0.8, 0.6, -0.6, 0.8);
	float f = 0.0f;
	f += 0.5000*eye_noise(p); p*=m*2.02;
	f += 0.2500*eye_noise(p); p*=m*2.03;
	f += 0.1250*eye_noise(p); p*=m*2.01;
	f += 0.0625*eye_noise(p); p*=m*2.04;
	f /= 0.9375;
	return f;
}


void iqs_eyeball(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	float time = ((My_Uniforms*)uniforms)->globaltime;

	vec2 uv = *(vec2*)(&builtins->gl_FragCoord) / vec2(WIDTH, HEIGHT);
	vec2 p = -1 + 2.0*uv;
	p.x *= WIDTH /(float)HEIGHT;

	float r = sqrt(dot(p, p));
	float a = atan2(p.y, p.x);

	// change this to whatever you want background color to be
	vec3 bg_col = vec3(1.0);

	vec3 col = bg_col;

	float ss = 0.5 + 0.5*sin(time);
	float anim = 1.0 + 0.1*ss*clamp(1.0-r, 0.0, 1.0);
	r *= anim;

	if (r < 0.8) {
		col = vec3(0.0, 0.3, 0.4);

		float f = fbm(5.0*p);
		col = mix(col, vec3(0.2, 0.5, 0.4), f);

		f = 1.0 - smoothstep(0.2, 0.5, r);
		col = mix(col, vec3(0.9, 0.6, 0.2), f);

		a += 0.05*fbm(20.0*p);

		f = smoothstep(0.3, 1.0, fbm(vec2(6.0*r, 20.0*a)));
		col = mix(col, vec3(1.0), f);

		f = smoothstep(0.4, 0.9, fbm(vec2(10.0*r, 15.0*a)));
		col *= 1.0 - 0.5*f;

		f = smoothstep(0.6, 0.8, r);
		col *= 1.0 - 0.5*f;

		f = smoothstep(0.2, 0.25, r);
		col *= f;

		f = 1.0 - smoothstep(0.0, 0.3, length(p - vec2(0.24, 0.2)));
		col += vec3(1.0, 0.9, 0.8)*f*0.8;

		f = smoothstep(0.75, 0.8, r);
		col = mix(col, bg_col, f);
	}

	*(vec4*)&builtins->gl_FragColor = vec4(col, 1.0);
}

void setup_context()
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO)) {
		cout << "SDL_Init error: " << SDL_GetError() << "\n";
		exit(0);
	}

	window = SDL_CreateWindow("Shadertoy", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (!window) {
		cerr << "Failed to create window\n";
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000)) {
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

