

void clip_xy(int argc, char** argv, void* data)
{
	float points[] = {
		// top and bottom, 1 and 2, CCW and CW
		-0.7, 0.8, 0,
		-0.3, 0.8, 0,
		-0.5, 1.2, 0,

		 0.3, 1.2, 0,
		 0.7, 1.2, 0,
		 0.5, 0.8, 0,

		-0.3, -0.8, 0,
		-0.7, -0.8, 0,
		-0.5, -1.2, 0,

		 0.3, -1.2, 0,
		 0.7, -1.2, 0,
		 0.5, -0.8, 0,

		// left and right, 1 and 2, CCW and CW
		-0.8, -0.7, 0,
		-0.8, -0.3, 0,
		-1.2, -0.5, 0,

		-1.2,  0.3, 0,
		-1.2,  0.7, 0,
		-0.8,  0.5, 0,

		 0.8, -0.3, 0,
		 0.8, -0.7, 0,
		 1.2, -0.5, 0,

		 1.2,  0.7, 0,
		 1.2,  0.3, 0,
		 0.8,  0.5, 0,
	};

	switch (argc) {
		case 1:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case 2:
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
		case 3:
			glLineWidth(8);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case 4:
			glPointSize(8);
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
		case 5:
			glLineWidth(32);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case 6:
			glPointSize(32);
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
		default:
			break;
	}


	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Don't need a shader or uniform, just using the default shader 0
	// which is just a passthrough vs, draw everything red fs

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 24);

}

void clip_z(int argc, char** argv, void* data)
{
	float points[] = {
		// bottom three are CCW
		-0.9, -0.8, 0,
		-0.5, -0.8, 0,
		-0.7, -0.3, 0,

		-0.2, -0.8, 0,
		 0.2, -0.8, 0,
		 0.0, -0.3, -1.3,

		 0.5, -0.8, 1.3,
		 0.9, -0.8, 1.3,
		 0.7, -0.3, 0,

		// top three are CW
		-0.9, 0.8, 0,
		-0.5, 0.8, 0,
		-0.7, 0.3, 0,

		-0.2, 0.8, 0,
		 0.2, 0.8, 0,
		 0.0, 0.3, -1.3,

		 0.5, 0.8, 1.3,
		 0.9, 0.8, 1.3,
		 0.7, 0.3, 0,
	};

	switch (argc) {
		case 1:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case 2:
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
		case 3:
			glLineWidth(8);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case 4:
			glPointSize(8);
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
		case 5:
			glLineWidth(32);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case 6:
			glPointSize(32);
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
		case 7:
			glEnable(GL_DEPTH_CLAMP);
		default:
			break;
	}


	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Don't need a shader or uniform, just using the default shader 0
	// which is just a passthrough vs, draw everything red fs

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 18);

}

void clip_pnts_lns(int argc, char** argv, void* data)
{
	float points_n_lines[] = {
		// test -x and +y
		-1.1, 0.7, 0,
		-0.7, 1.1, 0,

		// +x and -y
		 1.1, -0.7, 0,
		 0.7, -1.1, 0,

		 // +z and -z
		-0.3, 0.5, 1.5,
		0.3, -0.5, -1.5,

		// points below
		// +z and -z for points
		-0.3, -0.3, 1.2,
		 0.3,  0.3, -1.2,

		 -0.9, 0.5, 0,
		  0.9, 0.5, 0,

		 -1.02, -0.5, 0,
		  1.02, -0.5, 0
	};

	switch (argc) {
		case 1:
			glPointSize(8);
			glLineWidth(8);
			break;
		case 2:
			glPointSize(32);
			glLineWidth(32);
			break;
		default:
			break;
	}


	GLuint verts;
	glGenBuffers(1, &verts);
	glBindBuffer(GL_ARRAY_BUFFER, verts);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points_n_lines), points_n_lines, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Don't need a shader or uniform, just using the default shader 0
	// which is just a passthrough vs, draw everything red fs

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_LINES, 0, 6);
	glDrawArrays(GL_POINTS, 6, 6);
}

// TODO test clipping z after perspective projection?
// Test interaction with depth clamp?

void skybox_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void skybox_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

