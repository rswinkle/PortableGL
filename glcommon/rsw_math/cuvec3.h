
typedef struct uvec3
{
	unsigned int x;
	unsigned int y;
	unsigned int z;
} uvec3;

inline uvec3 make_uv3(unsigned int x, unsigned int y, unsigned int z)
{
	uvec3 v = { x, y, z };
	return v;
}

inline void fprint_uv3(FILE* f, uvec3 v, const char* append)
{
	fprintf(f, "(%u, %u, %u)%s", v.x, v.y, v.z, append);
}

inline int fread_uv3(FILE* f, uvec3* v)
{
	int tmp = fscanf(f, " (%u, %u, %u)", &v->x, &v->y, &v->z);
	return (tmp == 3);
}

