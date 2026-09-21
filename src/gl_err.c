
// TODO different name? NO_ERROR_CHECKING? LOOK_MA_NO_HANDS?
#ifdef PGL_UNSAFE
#define PGL_LOG_NAMED(err, fn)
#define PGL_SET_ERR_NAMED(err, fn)
#define PGL_ERR_NAMED(check, err, fn)
#define PGL_SET_ERR_RET_NAMED(err, fn) return
#define PGL_ERR_RET_VAL_NAMED(check, err, ret, fn)
#define PGL_LOG(err)
#define PGL_SET_ERR(err)
#define PGL_ERR(check, err)
#define PGL_SET_ERR_RET(err) return
#define PGL_ERR_RET_VAL(check, err, ret)
#else
// Helpers that PGL_ERR take const char* api and use PGL_*_NAMED(..., api)
// so the log names the GL/PGL entry point. Public functions keep PGL_ERR
// which passes __func__.
#define PGL_LOG_NAMED(err, fn) \
	do { \
		if (c->dbg_output && c->dbg_callback) { \
			int len = snprintf(c->dbg_msg_buf, PGL_MAX_DEBUG_MESSAGE_LENGTH, "%s in %s() at %s:%d", pgl_err_strs[(err)-GL_NO_ERROR], (fn), __FILE__, __LINE__); \
			c->dbg_callback(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_HIGH, len, c->dbg_msg_buf, c->dbg_userparam); \
		} \
	} while (0)

#define PGL_SET_ERR_NAMED(err, fn) \
	do { \
		if (!c->error) c->error = (err); \
		PGL_LOG_NAMED(err, fn); \
	} while (0)

#define PGL_ERR_NAMED(check, err, fn) \
	do { \
		if (check) { \
			if (!c->error) c->error = (err); \
			PGL_LOG_NAMED(err, fn); \
			return; \
		} \
	} while (0)

#define PGL_SET_ERR_RET_NAMED(err, fn) \
	do { \
		PGL_SET_ERR_NAMED(err, fn); \
		return; \
	} while (0)

#define PGL_ERR_RET_VAL_NAMED(check, err, ret, fn) \
	do { \
		if (check) { \
			if (!c->error) c->error = (err); \
			PGL_LOG_NAMED(err, fn); \
			return (ret); \
		} \
	} while (0)

#define PGL_LOG(err)                     PGL_LOG_NAMED(err, __func__)
#define PGL_SET_ERR(err)                 PGL_SET_ERR_NAMED(err, __func__)
#define PGL_ERR(check, err)              PGL_ERR_NAMED(check, err, __func__)
#define PGL_SET_ERR_RET(err)             PGL_SET_ERR_RET_NAMED(err, __func__)
#define PGL_ERR_RET_VAL(check, err, ret) PGL_ERR_RET_VAL_NAMED(check, err, ret, __func__)
#endif

#ifndef PGL_UNSAFE
static const char* pgl_err_strs[] =
{
	"GL_NO_ERROR",
	"GL_INVALID_ENUM",
	"GL_INVALID_VALUE",
	"GL_INVALID_OPERATION",
	"GL_INVALID_FRAMEBUFFER_OPERATION",
	"GL_OUT_OF_MEMORY"
};
#endif

