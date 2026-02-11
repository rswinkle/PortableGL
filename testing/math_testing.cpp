#include "rsw_math.h"

#define PGL_PREFIX_TYPES
#define PORTABLEGL_IMPLEMENTATION
#include <portablegl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#define WIDTH 640
#define HEIGHT 480

using namespace std;

inline int eql_epsilon(float a, float b, float eps)
{
	return fabsf(a-b) < eps;
}

#define EPSILON (1e-6)
inline int cmp_m4s(float* m1, float* m2)
{
	//assert(!memcmp(m1, m2, 16*sizeof(float)));
	//return memcmp(m1, m2, 16*sizeof(float));
	
	for (int i=0; i<16; i++) {
		if (!eql_epsilon(m1[i], m2[i], EPSILON)) {
			printf("%d:\n%.8f\n%.8f\n", i, m1[i], m2[i]);
			return 1;
		}
	}
	return 0;
}

inline void print_if_diff(float* m1, float* m2)
{
	if (cmp_m4s(m1, m2)) {
		print_m4(m1, "\n");
		print_m4(m2, "\n");
	}
}
inline void print_m4(float* m1)
{
	for (int i=0; i<16; i++) {

		
	}
}

int main()
{
	float fov = DEG_TO_RAD(45.0f);
	float aspect = WIDTH/float(HEIGHT);
	float near = 0.1f, far = 100.0f;

	pgl_mat4 pm4_pers_proj;
	make_perspective_m4(pm4_pers_proj, fov, aspect, near, far);
	glm::mat4 pers_proj = glm::perspective(fov, aspect, near, far);

	print_if_diff((float*)&pers_proj, pm4_pers_proj);

	// TODO change to match glm::ortho/glOrtho?
	//n and f really are near and far not min and max so if you want the standard looking down the -z axis
	// then n > f otherwise n < f
	pgl_mat4 pm4_ortho;
	make_orthographic_m4(pm4_ortho, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f);

	glm::mat4 glm_ortho2D = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);
	glm::mat4 glm_ortho = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

	puts("Testing ortho2D");
	print_if_diff((float*)&glm_ortho2D, pm4_ortho);
	puts("Testing ortho");
	print_if_diff((float*)&glm_ortho, pm4_ortho);


	pgl_mat4 pers_mat, ortho_for_pers, result_mat;
	make_pers_m4(pers_mat, near, far);

	// have to calculate the bounds of the near plane using near and fov
	float t = near * tanf(fov * 0.5f);
	float b = -t;
	float l = b * aspect;
	float r = -l;
	// note for this it's actual near and far ie looking down the -z, -near > -far
	make_orthographic_m4(ortho_for_pers, l, r, b, t, -near, -far);

	print_m4(pers_mat, "\n");
	print_m4(ortho_for_pers, "\n");

	mult_m4_m4(result_mat, ortho_for_pers, pers_mat);

	puts("Testing P*O == Persective projection");
	print_if_diff(result_mat, pm4_pers_proj);



	



}


