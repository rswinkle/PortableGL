
typedef struct ivec4
{
	int x;
	int y;
	int z;
	int w;
} ivec4;

inline ivec4 make_iv4(int x, int y, int z, int w)
{
	ivec4 v = { x, y, z, w };
	return v;
}

inline void fprint_iv4(FILE* f, ivec4 v, const char* append)
{
	fprintf(f, "(%d, %d, %d, %d)%s", v.x, v.y, v.z, v.w, append);
}

inline int fread_iv4(FILE* f, ivec4* v)
{
	int tmp = fscanf(f, " (%d, %d, %d, %d)", &v->x, &v->y, &v->z, &v->w);
	return (tmp == 4);
}


