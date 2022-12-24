#ifndef MATH_ANIM_SYNTAX_HIGHLIGHTER_H
#define MATH_ANIM_SYNTAX_HIGHLIGHTER_H
#include "core.h"

namespace MathAnim
{
	struct SyntaxTheme;
	struct Grammar;

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

		CodeHighlights parse(const std::string& code, const SyntaxTheme& theme, bool printDebugInfo = false);

		void free();

	private:
		Grammar* grammar;
	};
}

#endif 