void clip_pers_proj(int argc, char** argv, void* data)
{
	// verts + colors
	float skybox[] = {
		// front
		-1.0f,  1.0f, -1.0f,  1.0f, 0.0f, 0.0f,
		-1.0f, -1.0f, -1.0f,  1.0f, 0.0f, 0.0f,
		 1.0f, -1.0f, -1.0f,  1.0f, 0.0f, 0.0f,
		 1.0f, -1.0f, -1.0f,  1.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, -1.0f,  1.0f, 0.0f, 0.0f,
		-1.0f,  1.0f, -1.0f,  1.0f, 0.0f, 0.0f,

		-1.0f, -1.0f,  1.0f,  0.0f, 1.0f, 0.0f,
		-1.0f, -1.0f, -1.0f,  0.0f, 1.0f, 0.0f,
		-1.0f,  1.0f, -1.0f,  0.0f, 1.0f, 0.0f,
		-1.0f,  1.0f, -1.0f,  0.0f, 1.0f, 0.0f,
		-1.0f,  1.0f,  1.0f,  0.0f, 1.0f, 0.0f,
		-1.0f, -1.0f,  1.0f,  0.0f, 1.0f, 0.0f,

		 1.0f, -1.0f, -1.0f,  0.0f, 0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f,  0.0f, 0.0f, 1.0f,
		 1.0f,  1.0f,  1.0f,  0.0f, 0.0f, 1.0f,
		 1.0f,  1.0f,  1.0f,  0.0f, 0.0f, 1.0f,
		 1.0f,  1.0f, -1.0f,  0.0f, 0.0f, 1.0f,
		 1.0f, -1.0f, -1.0f,  0.0f, 0.0f, 1.0f,

		 // back (what we actually see since we set the look vector to +1 z)
		-1.0f, -1.0f,  1.0f,  1.0f, 1.0f, 0.0f,
		-1.0f,  1.0f,  1.0f,  1.0f, 1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f,  1.0f, 1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f,  1.0f, 1.0f, 0.0f,
		 1.0f, -1.0f,  1.0f,  1.0f, 1.0f, 0.0f,
		-1.0f, -1.0f,  1.0f,  1.0f, 1.0f, 0.0f,

		-1.0f,  1.0f, -1.0f,  0.0f, 1.0f, 1.0f,
		 1.0f,  1.0f, -1.0f,  0.0f, 1.0f, 1.0f,
		 1.0f,  1.0f,  1.0f,  0.0f, 1.0f, 1.0f,
		 1.0f,  1.0f,  1.0f,  0.0f, 1.0f, 1.0f,
		-1.0f,  1.0f,  1.0f,  0.0f, 1.0f, 1.0f,
		-1.0f,  1.0f, -1.0f,  0.0f, 1.0f, 1.0f,

		-1.0f, -1.0f, -1.0f,  1.0f, 0.0f, 1.0f,
		-1.0f, -1.0f,  1.0f,  1.0f, 0.0f, 1.0f,
		 1.0f, -1.0f, -1.0f,  1.0f, 0.0f, 1.0f,
		 1.0f, -1.0f, -1.0f,  1.0f, 0.0f, 1.0f,
		-1.0f, -1.0f,  1.0f,  1.0f, 0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f,  1.0f, 0.0f, 1.0f
	};
	GLuint skyboxVBO;
	glGenBuffers(1, &skyboxVBO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skybox), &skybox, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
	glEnableVertexAttribArray(1);
	pglVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 3*sizeof(float));

	GLenum smooth[] = { PGL_SMOOTH3 };
	GLenum flat[] = { PGL_FLAT3 };
	GLuint shader = pglCreateProgram(skybox_vs, skybox_fs, 3, flat, GL_FALSE);
	glUseProgram(shader);
	pgl_uniforms uniforms;
	pglSetUniform(&uniforms);

	mat4 proj, view;
	make_perspective_m4(proj, radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
	vec3 eye = { 0.0f, 0.0f, 0.0f };
	vec3 up = { 0.0f, 1.0f, 0.0f };
	vec3 forward = { 0.0f, 0.0f, 1.0f };
	vec3 nf = norm_v3(forward);
	lookAt(view, eye, add_v3s(eye, nf), up);

	mult_m4_m4(uniforms.mvp_mat, proj, view);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
	glUseProgram(shader);

	// skybox cube
	glDrawArrays(GL_TRIANGLES, 0, 36);
}

void skybox_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	pgl_uniforms* u = (pgl_uniforms*)uniforms;

	// the extra 1 doesn't matter
	((vec4*)vs_output)[0] = vertex_attribs[1];

	vec4 pos = mult_m4_v4(u->mvp_mat, vertex_attribs[0]);
	//builtins->gl_Position = make_vec4(pos.x, pos.y, pos.z, pos.w);
	builtins->gl_Position = make_v4(pos.x, pos.y, pos.w, pos.w);
}

void skybox_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	//pgl_uniforms* u = (pgl_uniforms*)uniforms;
	//vec3 TexCoords = *(vec3*)&fs_input[0];
	vec3 color = *(vec3*)&fs_input[0];

	//builtins->gl_FragColor = texture_cubemap(u->tex, TexCoords.x, TexCoords.y, TexCoords.z);
	builtins->gl_FragColor = make_v4(color.x, color.y, color.z, 1.0f);
}











