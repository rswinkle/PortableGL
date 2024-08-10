
inline vec2 vec4_to_vec2(vec4 a)
{
	vec2 v = { a.x, a.y };
	return v;
}

inline vec3 vec4_to_vec3(vec4 a)
{
	vec3 v = { a.x, a.y, a.z };
	return v;
}

inline vec2 vec4_to_vec2h(vec4 a)
{
	vec2 v = { a.x/a.w, a.y/a.w };
	return v;
}

inline vec3 vec4_to_vec3h(vec4 a)
{
	vec3 v = { a.x/a.w, a.y/a.w, a.z/a.w };
	return v;
}

