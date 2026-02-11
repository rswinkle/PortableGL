
typedef struct uvec4
{
	unsigned int x;
	unsigned int y;
	unsigned int z;
	unsigned int w;
} uvec4;

inline uvec4 make_uv4(unsigned int x, unsigned int y, unsigned int z, unsigned int w)
{
	uvec4 v = { x, y, z, w };
	return v;
}

inline void fprint_uv4(FILE* f, uvec4 v, const char* append)
{
	fprintf(f, "(%u, %u, %u, %u)%s", v.x, v.y, v.z, v.w, append);
}

inline int fread_uv4(FILE* f, uvec4* v)
{
	int tmp = fscanf(f, " (%u, %u, %u, %u)", &v->x, &v->y, &v->z, &v->w);
	return (tmp == 4);
}

