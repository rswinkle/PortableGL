
extern inline vec4 make_vec4(float x, float y, float z, float w);
extern inline vec4 negate_vec4(vec4 v);
extern inline void fprint_vec4(FILE* f, vec4 v, const char* append);
extern inline void print_vec4(vec4 v, const char* append);
extern inline int fread_vec4(FILE* f, vec4* v);
extern inline float length_vec4(vec4 a);
extern inline vec4 norm_vec4(vec4 a);
extern inline void normalize_vec4(vec4* a);
extern inline vec4 add_vec4s(vec4 a, vec4 b);
extern inline vec4 sub_vec4s(vec4 a, vec4 b);
extern inline vec4 mult_vec4s(vec4 a, vec4 b);
extern inline vec4 div_vec4s(vec4 a, vec4 b);
extern inline float dot_vec4s(vec4 a, vec4 b);
extern inline vec4 scale_vec4(vec4 a, float s);
extern inline int equal_vec4s(vec4 a, vec4 b);
extern inline int equal_epsilon_vec4s(vec4 a, vec4 b, float epsilon);


