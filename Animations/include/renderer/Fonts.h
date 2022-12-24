#ifndef MATH_ANIM_FONTS_H
#define MATH_ANIM_FONTS_H
#include "core.h"
#include "renderer/Texture.h"

namespace MathAnim
{
	struct SvgObject;

	struct CharRange
	{
		uint32 firstCharCode;
		uint32 lastCharCode;

		static CharRange Ascii;
	};

	struct GlyphOutline
	{
		SvgObject* svg;
		float advanceX;
		float bearingX;
		float bearingY;
		float descentY;
		float glyphWidth;
		float glyphHeight;

		void free();
	};

	struct Font
	{
		FT_Face fontFace;
		std::unordered_map<uint32, GlyphOutline> glyphMap;
		std::string fontFilepath;
		float unitsPerEM;
		float lineHeight;

		const GlyphOutline& getGlyphInfo(uint32 glyphIndex) const;
		float getKerning(uint32 leftCodepoint, uint32 rightCodepoint) const;
		glm::vec2 getSizeOfString(const std::string& string) const;
		glm::vec2 getSizeOfString(const std::string& string, int fontSizePixels) const;
	};

	struct GlyphTexture
	{
		uint32 lruCacheId;
		Vec2 uvMin;
		Vec2 uvMax;
	};

	struct SizedFont
	{
		Font* unsizedFont;
		std::unordered_map<uint32, GlyphTexture> glyphTextureCoords;
		int fontSizePixels;
		Texture texture;

		const GlyphTexture& getGlyphTexture(uint32 codepoint) const;
		inline const GlyphOutline& getGlyphInfo(uint32 glyphIndex) const { g_logger_assert(unsizedFont != nullptr, "How did this happen."); return unsizedFont->getGlyphInfo(glyphIndex); }
		inline float getKerning(uint32 leftCodepoint, uint32 rightCodepoint) const { g_logger_assert(unsizedFont != nullptr, "How did this happen."); return unsizedFont->getKerning(leftCodepoint, rightCodepoint); }
		inline glm::vec2 getSizeOfString(const std::string& string) const { g_logger_assert(unsizedFont != nullptr, "How did this happen."); return unsizedFont->getSizeOfString(string, fontSizePixels); }
	};

	namespace Fonts
	{
		void init();

		// Returns a non-zero value if creating the outline fails
		int createOutline(Font* font, uint32 character, GlyphOutline* outlineResult);

		// Loads a sized font if it is not already loaded and creates
		// a texture with the default charset.
		// If the font is already loaded, it just increments
		// a reference count and returns the font.
		SizedFont* loadSizedFont(const char* filepath, int fontSizePixels, CharRange defaultCharset = CharRange::Ascii);

		// Decreases a reference count to the font
		// If the reference count goes below 0, the 
		// font is fully unloaded
		void unloadSizedFont(SizedFont* sizedFont);

		// Decreases a reference count to the font
		// If the reference count goes below 0, the 
		// font is fully unloaded
		void unloadSizedFont(const char* filepath, int fontSizePixels);

		// Decreases a reference count to the font
		// If the reference count goes below 0, the 
		// font is fully unloaded
		void unloadFont(const char* filepath);

		// Loads a font if it is not already loaded.
		// If the font is already loaded, it just increments
		// a reference count and returns the font.
		Font* loadFont(const char* filepath, CharRange defaultCharset = CharRange::Ascii);

		// Decreases a reference count to the font
		// If the reference count goes below 0, the 
		// font is fully unloaded
		void unloadFont(Font* font);

		// Decreases a reference count to the font
		// If the reference count goes below 0, the 
		// font is fully unloaded
		void unloadFont(const char* filepath);

		// Forcefully unloads all fonts irregardless
		// of their current reference counts
		void unloadAllFonts();

		Font* getDefaultMonoFont();
	}
}

#endif
