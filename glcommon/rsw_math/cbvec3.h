
typedef struct bvec3
{
	u8 x;
	u8 y;
	u8 z;
} bvec3;

inline bvec3 make_bv3(int x, int y, int z)
{
	bvec3 v = { !!x, !!y, !!z };
	return v;
}

inline void fprint_bv3(FILE* f, bvec3 v, const char* append)
{
	fprintf(f, "(%u, %u, %u)%s", v.x, v.y, v.z, append);
}

inline int fread_bv3(FILE* f, bvec3* v)
{
	int tmp = fscanf(f, " (%hhu, %hhu, %hhu)", &v->x, &v->y, &v->z);
	return (tmp == 3);
}

