
extern inline vec3 make_vec3(float x, float y, float z);
extern inline vec3 negate_vec3(vec3 v);
extern inline void fprint_vec3(FILE* f, vec3 v, const char* append);
extern inline void print_vec3(vec3 v, const char* append);
extern inline int fread_vec3(FILE* f, vec3* v);
extern inline float length_vec3(vec3 a);
extern inline vec3 norm_vec3(vec3 a);
extern inline void normalize_vec3(vec3* a);
extern inline vec3 add_vec3s(vec3 a, vec3 b);
extern inline vec3 sub_vec3s(vec3 a, vec3 b);
extern inline vec3 mult_vec3s(vec3 a, vec3 b);
extern inline vec3 div_vec3s(vec3 a, vec3 b);
extern inline float dot_vec3s(vec3 a, vec3 b);
extern inline vec3 scale_vec3(vec3 a, float s);
extern inline int equal_vec3s(vec3 a, vec3 b);
extern inline int equal_epsilon_vec3s(vec3 a, vec3 b, float epsilon);
extern inline vec3 cross_product(const vec3 u, const vec3 v);
extern inline float angle_between_vec3(const vec3 u, const vec3 v);

