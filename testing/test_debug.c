// KHR_debug / context-flag queries. Pass/fail encoded as a green/red clear.

#include <string.h>

#ifndef PGL_UNSAFE
static char dbg_last_msg[PGL_MAX_DEBUG_MESSAGE_LENGTH];

static void test_dbg_capture(GLenum source, GLenum type, GLuint id, GLenum severity,
                             GLsizei length, const GLchar* message, const void* userParam)
{
	PGL_UNUSED(source);
	PGL_UNUSED(type);
	PGL_UNUSED(id);
	PGL_UNUSED(severity);
	PGL_UNUSED(length);
	PGL_UNUSED(userParam);
	snprintf(dbg_last_msg, sizeof(dbg_last_msg), "%s", message ? message : "");
}
#endif

void test_debug_context(int num, char** argv, void* data)
{
	PGL_UNUSED(num);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	GLint flags = 0;
	glGetError();
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	PGL_EXPECT(glGetError() == GL_NO_ERROR, "CONTEXT_FLAGS query");
#ifndef PGL_UNSAFE
	PGL_EXPECT((flags & (GLint)GL_CONTEXT_FLAG_DEBUG_BIT) != 0, "debug bit set");
	PGL_EXPECT(glIsEnabled(GL_DEBUG_OUTPUT) == GL_TRUE, "DEBUG_OUTPUT default on");
#else
	PGL_EXPECT((flags & (GLint)GL_CONTEXT_FLAG_DEBUG_BIT) == 0, "unsafe: no debug bit");
	PGL_EXPECT(glIsEnabled(GL_DEBUG_OUTPUT) == GL_FALSE, "unsafe: DEBUG_OUTPUT off");
#endif

	PGL_EXPECT(glIsEnabled(GL_DEBUG_OUTPUT_SYNCHRONOUS) == GL_FALSE,
	           "SYNCHRONOUS default off");

	GLboolean b = GL_FALSE;
	glGetBooleanv(GL_DEBUG_OUTPUT, &b);
#ifndef PGL_UNSAFE
	PGL_EXPECT(b == GL_TRUE, "GetBooleanv DEBUG_OUTPUT");
#else
	PGL_EXPECT(b == GL_FALSE, "GetBooleanv DEBUG_OUTPUT unsafe");
#endif

	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	PGL_EXPECT(glIsEnabled(GL_DEBUG_OUTPUT_SYNCHRONOUS) == GL_TRUE, "SYNCHRONOUS enable");
	glGetBooleanv(GL_DEBUG_OUTPUT_SYNCHRONOUS, &b);
	PGL_EXPECT(b == GL_TRUE, "GetBooleanv SYNCHRONOUS");
	glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	PGL_EXPECT(glIsEnabled(GL_DEBUG_OUTPUT_SYNCHRONOUS) == GL_FALSE, "SYNCHRONOUS disable");

	// LearnOpenGL 7.1 catch-all
	glGetError();
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
	PGL_EXPECT(glGetError() == GL_NO_ERROR, "DebugMessageControl catch-all");

#ifndef PGL_UNSAFE
	glDebugMessageCallback(test_dbg_capture, NULL);

	GLuint mip_tex;
	glGenTextures(1, &mip_tex);
	glBindTexture(GL_TEXTURE_2D, mip_tex);
	dbg_last_msg[0] = 0;
	glGetError();
	glGenerateMipmap(GL_TEXTURE_2D);
	PGL_EXPECT(glGetError() == GL_INVALID_OPERATION, "GenerateMipmap empty texture");
	PGL_EXPECT(strstr(dbg_last_msg, "glGenerateMipmap") != NULL,
	           "GenerateMipmap log names the GL API");
	PGL_EXPECT(strstr(dbg_last_msg, "pgl_generate_mipmap_tex") == NULL,
	           "GenerateMipmap log is not the helper");

	dbg_last_msg[0] = 0;
	glGetError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_REPEAT);
	PGL_EXPECT(glGetError() == GL_INVALID_ENUM, "TexParameteri bad mag filter");
	PGL_EXPECT(strstr(dbg_last_msg, "glTexParameteri") != NULL,
	           "TexParameteri log names the GL API");
	PGL_EXPECT(strstr(dbg_last_msg, "set_texparami") == NULL,
	           "TexParameteri log is not the helper");

	glDebugMessageCallback(NULL, NULL);
	glDisable(GL_DEBUG_OUTPUT); // don't print expected errors
	PGL_EXPECT(glIsEnabled(GL_DEBUG_OUTPUT) == GL_FALSE, "DEBUG_OUTPUT disable");

	glGetError();
	glDebugMessageControl(GL_DEBUG_OUTPUT, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
	PGL_EXPECT(glGetError() == GL_INVALID_ENUM, "bad source INVALID_ENUM");

	glGetError();
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, -1, NULL, GL_TRUE);
	PGL_EXPECT(glGetError() == GL_INVALID_VALUE, "count < 0 INVALID_VALUE");

	GLuint id = 1;
	glGetError();
	glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_ERROR, GL_DONT_CARE, 1, &id, GL_TRUE);
	PGL_EXPECT(glGetError() == GL_INVALID_OPERATION,
	           "count>0 + DONT_CARE source INVALID_OPERATION");

	glEnable(GL_DEBUG_OUTPUT);
	PGL_EXPECT(glIsEnabled(GL_DEBUG_OUTPUT) == GL_TRUE, "re-enable DEBUG_OUTPUT");
#endif

	if (pgl_test_fails == 0)
		glClearColor(0.f, 0.55f, 0.1f, 1.f);
	else
		glClearColor(0.8f, 0.05f, 0.05f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
}
