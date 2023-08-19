#ifndef MATH_ANIM_SHADER_H
#define MATH_ANIM_SHADER_H
#include "core.h"

typedef unsigned int GLuint;

namespace MathAnim
{
	struct Shader
	{
		uint32 programId;
		uint32 startIndex;
		std::filesystem::path filepath;

		void compile(const std::filesystem::path& shaderFilepath);
		void compileRaw(const std::string& rawSource);
		void bind() const;
		void unbind() const;
		void destroy();

		void uploadVec4(const char* varName, const glm::vec4& vec4) const;
		void uploadVec3(const char* varName, const glm::vec3& vec3) const;
		void uploadVec2(const char* varName, const glm::vec2& vec2) const;
		inline void uploadVec4(const char* varName, const Vec4& vec4) const { uploadVec4(varName, glm::vec4(vec4.r, vec4.g, vec4.b, vec4.a)); }
		inline void uploadVec3(const char* varName, const Vec3& vec3) const { uploadVec3(varName, glm::vec3(vec3.r, vec3.g, vec3.b)); }
		inline void uploadVec2(const char* varName, const Vec2& vec2) const { uploadVec2(varName, glm::vec2(vec2.x, vec2.y)); }
		void uploadFloat(const char* varName, float value) const;
		void uploadInt(const char* varName, int value) const;
		void uploadIntArray(const char* varName, int length, const int* array) const;
		void uploadUInt(const char* varName, uint32 value) const;
		void uploadUVec2(const char* varName, const glm::uvec2& vec2) const;
		void uploadU64AsUVec2(const char* varName, uint64 value) const;

		void uploadMat4(const char* varName, const glm::mat4& mat4) const;
		void uploadMat3(const char* varName, const glm::mat3& mat3) const;

		bool isNull() const;
	};
}

#endif
