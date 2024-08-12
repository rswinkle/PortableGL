
typedef struct dvec3
{
	double x;
	double y;
	double z;
} dvec3;

inline void fprint_dvec3(FILE* f, dvec3 v, const char* append)
{
	fprintf(f, "(%f, %f, %f)%s", v.x, v.y, v.z, append);
}

inline int fread_dvec3(FILE* f, dvec3* v)
{
	int tmp = fscanf(f, " (%lf, %lf, %lf)", &v->x, &v->y, &v->z);
	return (tmp == 3);
}

