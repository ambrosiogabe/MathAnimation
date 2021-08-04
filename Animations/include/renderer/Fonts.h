#ifndef MATH_ANIM_FONTS_H
#define MATH_ANIM_FONTS_H
#include "core.h"
#include "renderer/Texture.h"

namespace MathAnim
{
	typedef uint32 FontSize;

	struct Font
	{
		FT_Face fontFace;
		Texture texture;
		FontSize fontSize;
	};

	namespace Fonts
	{
		void init();

		Font* loadFont(const char* filepath, FontSize fontSize);
		void unloadFont(Font* font);
		void unloadFont(const char* filepath, FontSize fontSize);

		Font* getFont(const char* filepath, FontSize fontSize);
	}

	FontSize operator""_px(uint64 numPixels);
	FontSize operator""_em(long double emSize);
}

#endif
