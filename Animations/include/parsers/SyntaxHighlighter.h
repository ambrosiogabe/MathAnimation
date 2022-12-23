#ifndef MATH_ANIM_SYNTAX_HIGHLIGHTER_H
#define MATH_ANIM_SYNTAX_HIGHLIGHTER_H
#include "core.h"

namespace MathAnim
{
	struct SyntaxTheme;
	struct Grammar;

	struct Highlight
	{
		size_t startPos;
		size_t endPos;
		Vec4 color;
	};

	struct CodeHighlights
	{
		std::vector<Highlight> highlights;
		std::string codeBlock;
	};

	class SyntaxHighlighter
	{
	public:
		SyntaxHighlighter(const std::filesystem::path& grammar);

		CodeHighlights parse(const std::string& code, const SyntaxTheme& theme);

		void free();

	private:
		Grammar* grammar;
	};
}

#endif 