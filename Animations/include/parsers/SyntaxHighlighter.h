#ifndef MATH_ANIM_SYNTAX_HIGHLIGHTER_H
#define MATH_ANIM_SYNTAX_HIGHLIGHTER_H
#include "core.h"

namespace MathAnim
{
	struct SyntaxTheme;
	struct Grammar;

	enum class HighlighterLanguage : uint8
	{
		None,
		Cpp,
	};

	enum class HighlighterTheme : uint8
	{
		Monokai
	};

	// Styles a text segment from
	//   startPos <= text < endPos
	struct HighlightSegment
	{
		size_t startPos;
		size_t endPos;
		Vec4 color;
	};

	struct CodeHighlights
	{
		std::vector<HighlightSegment> segments;
		std::string codeBlock;
	};

	class SyntaxHighlighter
	{
	public:
		SyntaxHighlighter(const std::filesystem::path& grammar);

		CodeHighlights parse(const std::string& code, const SyntaxTheme& theme, bool printDebugInfo = false) const;

		void free();

	private:
		Grammar* grammar;
	};

	namespace Highlighters
	{
		void init();

		const SyntaxHighlighter* getHighlighter(HighlighterLanguage language);
		const SyntaxTheme* getTheme(HighlighterTheme theme);

		void free();
	}
}

#endif 