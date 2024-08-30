#ifndef MATH_ANIM_SCRIPT_ANALYZER
#define MATH_ANIM_SCRIPT_ANALYZER
#include "core.h"

#pragma warning( push )
#pragma warning( disable : 4100 )
#pragma warning( disable : 4324 )
#pragma warning( disable : 4324 )
#include <Luau/Frontend.h>
#pragma warning( pop )

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

		std::optional<Luau::SourceCode> resolveFile(const std::string& filename);

		void free();

	private:
		const std::filesystem::path m_scriptDirectory;
		Luau::FileResolver* fileResolver;
		Luau::ConfigResolver* configResolver;
		Luau::Frontend* frontend;
	};
}

#endif 