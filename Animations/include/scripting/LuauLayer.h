#ifndef MATH_ANIM_LUAU_LAYER_H
#define MATH_ANIM_LUAU_LAYER_H
#include "core.h"

namespace MathAnim
{
	struct AnimationManagerData;
	struct CodeEditorPanelData;

	struct Bytecode
	{
		std::string scriptFilepath;
		char* bytes;
		size_t size;
		bool isValid;
	};

	namespace LuauLayer
	{
		void init(const std::filesystem::path& scriptDirectory, AnimationManagerData* am);

		void update(AnimationManagerData* am);

		bool compile(const std::string& filename);
		bool compile(const std::string& sourceCode, const std::string& scriptName);

		/**
		* @brief Tries to get cached bytecode, if the bytecode is not cached attempts
		*        to compile and return the bytecode. If compilation fails, the bytecode
		*        sets `isValid` to false
		* 
		* @param filename The filename of the script you'd like to compile
		* @returns The compiled bytecode, or an empty object if compilation fails
		*/
		Bytecode getBytecode(const std::string& filename);

		bool startDebugging(const std::string& filename, CodeEditorPanelData* editor);

		const std::string& getCurrentExecutingScriptFilepath();

		bool pushBytecode(const std::string& filename);
		bool executeBytecode();
		bool popBytecode();

		bool executeOnAnimate(const std::string& functionName, AnimationManagerData* am, AnimObjId obj, float t);
		bool executeOnAnimObj(const std::string& functionName, AnimationManagerData* am, AnimObjId obj);
		bool executeFn(const std::string& functionName);
		bool debugGenerateAnimObj(const std::string& filename, AnimationManagerData* am, AnimObjId obj);

		bool execute(const std::string& filename);

		bool remove(const std::string& scriptName);

		void free();
	}
}

#endif 