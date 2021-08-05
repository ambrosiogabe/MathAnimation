#ifndef MATH_ANIM_FONTS_H
#define MATH_ANIM_FONTS_H
#include "core.h"
#include "renderer/Texture.h"

namespace MathAnim
{
	typedef uint32 FontSize;

	struct RenderableChar
	{
		glm::vec2 texCoordStart;
		glm::vec2 texCoordSize;
		glm::vec2 advance;
	};

	struct CharRange
	{
		uint32 firstCharCode;
		uint32 lastCharCode;

		static CharRange Ascii;
	};

	struct Font
	{
		FT_Face fontFace;
		Texture texture;
		FontSize fontSize;
		std::unordered_map<char, RenderableChar> characterMap;

		const RenderableChar& getCharInfo(char c) const;
		float getKerning(char leftChar, char rightChar) const;
	};

	namespace Fonts
	{
		void init();

		Font* loadFont(const char* filepath, FontSize fontSize, CharRange defaultCharset = CharRange::Ascii);
		void unloadFont(Font* font);
		void unloadFont(const char* filepath, FontSize fontSize);

		Font* getFont(const char* filepath, FontSize fontSize);
	}

	FontSize operator""_px(uint64 numPixels);
	FontSize operator""_em(long double emSize);
}

#endif
