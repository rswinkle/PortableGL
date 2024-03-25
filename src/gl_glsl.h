


/*************************************
 *  GLSL(ish) functions
 *************************************/

// Some duplication with crsw_math.h because
// we use these internally and the user can exclude
// those functions (with the official glsl names) to
// avoid clashes
//float clampf_01(float f);
//float clampf(float f, float min, float max);
//int clampi(int i, int min, int max);

//shader texture functions
vec4 texture1D(GLuint tex, float x);
vec4 texture2D(GLuint tex, float x, float y);
vec4 texture3D(GLuint tex, float x, float y, float z);
vec4 texture2DArray(GLuint tex, float x, float y, int z);
vec4 texture_rect(GLuint tex, float x, float y);
vec4 texture_cubemap(GLuint texture, float x, float y, float z);




