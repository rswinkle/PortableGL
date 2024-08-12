
typedef struct dvec4
{
	double x;
	double y;
	double z;
	double w;
} dvec4;

inline void fprint_dvec4(FILE* f, dvec4 v, const char* append)
{
	fprintf(f, "(%f, %f, %f, %f)%s", v.x, v.y, v.z, v.w, append);
}

inline int fread_dvec4(FILE* f, dvec4* v)
{
	int tmp = fscanf(f, " (%lf, %lf, %lf, %lf)", &v->x, &v->y, &v->z, &v->w);
	return (tmp == 4);
}

