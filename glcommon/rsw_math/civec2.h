
typedef struct ivec2
{
	int x;
	int y;
} ivec2;

RSW_INLINE ivec2 make_iv2(int x, int y)
{
	ivec2 v = { x, y };
	return v;
}

RSW_INLINE void fprint_iv2(FILE* f, ivec2 v, const char* append)
{
	fprintf(f, "(%d, %d)%s", v.x, v.y, append);
}

RSW_INLINE int fread_iv2(FILE* f, ivec2* v)
{
	int tmp = fscanf(f, " (%d, %d)", &v->x, &v->y);
	return (tmp == 2);
}

