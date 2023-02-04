#include "core.h"
#include "renderer/GLApi.h"
#include "renderer/Texture.h"

namespace MathAnim
{
	namespace GL
	{
		static bool compatMode = true;
		static bool gl40Support = true;
		static bool gl43Support = true;
		static bool gl44Support = true;

		static constexpr int minSupportedVersionMajor = 3;
		static constexpr int minSupportedVersionMinor = 1;

		void init(int versionMajor, int versionMinor)
		{
			if (versionMajor < minSupportedVersionMajor || 
				(versionMajor >= minSupportedVersionMajor && versionMinor < minSupportedVersionMinor))
			{
				g_logger_error("You are running OpenGL version less than %d.%d. This app does not support GL versions less than %d.%d, you can try to update your graphics drivers to see if that helps.",
					minSupportedVersionMajor, minSupportedVersionMinor,
					minSupportedVersionMajor, minSupportedVersionMinor
				);
				g_logger_assert(false, "GL Version %d.%d not supported.", versionMajor, versionMinor);
			}

			if (versionMajor >= 4 && versionMinor >= 6)
			{
				compatMode = false;
			}
			else
			{
				if (versionMajor >= 4 && versionMinor < 4)
				{
					gl44Support = false;
				}

				if (versionMajor >= 4 && versionMinor < 3)
				{
					gl43Support = false;
				}

				if (versionMajor < 4)
				{
					gl40Support = false;
					gl43Support = false;
					gl44Support = false;
				}
				g_logger_warning("You are using an OpenGL version <= 4.6. This means it will run in compatibility mode. This means the app may render inappropriately in certain cases, and performance may be degraded. Please update your drivers if possible to the latest GL version.");
			}

			g_logger_info("Initialized GL wrapper with version %d.%d", versionMajor, versionMinor);
		}

		// Blending
		void blendFunc(GLenum sfactor, GLenum dfactor)
		{
			glBlendFunc(sfactor, dfactor);
		}

		void blendFunci(GLuint buf, GLenum src, GLenum dst)
		{
			if (gl40Support)
			{
				glBlendFunci(buf, src, dst);
			}
			else
			{
				// NOTE: This will really screw stuff up in OIT rendering
				// Come up with fallback for OIT
				glBlendFunc(src, dst);
			}
		}

		void blendEquation(GLenum mode)
		{
			glBlendEquation(mode);
		}

		void blendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
		{
			glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
		}

		// Framebuffers
		void bindFramebuffer(GLenum target, GLuint framebuffer)
		{
			glBindFramebuffer(target, framebuffer);
		}

		void bindRenderbuffer(GLenum target, GLuint renderbuffer)
		{
			glBindRenderbuffer(target, renderbuffer);
		}

		void readBuffer(GLenum src)
		{
			glReadBuffer(src);
		}

		void clearBufferiv(GLenum buffer, GLint drawbuffer, const GLint* value)
		{
			glClearBufferiv(buffer, drawbuffer, value);
		}

		void clearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint* value)
		{
			glClearBufferuiv(buffer, drawbuffer, value);
		}

