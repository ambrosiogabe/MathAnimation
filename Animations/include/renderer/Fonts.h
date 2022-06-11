#ifndef MATH_ANIM_FONTS_H
#define MATH_ANIM_FONTS_H
#include "core.h"

struct NVGcontext;

namespace MathAnim
{
	struct CharRange
	{
		uint32 firstCharCode;
		uint32 lastCharCode;

		static CharRange Ascii;
	};

	struct GlyphVertex
	{
		Vec2 position;
		bool controlPoint;
	};

	struct GlyphContour
	{
		GlyphVertex* vertices;
		int numVertices;
		int numCurves;
	};

	struct GlyphOutline
	{
		GlyphContour* contours;
		int numContours;
		float totalCurveLengthApprox;
		float advanceX;
		float bearingX;
		float bearingY;
		float descentY;

		void free();
	};

	struct Font
	{
		FT_Face fontFace;
		std::unordered_map<uint32, GlyphOutline> glyphMap;
		std::string vgFontFace;
		float unitsPerEM;
		float lineHeight;

		const GlyphOutline& getGlyphInfo(uint32 glyphIndex) const;
		float getKerning(uint32 leftCodepoint, uint32 rightCodepoint) const;
		glm::vec2 getSizeOfString(const std::string& string) const;
		glm::vec2 getSizeOfString(const std::string& string, int fontSizePixels) const;
	};

	namespace Fonts
	{
		void init();

		// Returns a non-zero value if creating the outline fails
		int createOutline(Font* font, uint32 character, GlyphOutline* outlineResult);

		Font* loadFont(const char* filepath, NVGcontext* vg, CharRange defaultCharset = CharRange::Ascii);
		void unloadFont(Font* font);
		void unloadFont(const char* filepath);
		void unloadAllFonts();

		Font* getFont(const char* filepath);
	}
}

#endif
