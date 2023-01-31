#ifndef MATH_ANIM_GL_API_H
#define MATH_ANIM_GL_API_H
#include "core.h"

namespace MathAnim
{
	struct Texture;

	namespace GL
	{
		void init(int versionMajor, int versionMinor);

		// Blending
		void blendFunc(GLenum sfactor, GLenum dfactor);
		void blendFunci(GLuint buf, GLenum src, GLenum dst);
		void blendEquation(GLenum mode);
		void blendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);

		// Framebuffers
		void bindFramebuffer(GLenum target, GLuint framebuffer);
		void bindRenderbuffer(GLenum target, GLuint renderbuffer);
		void readBuffer(GLenum src);
		void clearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat* value);
		void clearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);
		void deleteFramebuffers(GLsizei n, const GLuint* framebuffers);
		void deleteRenderbuffers(GLsizei n, const GLuint* renderbuffers);
		void genFramebuffers(GLsizei n, GLuint* framebuffers);
		void genRenderbuffers(GLsizei n, GLuint* renderbuffers);
		void drawBuffers(GLsizei n, const GLenum* bufs);
		void framebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
		void renderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
		void framebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
		GLenum checkFramebufferStatus(GLenum target);

		// Vaos
		void bindVertexArray(GLuint array);
		void createVertexArray(GLuint* name);
		void vertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
		void vertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer);
		void enableVertexAttribArray(GLuint index);
		void deleteVertexArrays(GLsizei n, const GLuint* arrays);

		// Buffer objects
		void bindBuffer(GLenum target, GLuint buffer);
		void bufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
		void bufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data);
		void genBuffers(GLsizei n, GLuint* buffers);
		void deleteBuffers(GLsizei n, const GLuint* buffers);
		void* mapBuffer(GLenum target, GLenum access);
		GLboolean unmapBuffer(GLenum target);

		// Scissor stuff
		void scissor(GLint x, GLint y, GLsizei width, GLsizei height);

		// Render functions
		void drawArrays(GLenum mode, GLint first, GLsizei count);
		void drawElements(GLenum mode, GLsizei count, GLenum type, const void* indices);
		void drawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void* indices, GLint basevertex);

		// Textures
		void clearTexImage(const Texture& texture, GLint level, const void* data, size_t dataLength);
		void readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels);
		void genTextures(GLsizei n, GLuint* textures);
		void activeTexture(GLenum texture);
		void bindTexture(GLenum target, GLuint texture);
		void deleteTextures(GLsizei n, const GLuint* textures);
		void texImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);
		void texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels);
		void texParameteri(GLenum target, GLenum pname, GLint param);
		void texParameteriv(GLenum target, GLenum pname, const GLint* params);
		void pixelStorei(GLenum pname, GLint param);

		// Shaders
		GLuint createProgram(void);
		void useProgram(GLuint program);
		void linkProgram(GLuint program);
		void deleteProgram(GLuint program);
		void getProgramiv(GLuint program, GLenum pname, GLint* params);
		void getProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
		GLuint createShader(GLenum type);
		void attachShader(GLuint program, GLuint shader);
		void detachShader(GLuint program, GLuint shader);
		void deleteShader(GLuint shader);
		void shaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
		void compileShader(GLuint shader);
		void getShaderiv(GLuint shader, GLenum pname, GLint* params);
		void getShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
		void getActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
		GLint getUniformLocation(GLuint program, const GLchar* name);
		GLint getAttribLocation(GLuint program, const GLchar* name);
		void uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
		void uniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
		void uniform2f(GLint location, GLfloat v0, GLfloat v1);
		void uniform1f(GLint location, GLfloat v0);
		void uniform1iv(GLint location, GLsizei count, const GLint* value);
		void uniform1i(GLint location, GLint v0);
		void uniform2ui(GLint location, GLuint v0, GLuint v1);
		void uniform1ui(GLint location, GLuint v0);
		void uniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
		void uniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);

		// Basic functions
		void enable(GLenum cap);
		void disable(GLenum cap);
		void clearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
		void clear(GLbitfield mask);
		void depthMask(GLboolean flag);
		void viewport(GLint x, GLint y, GLsizei width, GLsizei height);
		void lineWidth(GLfloat width);
		void polygonMode(GLenum face, GLenum mode);
		void getIntegerv(GLenum pname, GLint* data);
		const GLubyte* getStringi(GLenum name, GLuint index);

		// Debug callback stuffs
		void debugMessageCallback(GLDEBUGPROC callback, const void* userParam);
		void pushDebugGroup(GLenum source, GLuint id, GLsizei length, const GLchar* message);
		void popDebugGroup(void);
		GLenum getError(void);
	}
}

#endif 