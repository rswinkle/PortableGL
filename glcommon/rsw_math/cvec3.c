
extern inline vec3 make_v3(float x, float y, float z);
extern inline vec3 neg_v3(vec3 v);
extern inline void fprint_v3(FILE* f, vec3 v, const char* append);
extern inline void print_v3(vec3 v, const char* append);
extern inline int fread_v3(FILE* f, vec3* v);
extern inline float len_v3(vec3 a);
extern inline vec3 norm_v3(vec3 a);
extern inline void normalize_v3(vec3* a);
extern inline vec3 add_v3s(vec3 a, vec3 b);
extern inline vec3 sub_v3s(vec3 a, vec3 b);
extern inline vec3 mult_v3s(vec3 a, vec3 b);
extern inline vec3 div_v3s(vec3 a, vec3 b);
extern inline float dot_v3s(vec3 a, vec3 b);
extern inline vec3 scale_v3(vec3 a, float s);
extern inline int equal_v3s(vec3 a, vec3 b);
extern inline int equal_epsilon_v3s(vec3 a, vec3 b, float epsilon);
extern inline vec3 cross_v3s(const vec3 u, const vec3 v);
extern inline float angle_v3s(const vec3 u, const vec3 v);

