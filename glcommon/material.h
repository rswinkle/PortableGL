#ifndef MATERIAL_H
#define MATERIAL_H


class material
{
public:

	
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;

	
	material(vec3 a=vec3(), vec3 d=vec3(1,1,1), vec3 s= vec3(0,0,0), float shine=0)
	{
		ambient = a;
		diffuse = d;
		specular = s;
		shininess = shine;
	}
	
	void set_material(vec3 amb, vec3 dif, vec3 s, float shine)
	{
		ambient = amb;
		diffuse = dif;
		specular = s;
		shininess = shine;
	}
	
	void pglSetUniforms(GLSLProgram *program, bool K = true)
	{
		if (K) {
			program->pglSetUniform("Ka", ambient);
			program->pglSetUniform("Kd", diffuse);
			program->pglSetUniform("Ks", specular);
		}
		program->pglSetUniform("Shininess", shininess);
	}
	
	
	
};




#endif