		void clearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat* value)
		{
			glClearBufferfv(buffer, drawbuffer, value);
		}

		void clearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)
		{
			glClearBufferfi(buffer, drawbuffer, depth, stencil);
		}

		void deleteFramebuffers(GLsizei n, const GLuint* framebuffers)
		{
			glDeleteFramebuffers(n, framebuffers);
		}

		void deleteRenderbuffers(GLsizei n, const GLuint* renderbuffers)
		{
			glDeleteRenderbuffers(n, renderbuffers);
		}

		void genFramebuffers(GLsizei n, GLuint* framebuffers)
		{
			glGenFramebuffers(n, framebuffers);
		}

		void genRenderbuffers(GLsizei n, GLuint* renderbuffers)
		{
			glGenRenderbuffers(n, renderbuffers);
		}

		void drawBuffers(GLsizei n, const GLenum* bufs)
		{
			glDrawBuffers(n, bufs);
		}

		void framebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
		{
			glFramebufferTexture2D(target, attachment, textarget, texture, level);
		}

		void renderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
		{
			glRenderbufferStorage(target, internalformat, width, height);
		}

		void framebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
		{
			glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
		}

		GLenum checkFramebufferStatus(GLenum target)
		{
			return glCheckFramebufferStatus(target);
		}

		// Vaos
		void bindVertexArray(GLuint array)
		{
			glBindVertexArray(array);
		}

		void createVertexArray(GLuint* name)
		{
			glGenVertexArrays(1, name);
		}

		void vertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer)
		{
			glVertexAttribPointer(index, size, type, normalized, stride, pointer);
		}

		void vertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer)
		{
			glVertexAttribIPointer(index, size, type, stride, pointer);
		}

		void enableVertexAttribArray(GLuint index)
		{
			glEnableVertexAttribArray(index);
		}

		void deleteVertexArrays(GLsizei n, const GLuint* arrays)
		{
			glDeleteVertexArrays(n, arrays);
		}

		// Buffer objects
		void bindBuffer(GLenum target, GLuint buffer)
		{
			glBindBuffer(target, buffer);
		}

		void bufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage)
		{
			glBufferData(target, size, data, usage);
		}

		void bufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data)
		{
			glBufferSubData(target, offset, size, data);
		}

		void genBuffers(GLsizei n, GLuint* buffers)
		{
			glGenBuffers(n, buffers);
		}

		void deleteBuffers(GLsizei n, const GLuint* buffers)
		{
			glDeleteBuffers(n, buffers);
		}

		void* mapBuffer(GLenum target, GLenum access)
		{
			return glMapBuffer(target, access);
		}

		GLboolean unmapBuffer(GLenum target)
		{
			return glUnmapBuffer(target);
		}

		void scissor(GLint x, GLint y, GLsizei width, GLsizei height)
		{
			glScissor(x, y, width, height);
		}

		// Render functions
		void drawArrays(GLenum mode, GLint first, GLsizei count)
		{
			glDrawArrays(mode, first, count);
		}

		void drawElements(GLenum mode, GLsizei count, GLenum type, const void* indices)
		{
			glDrawElements(mode, count, type, indices);
		}

		void drawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void* indices, GLint basevertex)
		{
			glDrawElementsBaseVertex(mode, count, type, indices, basevertex);
		}

		// Textures
		void readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels)
		{
			glReadPixels(x, y, width, height, format, type, pixels);
		}

		void genTextures(GLsizei n, GLuint* textures)
		{
			glGenTextures(n, textures);
		}

		void activeTexture(GLenum texture)
		{
			glActiveTexture(texture);
		}

		void bindTexture(GLenum target, GLuint texture)
		{
			glBindTexture(target, texture);
		}

		void deleteTextures(GLsizei n, const GLuint* textures)
		{
			glDeleteTextures(n, textures);
		}

		void texImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels)
		{
			glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
		}

		void texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels)
		{
			glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
		}

		void texParameteri(GLenum target, GLenum pname, GLint param)
		{
			glTexParameteri(target, pname, param);
		}

		void texParameteriv(GLenum target, GLenum pname, const GLint* params)
		{
			glTexParameteriv(target, pname, params);
		}

		void pixelStorei(GLenum pname, GLint param)
		{
			glPixelStorei(pname, param);
		}

		// Shaders
		GLuint createProgram(void)
		{
			return glCreateProgram();
		}

		void useProgram(GLuint program)
		{
			glUseProgram(program);
		}

		void linkProgram(GLuint program)
		{
			glLinkProgram(program);
		}

		void deleteProgram(GLuint program)
		{
			glDeleteProgram(program);
		}

		void getProgramiv(GLuint program, GLenum pname, GLint* params)
		{
			glGetProgramiv(program, pname, params);
		}

		void getProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog)
		{
			glGetProgramInfoLog(program, bufSize, length, infoLog);
		}

		GLuint createShader(GLenum type)
		{
			return glCreateShader(type);
		}

		void attachShader(GLuint program, GLuint shader)
		{
			glAttachShader(program, shader);
		}

		void detachShader(GLuint program, GLuint shader)
		{
			glDetachShader(program, shader);
		}

		void deleteShader(GLuint shader)
		{
			glDeleteShader(shader);
		}

		void shaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length)
		{
			glShaderSource(shader, count, string, length);
		}

		void compileShader(GLuint shader)
		{
			glCompileShader(shader);
		}

		void getShaderiv(GLuint shader, GLenum pname, GLint* params)
		{
			glGetShaderiv(shader, pname, params);
		}

		void getShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog)
		{
			glGetShaderInfoLog(shader, bufSize, length, infoLog);
		}

		void getActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name)
		{
			glGetActiveUniform(program, index, bufSize, length, size, type, name);
		}

		GLint getUniformLocation(GLuint program, const GLchar* name)
		{
			return glGetUniformLocation(program, name);
		}

		GLint getAttribLocation(GLuint program, const GLchar* name)
		{
			return glGetAttribLocation(program, name);
		}

		void uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
		{
			glUniform4f(location, v0, v1, v2, v3);
		}

		void uniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
		{
			glUniform3f(location, v0, v1, v2);
		}

		void uniform2f(GLint location, GLfloat v0, GLfloat v1)
		{
			glUniform2f(location, v0, v1);
		}

		void uniform1f(GLint location, GLfloat v0)
		{
			glUniform1f(location, v0);
		}

		void uniform1iv(GLint location, GLsizei count, const GLint* value)
		{
			glUniform1iv(location, count, value);
		}

		void uniform1i(GLint location, GLint v0)
		{
			glUniform1i(location, v0);
		}

		void uniform2ui(GLint location, GLuint v0, GLuint v1)
		{
			glUniform2ui(location, v0, v1);
		}

		void uniform1ui(GLint location, GLuint v0)
		{
			glUniform1ui(location, v0);
		}

		void uniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
		{
			glUniformMatrix4fv(location, count, transpose, value);
		}

		void uniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
		{
			glUniformMatrix3fv(location, count, transpose, value);
		}

		// Basic functions
		void enable(GLenum cap)
		{
			glEnable(cap);
		}

		void disable(GLenum cap)
		{
			glDisable(cap);
		}

		void clearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
		{
			glClearColor(red, green, blue, alpha);
		}

		void clear(GLbitfield mask)
		{
			glClear(mask);
		}

		void depthMask(GLboolean flag)
		{
			glDepthMask(flag);
		}

		void viewport(GLint x, GLint y, GLsizei width, GLsizei height)
		{
			glViewport(x, y, width, height);
		}

		void lineWidth(GLfloat width)
		{
			glLineWidth(width);
		}

		void polygonMode(GLenum face, GLenum mode)
		{
			glPolygonMode(face, mode);
		}

		void getIntegerv(GLenum pname, GLint* data)
		{
			glGetIntegerv(pname, data);
		}

		const GLubyte* getStringi(GLenum name, GLuint index)
		{
			return glGetStringi(name, index);
		}

		// Debug callback stuffs
		void debugMessageCallback(GLDEBUGPROC callback, const void* userParam)
		{
			if (gl43Support)
			{
				glDebugMessageCallback(callback, userParam);
			}
			else
			{
				g_logger_warning("User system does not support GL 4.3 or greater. No debug message callback will be installed.");
			}
		}

		void pushDebugGroup(GLenum source, GLuint id, GLsizei length, const GLchar* message)
		{
			if (gl43Support)
			{
				glPushDebugGroup(source, id, length, message);
			}
		}

		void popDebugGroup(void)
		{
			if (gl43Support)
			{
				glPopDebugGroup();
			}
		}

		GLenum getError(void)
		{
			return glGetError();
		}
	}
}