
RSW_INLINE vec2 v3_to_v2(vec3 a)
{
	vec2 v = { a.x, a.y };
	return v;
}

RSW_INLINE vec2 v4_to_v2(vec4 a)
{
	vec2 v = { a.x, a.y };
	return v;
}

RSW_INLINE vec3 v4_to_v3(vec4 a)
{
	vec3 v = { a.x, a.y, a.z };
	return v;
}

RSW_INLINE vec2 v4_to_v2h(vec4 a)
{
	vec2 v = { a.x/a.w, a.y/a.w };
	return v;
}

RSW_INLINE vec3 v4_to_v3h(vec4 a)
{
	vec3 v = { a.x/a.w, a.y/a.w, a.z/a.w };
	return v;
}

