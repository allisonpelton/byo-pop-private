#ifndef __GLFUNCS_H__
#define __GLFUNCS_H__

// port: the OpenGL ES 3.0 / WebGL2 functions the GPU path uses (framework/GPUInterface.h), loaded at run time through
// SDL_GL_GetProcAddress once a context is current (platform/GLContext.h). Nothing links against a GL library, so games
// that never make a GL context are built as before. The same calls exist in GL 3.3 core, which is the fallback where
// the driver has no ES context.

#include <cstddef>
#include <cstdint>

namespace Sexy
{

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef char GLchar;
typedef std::ptrdiff_t GLsizeiptr;
typedef std::ptrdiff_t GLintptr;

enum : GLenum
{
	GL_ZERO = 0, GL_ONE = 1, GL_FALSE_ = 0, GL_TRUE_ = 1,
	GL_LINES = 0x0001, GL_LINE_STRIP = 0x0003, GL_TRIANGLES = 0x0004, GL_TRIANGLE_STRIP = 0x0005, GL_TRIANGLE_FAN = 0x0006,
	GL_NEVER = 0x0200, GL_LESS = 0x0201, GL_EQUAL = 0x0202, GL_LEQUAL = 0x0203, GL_GREATER = 0x0204,
	GL_NOTEQUAL = 0x0205, GL_GEQUAL = 0x0206, GL_ALWAYS = 0x0207,
	GL_SRC_COLOR = 0x0300, GL_ONE_MINUS_SRC_COLOR = 0x0301, GL_SRC_ALPHA = 0x0302, GL_ONE_MINUS_SRC_ALPHA = 0x0303,
	GL_DST_ALPHA = 0x0304, GL_ONE_MINUS_DST_ALPHA = 0x0305, GL_DST_COLOR = 0x0306, GL_ONE_MINUS_DST_COLOR = 0x0307,
	GL_SRC_ALPHA_SATURATE = 0x0308,
	GL_FRONT = 0x0404, GL_BACK = 0x0405, GL_CW = 0x0900, GL_CCW = 0x0901,
	GL_CULL_FACE = 0x0B44, GL_DEPTH_TEST = 0x0B71, GL_DITHER = 0x0BD0, GL_BLEND = 0x0BE2, GL_SCISSOR_TEST = 0x0C11,
	GL_UNPACK_ALIGNMENT = 0x0CF5, GL_PACK_ALIGNMENT = 0x0D05,
	GL_TEXTURE_2D = 0x0DE1,
	GL_UNSIGNED_BYTE = 0x1401, GL_UNSIGNED_SHORT = 0x1403, GL_FLOAT = 0x1406,
	GL_RGB = 0x1907, GL_RGBA = 0x1908,
	GL_VENDOR = 0x1F00, GL_RENDERER = 0x1F01, GL_VERSION = 0x1F02,
	GL_NEAREST = 0x2600, GL_LINEAR = 0x2601,
	GL_TEXTURE_MAG_FILTER = 0x2800, GL_TEXTURE_MIN_FILTER = 0x2801, GL_TEXTURE_WRAP_S = 0x2802,
	GL_TEXTURE_WRAP_T = 0x2803, GL_REPEAT = 0x2901,
	GL_CLAMP_TO_EDGE = 0x812F, GL_MIRRORED_REPEAT = 0x8370, GL_TEXTURE_MAX_LEVEL = 0x813D,
	GL_RGB8 = 0x8051, GL_RGBA8 = 0x8058,
	GL_TEXTURE0 = 0x84C0,
	GL_DEPTH_COMPONENT16 = 0x81A5, GL_DEPTH_COMPONENT24 = 0x81A6,
	GL_ARRAY_BUFFER = 0x8892, GL_ELEMENT_ARRAY_BUFFER = 0x8893, GL_STREAM_DRAW = 0x88E0, GL_STATIC_DRAW = 0x88E4,
	GL_FRAGMENT_SHADER = 0x8B30, GL_VERTEX_SHADER = 0x8B31, GL_COMPILE_STATUS = 0x8B81, GL_LINK_STATUS = 0x8B82,
	GL_INFO_LOG_LENGTH = 0x8B84,
	GL_READ_FRAMEBUFFER = 0x8CA8, GL_DRAW_FRAMEBUFFER = 0x8CA9, GL_FRAMEBUFFER_COMPLETE = 0x8CD5,
	GL_COLOR_ATTACHMENT0 = 0x8CE0, GL_DEPTH_ATTACHMENT = 0x8D00, GL_FRAMEBUFFER = 0x8D40, GL_RENDERBUFFER = 0x8D41,
	GL_COLOR_BUFFER_BIT = 0x00004000, GL_DEPTH_BUFFER_BIT = 0x00000100,
	GL_NO_ERROR = 0
};

#define BYOPOP_GL_FUNCS(X) \
	X(void, glActiveTexture, (GLenum texture)) \
	X(void, glAttachShader, (GLuint program, GLuint shader)) \
	X(void, glBindAttribLocation, (GLuint program, GLuint index, const GLchar* name)) \
	X(void, glBindBuffer, (GLenum target, GLuint buffer)) \
	X(void, glBindFramebuffer, (GLenum target, GLuint framebuffer)) \
	X(void, glBindRenderbuffer, (GLenum target, GLuint renderbuffer)) \
	X(void, glBindSampler, (GLuint unit, GLuint sampler)) \
	X(void, glBindTexture, (GLenum target, GLuint texture)) \
	X(void, glBindVertexArray, (GLuint array)) \
	X(void, glBlendFunc, (GLenum sfactor, GLenum dfactor)) \
	X(void, glBlitFramebuffer, (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, \
		GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)) \
	X(void, glBufferData, (GLenum target, GLsizeiptr size, const void* data, GLenum usage)) \
	X(GLenum, glCheckFramebufferStatus, (GLenum target)) \
	X(void, glClear, (GLbitfield mask)) \
	X(void, glClearColor, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)) \
	X(void, glClearDepthf, (GLfloat d)) \
	X(void, glColorMask, (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)) \
	X(void, glCompileShader, (GLuint shader)) \
	X(GLuint, glCreateProgram, (void)) \
	X(GLuint, glCreateShader, (GLenum type)) \
	X(void, glCullFace, (GLenum mode)) \
	X(void, glDeleteBuffers, (GLsizei n, const GLuint* buffers)) \
	X(void, glDeleteFramebuffers, (GLsizei n, const GLuint* framebuffers)) \
	X(void, glDeleteProgram, (GLuint program)) \
	X(void, glDeleteRenderbuffers, (GLsizei n, const GLuint* renderbuffers)) \
	X(void, glDeleteSamplers, (GLsizei count, const GLuint* samplers)) \
	X(void, glDeleteShader, (GLuint shader)) \
	X(void, glDeleteTextures, (GLsizei n, const GLuint* textures)) \
	X(void, glDeleteVertexArrays, (GLsizei n, const GLuint* arrays)) \
	X(void, glDepthFunc, (GLenum func)) \
	X(void, glDepthMask, (GLboolean flag)) \
	X(void, glDepthRangef, (GLfloat n, GLfloat f)) \
	X(void, glDisable, (GLenum cap)) \
	X(void, glDisableVertexAttribArray, (GLuint index)) \
	X(void, glDrawArrays, (GLenum mode, GLint first, GLsizei count)) \
	X(void, glDrawElements, (GLenum mode, GLsizei count, GLenum type, const void* indices)) \
	X(void, glEnable, (GLenum cap)) \
	X(void, glEnableVertexAttribArray, (GLuint index)) \
	X(void, glFramebufferRenderbuffer, (GLenum target, GLenum attachment, GLenum renderbuffertarget, \
		GLuint renderbuffer)) \
	X(void, glFramebufferTexture2D, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)) \
	X(void, glFrontFace, (GLenum mode)) \
	X(void, glGenBuffers, (GLsizei n, GLuint* buffers)) \
	X(void, glGenFramebuffers, (GLsizei n, GLuint* framebuffers)) \
	X(void, glGenRenderbuffers, (GLsizei n, GLuint* renderbuffers)) \
	X(void, glGenSamplers, (GLsizei count, GLuint* samplers)) \
	X(void, glGenTextures, (GLsizei n, GLuint* textures)) \
	X(void, glGenVertexArrays, (GLsizei n, GLuint* arrays)) \
	X(GLenum, glGetError, (void)) \
	X(void, glGetProgramInfoLog, (GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog)) \
	X(void, glGetProgramiv, (GLuint program, GLenum pname, GLint* params)) \
	X(void, glGetShaderInfoLog, (GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog)) \
	X(void, glGetShaderiv, (GLuint shader, GLenum pname, GLint* params)) \
	X(const unsigned char*, glGetString, (GLenum name)) \
	X(GLint, glGetUniformLocation, (GLuint program, const GLchar* name)) \
	X(void, glLinkProgram, (GLuint program)) \
	X(void, glPixelStorei, (GLenum pname, GLint param)) \
	X(void, glReadPixels, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels)) \
	X(void, glRenderbufferStorage, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height)) \
	X(void, glSamplerParameteri, (GLuint sampler, GLenum pname, GLint param)) \
	X(void, glScissor, (GLint x, GLint y, GLsizei width, GLsizei height)) \
	X(void, glShaderSource, (GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length)) \
	X(void, glTexImage2D, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, \
		GLint border, GLenum format, GLenum type, const void* pixels)) \
	X(void, glTexParameteri, (GLenum target, GLenum pname, GLint param)) \
	X(void, glTexSubImage2D, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, \
		GLsizei height, GLenum format, GLenum type, const void* pixels)) \
	X(void, glUniform1i, (GLint location, GLint v0)) \
	X(void, glUniform1f, (GLint location, GLfloat v0)) \
	X(void, glUniform4fv, (GLint location, GLsizei count, const GLfloat* value)) \
	X(void, glUniformMatrix4fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)) \
	X(void, glUseProgram, (GLuint program)) \
	X(void, glVertexAttribPointer, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, \
		const void* pointer)) \
	X(void, glVertexAttrib4f, (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)) \
	X(void, glViewport, (GLint x, GLint y, GLsizei width, GLsizei height))

// GL's calling convention: __stdcall on 32-bit Windows (APIENTRY), the default elsewhere
#if defined(_WIN32) && !defined(_WIN64)
#define BYOPOP_GLAPI __stdcall
#else
#define BYOPOP_GLAPI
#endif

// The calls go through the gGL table: gGL.glClear(...)
struct GLFuncs
{
#define BYOPOP_GL_DECLARE(ret, name, args) ret (BYOPOP_GLAPI* name) args;
	BYOPOP_GL_FUNCS(BYOPOP_GL_DECLARE)
#undef BYOPOP_GL_DECLARE
};

extern GLFuncs gGL;

// Loads every function of the list from the current context; false (with the missing name logged) when one is missing
bool LoadGLFuncs();

}  // namespace Sexy

#endif
