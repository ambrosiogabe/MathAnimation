#ifndef MATH_ANIM_SCRIPT_ANALYZER
#define MATH_ANIM_SCRIPT_ANALYZER
#include "core.h"

namespace Luau
{
	struct FileResolver;
	struct ConfigResolver;
	struct Frontend;
}

namespace MathAnim
{
	class ScriptAnalyzer
	{
	public:
		ScriptAnalyzer(const std::filesystem::path& scriptDirectory);

		bool analyze(const std::string& filename);
		bool analyze(const std::string& sourceCode, const std::string& scriptName);

		void free();

	private:
		const std::filesystem::path m_scriptDirectory;
		Luau::FileResolver* fileResolver;
		Luau::ConfigResolver* configResolver;
		Luau::Frontend* frontend;
	};
}

#endif 