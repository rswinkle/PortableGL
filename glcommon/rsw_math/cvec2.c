
extern inline vec2 make_vec2(float x, float y);
extern inline vec2 negate_vec2(vec2 v);
extern inline void fprint_vec2(FILE* f, vec2 v, const char* append);
extern inline void print_vec2(vec2 v, const char* append);
extern inline int fread_vec2(FILE* f, vec2* v);
extern inline float length_vec2(vec2 a);
extern inline vec2 norm_vec2(vec2 a);
extern inline void normalize_vec2(vec2* a);
extern inline vec2 add_vec2s(vec2 a, vec2 b);
extern inline vec2 sub_vec2s(vec2 a, vec2 b);
extern inline vec2 mult_vec2s(vec2 a, vec2 b);
extern inline vec2 div_vec2s(vec2 a, vec2 b);
extern inline float dot_vec2s(vec2 a, vec2 b);
extern inline vec2 scale_vec2(vec2 a, float s);
extern inline int equal_vec2s(vec2 a, vec2 b);
extern inline int equal_epsilon_vec2s(vec2 a, vec2 b, float epsilon);


