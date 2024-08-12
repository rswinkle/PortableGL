
typedef struct dvec2
{
	double x;
	double y;
} dvec2;

inline void fprint_dvec2(FILE* f, dvec2 v, const char* append)
{
	fprintf(f, "(%f, %f)%s", v.x, v.y, append);
}

inline int fread_dvec2(FILE* f, dvec2* v)
{
	int tmp = fscanf(f, " (%lf, %lf)", &v->x, &v->y);
	return (tmp == 2);
}

