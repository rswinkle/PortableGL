
typedef struct uvec2
{
	unsigned int x;
	unsigned int y;
} uvec2;

inline uvec2 make_uv2(unsigned int x, unsigned int y)
{
	uvec2 v = { x, y };
	return v;
}

inline void fprint_uv2(FILE* f, uvec2 v, const char* append)
{
	fprintf(f, "(%u, %u)%s", v.x, v.y, append);
}

inline int fread_uv2(FILE* f, uvec2* v)
{
	int tmp = fscanf(f, " (%u, %u)", &v->x, &v->y);
	return (tmp == 2);
}

