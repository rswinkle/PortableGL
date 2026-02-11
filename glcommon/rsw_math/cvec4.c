
extern inline vec4 make_v4(float x, float y, float z, float w);
extern inline vec4 neg_v4(vec4 v);
extern inline void fprint_v4(FILE* f, vec4 v, const char* append);
extern inline void print_v4(vec4 v, const char* append);
extern inline int fread_v4(FILE* f, vec4* v);
extern inline float len_v4(vec4 a);
extern inline vec4 norm_v4(vec4 a);
extern inline void normalize_v4(vec4* a);
extern inline vec4 add_v4s(vec4 a, vec4 b);
extern inline vec4 sub_v4s(vec4 a, vec4 b);
extern inline vec4 mult_v4s(vec4 a, vec4 b);
extern inline vec4 div_v4s(vec4 a, vec4 b);
extern inline float dot_v4s(vec4 a, vec4 b);
extern inline vec4 scale_v4(vec4 a, float s);
extern inline int equal_v4s(vec4 a, vec4 b);
extern inline int equal_epsilon_v4s(vec4 a, vec4 b, float epsilon);

