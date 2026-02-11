
extern inline vec2 make_v2(float x, float y);
extern inline vec2 neg_v2(vec2 v);
extern inline void fprint_v2(FILE* f, vec2 v, const char* append);
extern inline void print_v2(vec2 v, const char* append);
extern inline int fread_v2(FILE* f, vec2* v);
extern inline float len_v2(vec2 a);
extern inline vec2 norm_v2(vec2 a);
extern inline void normalize_v2(vec2* a);
extern inline vec2 add_v2s(vec2 a, vec2 b);
extern inline vec2 sub_v2s(vec2 a, vec2 b);
extern inline vec2 mult_v2s(vec2 a, vec2 b);
extern inline vec2 div_v2s(vec2 a, vec2 b);
extern inline float dot_v2s(vec2 a, vec2 b);
extern inline vec2 scale_v2(vec2 a, float s);
extern inline int equal_v2s(vec2 a, vec2 b);
extern inline int equal_epsilon_v2s(vec2 a, vec2 b, float epsilon);
extern inline float cross_v2s(vec2 a, vec2 b);
extern inline float angle_v2s(vec2 a, vec2 b);

