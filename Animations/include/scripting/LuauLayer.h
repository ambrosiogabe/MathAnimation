#ifndef MATH_ANIM_LUAU_LAYER_H
#define MATH_ANIM_LUAU_LAYER_H
#include "core.h"

namespace MathAnim
{
	struct AnimationManagerData;
	struct CodeEditorPanelData;

	namespace LuauLayer
	{
		void init(const std::filesystem::path& scriptDirectory, AnimationManagerData* am);

		void update(AnimationManagerData* am);

		bool compile(const std::string& filename);
		bool compile(const std::string& sourceCode, const std::string& scriptName);

		bool startDebugging(const std::string& filename, CodeEditorPanelData* editor);

		const std::string& getCurrentExecutingScriptFilepath();

		bool pushBytecode(const std::string& filename);
		bool executeBytecode();
		bool popBytecode();

		bool executeOnAnimObj(const std::string& functionName, AnimationManagerData* am, AnimObjId obj);
		bool debugGenerateAnimObj(const std::string& filename, AnimationManagerData* am, AnimObjId obj);

		bool execute(const std::string& filename);

		bool remove(const std::string& scriptName);

		void free();
	}
}

#endif 