#include "renderer/Fonts.h"
#include "renderer/Renderer.h"
#include "core/Application.h"
#include "svg/Svg.h"
#include "utils/CMath.h"

#include <freetype/ftglyph.h>
#include <freetype/freetype.h>
#include <freetype/ftoutln.h>
#include <freetype/ftbbox.h>

enum class PointType : uint8
{
	None,
	CurveTagOn,
	CurveTagConic,
	CurveTagCubic
};

enum class DropoutMode : uint8
{
	None,
	Undefined
};

inline PointType FT_getPointType(char flag)
{
	// These interpretations can be found here:
	// https://freetype.org/freetype2/docs/reference/ft2-outline_processing.html#ft_outline
	/*
	* If bit 0 is unset, the point is ‘off’ the curve, i.e., a Bezier control point,
	* while it is ‘on’ if set.
	*
	* Bit 1 is meaningful for ‘off’ points only.
	* If set, it indicates a third-order Bezier arc control point;
	* and a second-order control point if unset.
	*/
	bool bit0Set = (flag & 0x1);
	bool bit1Set = (flag & 0x2);

	if (bit0Set)
	{
		return PointType::CurveTagOn;
	}

	// Bit 0 is set at this point
	if (bit1Set)
	{
		return PointType::CurveTagCubic;
	}

	return PointType::CurveTagConic;
}

inline DropoutMode FT_getDropoutMode(char flag)
{
	// These interpretations can be found here:
	// https://freetype.org/freetype2/docs/reference/ft2-outline_processing.html#ft_outline
	/*
	* If bit 2 is set, bits 5-7 contain the drop-out mode
	* (as defined in the OpenType specification; the value is the same as the argument to the ‘SCANMODE’ instruction).
	*
	* Bits 3 and 4 are reserved for internal purposes.
	*/
	bool bit2Set = (flag & 0x4);
	if (!bit2Set)
	{
		return DropoutMode::None;
	}

	return DropoutMode::Undefined;
}

inline MathAnim::Vec2 FT_Vector_ToVec2(const FT_Vector& vec, float glyphLeft, float descentY, float upem, float glyphHeight)
{
	float pointx = ((float)(vec.x - glyphLeft) / upem);
	float pointy = ((float)(vec.y + descentY) / upem);
	// Flip the point y since the glyph is "upside down"
	pointy = glyphHeight - pointy;
	return MathAnim::Vec2{
		pointx,
		pointy
	};
}

namespace MathAnim
{
	CharRange CharRange::Ascii = { 32, 126 };

	struct SharedFont
	{
		Font font;
		int referenceCount;
	};

	struct SharedSizedFont
	{
		SizedFont font;
		int referenceCount;
	};

	const GlyphOutline& Font::getGlyphInfo(uint32 glyphIndex) const
	{
		auto iter = glyphMap.find(glyphIndex);
		if (iter != glyphMap.end())
		{
			return iter->second;
		}

		g_logger_error("Glyph index '%d' does not exist in font '%s'.", glyphIndex, fontFilepath.c_str());
		static GlyphOutline defaultGlyph = {};
		return defaultGlyph;
	}

	float Font::getKerning(uint32 leftCodepoint, uint32 rightCodepoint) const
	{
		FT_Vector kerning;

		FT_UInt leftGlyph = FT_Get_Char_Index(fontFace, leftCodepoint);
		FT_UInt rightGlyph = FT_Get_Char_Index(fontFace, rightCodepoint);
		int error = FT_Get_Kerning(fontFace, leftGlyph, rightGlyph, FT_Kerning_Mode::FT_KERNING_DEFAULT, &kerning);
		if (error)
		{
			return 0.0f;
		}

		return (float)kerning.x / unitsPerEM;
	}

	glm::vec2 Font::getSizeOfString(const std::string& string) const
	{
		glm::vec2 cursor = glm::vec2();
		for (int i = 0; i < string.length(); i++)
		{
			const GlyphOutline& outline = getGlyphInfo((uint32)string[i]);
			cursor.x += outline.advanceX;

			if (string[i] == '\n')
			{
				cursor.y += lineHeight;
			}
		}

		cursor.y += lineHeight;
		return cursor;
	}

	glm::vec2 Font::getSizeOfString(const std::string& string, int fontSizePixels) const
	{
		return getSizeOfString(string) * (float)fontSizePixels;
	}

	void GlyphOutline::free()
	{
		if (svg)
		{
			svg->free();
			g_memory_free(svg);
			svg = nullptr;
		}

		advanceX = 0.0f;
		bearingX = 0.0f;
		descentY = 0.0f;
	}

	const GlyphTexture& SizedFont::getGlyphTexture(uint32 codepoint) const
	{
		if (this->glyphTextureCoords.find(codepoint) == this->glyphTextureCoords.end())
		{
			g_logger_warning("Trying to access glyph texture for unloaded glyph: '%c' in font '%s'.", codepoint, this->unsizedFont->fontFilepath.c_str());
			static GlyphTexture nullTexture = {
				0,
				Vec2{0, 0},
				Vec2{0, 0}
			};
			return nullTexture;
		}

		return this->glyphTextureCoords.at(codepoint);
	}

	namespace Fonts
	{
		static bool initialized = false;
		static const int hzPadding = 2;
		static const int vtPadding = 2;
		static FT_Library library;
		static Font* defaultMonoFont = nullptr;
		static std::unordered_map<std::string, SharedFont> loadedFonts;
		static std::unordered_map<std::string, SharedSizedFont> loadedSizedFonts;

		static void generateDefaultCharset(Font& font, CharRange defaultCharset);
		static int getOutline(FT_Glyph glyph, FT_OutlineGlyph* Outg);
		static GlyphOutline createOutlineInternal(FT_OutlineGlyph outlineGlyph, FT_Face face);
		static std::string getSizedFontKey(const char* filepath, int fontSizePixels);
		static std::string getUnsizedFontKey(const char* filepath);

		void init()
		{
			int error = FT_Init_FreeType(&library);
			if (error)
			{
				g_logger_error("An error occurred during freetype initialization. Font rendering will not work.");
			}

			g_logger_info("Initialized freetype library.");
			initialized = true;

#if defined(_WIN32)
			defaultMonoFont = loadFont("C:\\Windows\\Fonts\\consola.ttf");
#elif defined(__linux__)
			defaultMonoFont = loadFont("/usr/share/fonts/liberation/LiberationMono-Regular.ttf");
#endif
		}

		int createOutline(Font* font, uint32 character, GlyphOutline* outlineResult)
		{
			g_logger_assert(font->fontFace != nullptr, "Cannot create outline for uninitialized font '%s'.", font->fontFilepath.c_str());

			FT_Face fontFace = font->fontFace;
			FT_UInt glyphIndex = FT_Get_Char_Index(fontFace, character);
			if (glyphIndex == 0)
			{
				g_logger_warning("Character code '%c' not found. Missing glyph.", character);
				return 1;
			}

			// Load the glyph
			FT_Error error = FT_Load_Glyph(fontFace, glyphIndex, FT_LOAD_NO_SCALE);
			if (error)
			{
				g_logger_error("Freetype could not load glyph for character code '%c'.", character);
				return 2;
			}

			FT_Glyph glyph;
			FT_Get_Glyph(fontFace->glyph, &glyph);
			FT_OutlineGlyph outline;

			error = getOutline(glyph, &outline);
			if (error)
			{
				g_logger_error("Could not get outline for '%c'.", character);
				return 3;
			}

			*outlineResult = createOutlineInternal(outline, fontFace);
			FT_Done_Glyph(glyph);

			return 0;
		}

		SizedFont* loadSizedFont(const char* filepath, int fontSizePixels, CharRange defaultCharset)
		{
			// Check if the sized font is already loaded
			std::string sizedFontKey = getSizedFontKey(filepath, fontSizePixels);
			{
				auto iter = loadedSizedFonts.find(sizedFontKey);
				if (iter != loadedSizedFonts.end())
				{
					// Increment reference count and return
					iter->second.referenceCount++;
					return &iter->second.font;
				}
			}

			g_logger_info("Caching sized font '%s'.", sizedFontKey.c_str());

			SizedFont res;
			res.unsizedFont = loadFont(filepath, defaultCharset);
			res.fontSizePixels = fontSizePixels;

			{
				FT_Error error = FT_Set_Pixel_Sizes(res.unsizedFont->fontFace, fontSizePixels, fontSizePixels);
				if (error)
				{
					g_logger_error("Freetype failed to set the pixel size for font '%s'.", filepath);
					unloadFont(res.unsizedFont);
					return nullptr;
				}
			}

			constexpr uint32 textureWidth = 2048;
			constexpr uint32 textureHeight = 2048;
			res.texture = TextureBuilder()
				.setFormat(ByteFormat::R8_UI)
				.setWidth(textureWidth)
				.setHeight(textureHeight)
				.setMagFilter(FilterMode::Linear)
				.setMinFilter(FilterMode::Linear)
				.setWrapS(WrapMode::None)
				.setWrapT(WrapMode::None)
				.generate();

			// Generate the texture and upload it to the GPU
			uint8* textureMemory = (uint8*)g_memory_allocate(sizeof(uint8) * textureWidth * textureHeight);
			g_memory_zeroMem(textureMemory, sizeof(uint8) * textureWidth * textureHeight);
			uint32 cursorX = 0;
			uint32 cursorY = 0;
			uint32 lineHeight = 0;
			for (uint32 codepoint = defaultCharset.firstCharCode; codepoint <= defaultCharset.lastCharCode; codepoint++)
			{
				FT_UInt glyphIndex = FT_Get_Char_Index(res.unsizedFont->fontFace, codepoint);
				if (glyphIndex == 0)
				{
					g_logger_warning("Character code '%c' not found. Missing glyph.", codepoint);
					continue;
				}

				// Load the glyph
				FT_Error error = FT_Load_Glyph(res.unsizedFont->fontFace, glyphIndex, FT_LOAD_RENDER);
				if (error)
				{
					g_logger_error("Freetype could not load glyph for character code '%c'.", codepoint);
				}

				FT_Bitmap& bitmap = res.unsizedFont->fontFace->glyph->bitmap;
				if (cursorX + bitmap.width >= textureWidth)
				{
					cursorY += lineHeight;
					lineHeight = 0;
					cursorX = 0;
				}
				lineHeight = glm::max(bitmap.rows, lineHeight);

				if (cursorY + bitmap.rows >= textureHeight)
				{
					g_logger_error("Ran out of texture room for font '%s'", filepath);
					continue;
				}

				// Copy every row into our bitmap
				for (uint32 y = 0; y < bitmap.rows; y++)
				{
					uint8* dst = textureMemory + cursorX + ((cursorY + y) * textureWidth);
					uint8* src = bitmap.buffer + (y * bitmap.width);
					g_memory_copyMem(dst, src, sizeof(uint8) * bitmap.width);
				}

				// Add normalized glyph position
				res.glyphTextureCoords[codepoint].lruCacheId = 0;
				res.glyphTextureCoords[codepoint].uvMin = Vec2{
					(float)cursorX / (float)textureWidth,
					(float)cursorY / (float)textureHeight
				};
				res.glyphTextureCoords[codepoint].uvMax = Vec2{
					(float)cursorX / (float)textureWidth + (float)bitmap.width / (float)textureWidth,
					(float)cursorY / (float)textureHeight + (float)bitmap.rows / (float)textureHeight
				};

				// Increment our write cursor
				cursorX += bitmap.width;
			}

			// Upload texture memory to the GPU
			res.texture.uploadSubImage(0, 0, textureWidth, textureHeight, textureMemory, sizeof(uint8) * textureWidth * textureHeight);

			g_memory_free(textureMemory);

			// All done now cache the result and return it
			loadedSizedFonts[sizedFontKey].font = res;
			loadedSizedFonts[sizedFontKey].referenceCount = 1;

			return &loadedSizedFonts[sizedFontKey].font;
		}

		void unloadSizedFont(SizedFont* sizedFont)
		{
			if (!sizedFont)
			{
				g_logger_error("Tried to unload sized font that was nullptr.");
				return;
			}

			if (!sizedFont->unsizedFont)
			{
				g_logger_error("Tried to unload a sized font that had an unsized font that was nullptr.");
				return;
			}

			std::string fontKey = getSizedFontKey(sizedFont->unsizedFont->fontFilepath.c_str(), sizedFont->fontSizePixels);
			auto iter = loadedSizedFonts.find(fontKey);
			if (iter == loadedSizedFonts.end())
			{
				g_logger_warning("Tried to unload sized font '%s' that was not cached.", fontKey.c_str());
				return;
			}

			SizedFont* font = &iter->second.font;
			iter->second.referenceCount--;
			if (iter->second.referenceCount <= 0)
			{
				g_logger_info("Unloading sized font '%s' from cache.", fontKey.c_str());
			}

			unloadFont(font->unsizedFont);
			if (iter->second.referenceCount <= 0)
			{
				// Really unload the font now
				font->texture.destroy();
				loadedSizedFonts.erase(iter);
			}
		}

		void unloadSizedFont(const char* filepath, int fontSizePixels)
		{
			std::string fontKey = getSizedFontKey(filepath, fontSizePixels);
			auto iter = loadedSizedFonts.find(fontKey);
			if (iter == loadedSizedFonts.end())
			{
				g_logger_warning("Tried to unload sized font that was not cached '%s'.", fontKey.c_str());
				return;
			}

			SizedFont* font = &iter->second.font;
			unloadSizedFont(font);
		}

		Font* loadFont(const char* filepath, CharRange defaultCharset)
		{
			g_logger_assert(initialized, "Font library must be initialized to load a font.");

			std::string unsizedFontKey = getUnsizedFontKey(filepath);
			{
				auto iter = loadedFonts.find(unsizedFontKey);
				if (iter != loadedFonts.end())
				{
					iter->second.referenceCount++;
					return &iter->second.font;
				}
			}

			g_logger_info("Caching unsized font '%s'.", unsizedFontKey.c_str());

			// Load the new font into freetype.
			FT_Face face;
			int error = FT_New_Face(library, filepath, 0, &face);
			if (error == FT_Err_Unknown_File_Format)
			{
				g_logger_error("Unsupported font file format for '%s'. Could not load font.", filepath);
				return {};
			}
			else if (error)
			{
				g_logger_error("Font could not be opened or read or is broken '%s'. Could not load font.", filepath);
				return {};
			}

			// Generate a texture for the font and initialize the font structure
			Font font;
			font.fontFilepath = filepath;
			font.fontFace = face;
			font.unitsPerEM = (float)face->units_per_EM;
			font.lineHeight = (float)face->height / font.unitsPerEM;

			// TODO: Turn the preset characters into a parameter
			generateDefaultCharset(font, defaultCharset);

			loadedFonts[unsizedFontKey].font = font;
			loadedFonts[unsizedFontKey].referenceCount = 1;

			return &loadedFonts[unsizedFontKey].font;
		}

		void unloadFont(Font* font)
		{
			if (!font)
			{
				g_logger_warning("Tried to unload font that was nullptr.");
				return;
			}

			std::string unsizedFontKey = getUnsizedFontKey(font->fontFilepath.c_str());
			auto fontIter = loadedFonts.find(unsizedFontKey);
			if (fontIter == loadedFonts.end())
			{
				g_logger_error("Tried to unload font that was never cached '%s'", font->fontFilepath.c_str());
				return;
			}

			fontIter->second.referenceCount--;
			if (fontIter->second.referenceCount > 0)
			{
				return;
			}

			// If the reference count is <= 0 then really unload it
			g_logger_info("Unloading font '%s' from cache.", unsizedFontKey.c_str());

			for (std::pair<const uint32, GlyphOutline>& kv : font->glyphMap)
			{
				GlyphOutline& outline = kv.second;
				outline.free();
			}

			FT_Done_Face(font->fontFace);
			font->fontFace = nullptr;
			loadedFonts.erase(fontIter);
		}

		void unloadFont(const char* filepath)
		{
			std::string unsizedFontKey = getUnsizedFontKey(filepath);
			auto iter = loadedFonts.find(unsizedFontKey);
			if (iter == loadedFonts.end())
			{
				g_logger_warning("Tried to load font that was never cached '%s'.", filepath);
				return;
			}

			unloadFont(&iter->second.font);
		}

		void unloadAllFonts()
		{
			unloadFont(defaultMonoFont);
			defaultMonoFont = nullptr;

			// Delete all unsized fonts
			for (auto iter = loadedFonts.begin(); iter != loadedFonts.end();)
			{
				Font& font = iter->second.font;
				for (std::pair<const uint32, GlyphOutline>& kv : font.glyphMap)
				{
					GlyphOutline& outline = kv.second;
					outline.free();
				}

				FT_Done_Face(font.fontFace);
				font.fontFace = nullptr;
				iter->second.referenceCount = 0;

				iter = loadedFonts.erase(iter);
			}

			// Delete all sized fonts
			for (auto iter = loadedSizedFonts.begin(); iter != loadedSizedFonts.end();)
			{
				SizedFont& font = iter->second.font;
				font.texture.destroy();
				iter->second.referenceCount = 0;
				iter = loadedSizedFonts.erase(iter);
			}
		}

		Font* getDefaultMonoFont()
		{
			return defaultMonoFont;
		}

		static void generateDefaultCharset(Font& font, CharRange defaultCharset)
		{
			for (uint32 i = defaultCharset.firstCharCode; i <= defaultCharset.lastCharCode; i++)
			{
				GlyphOutline outlineResult;
				int error = createOutline(&font, i, &outlineResult);
				if (error)
				{
					g_logger_warning("Could not create glyph outline for character '%c'", i);
					continue;
				}

				font.glyphMap[i] = outlineResult;
			}
		}

		//******************* check error code ********************
		void Check(FT_Error ErrCode, const char* OKMsg, const char* ErrMsg)
		{
			if (ErrCode != 0)
			{
				std::cout << ErrMsg << ": " << ErrCode << "\n";
				std::cout << "program halted\n";
				exit(1);
			}
			else
				std::cout << OKMsg << "\n";
		}

		//******************** get outline ************************
		static int getOutline(FT_Glyph glyph, FT_OutlineGlyph* Outg)
		{
			int error = 0;

			switch (glyph->format)
			{
			case FT_GLYPH_FORMAT_BITMAP:
				error = 1;
				break;

			case FT_GLYPH_FORMAT_OUTLINE:
				*Outg = (FT_OutlineGlyph)glyph;
				break;

			default:
				break;
			}
			return error;
		}

		static GlyphOutline createOutlineInternal(FT_OutlineGlyph outlineGlyph, FT_Face face)
		{
			FT_Outline* outline = &outlineGlyph->outline;

			GlyphOutline res;
			FT_GlyphSlot glyphSlot = face->glyph;

			float upem = (float)face->units_per_EM;
			float glyphLeft = (float)face->glyph->metrics.horiBearingX;
			float descentY = (float)(glyphSlot->metrics.height - glyphSlot->metrics.horiBearingY);

			res.advanceX = (float)glyphSlot->advance.x / upem;
			res.bearingX = (float)glyphSlot->metrics.horiBearingX / upem;
			res.bearingY = (float)glyphSlot->metrics.horiBearingY / upem;
			res.descentY = descentY / upem;
			res.glyphWidth = (float)glyphSlot->metrics.width / upem;
			res.glyphHeight = (float)glyphSlot->metrics.height / upem;

			res.svg = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
			*res.svg = Svg::createDefault();

			{
				bool shouldCloseContour = false;

				short* contourEnds = outline->contours;     // pointer to contour ends
				FT_Vector* basePoints = outline->points;    // pointer to outline point
				char* baseFlags = outline->tags;            // pointer to flag

				for (int c = 0; c < outline->n_contours; c++)
				{
					char* flags = baseFlags;
					FT_Vector* points = basePoints;

					// Increment the base pointers to the next contour
					int startIndex = 0;
					if (c > 0)
					{
						startIndex = contourEnds[c - 1] + 1;
					}
					int endIndex = contourEnds[c];
					int numPoints = (endIndex - startIndex) + 1;
					baseFlags += numPoints;
					basePoints += numPoints;

					// Initialize the previous point stuff
					char previousFlag = flags[numPoints - 1];
					PointType previousPointType = FT_getPointType(previousFlag);
					Vec2 previousPosition = FT_Vector_ToVec2(points[numPoints - 1], glyphLeft, descentY, upem, res.glyphHeight);
					char firstPointFlag = flags[0];
					PointType firstPointType = FT_getPointType(firstPointFlag);
					Vec2 firstPoint = FT_Vector_ToVec2(points[0], glyphLeft, descentY, upem, res.glyphHeight);

					if (firstPointType == PointType::CurveTagOn)
					{
						Svg::moveTo(res.svg, firstPoint);
					}
					else if (firstPointType == PointType::CurveTagConic && previousPointType == PointType::CurveTagConic)
					{
						Vec2 hiddenPoint = Vec2{
							(firstPoint.x - previousPosition.x) / 2.0f + previousPosition.x,
							(firstPoint.y - previousPosition.y) / 2.0f + previousPosition.y
						};

						Svg::moveTo(res.svg, firstPoint);
					}
					else if (firstPointType == PointType::CurveTagConic && previousPointType == PointType::CurveTagOn)
					{
						Svg::moveTo(res.svg, firstPoint);
					}
					else
					{
						g_logger_assert(false, "Unknown curve start.");
					}

					for (int p = 1; p <= numPoints; p++)
					{
						shouldCloseContour = true;

						Vec2 position = FT_Vector_ToVec2(points[p % numPoints], glyphLeft, descentY, upem, res.glyphHeight);

						char flag = flags[p % numPoints];
						PointType pointType = FT_getPointType(flag);
						DropoutMode dropoutMode = FT_getDropoutMode(flag);
						if (dropoutMode != DropoutMode::None)
						{
							g_logger_warning("Add support for dropout modes.");
						}

						if (pointType == PointType::CurveTagOn)
						{
							// Line segment
							// On point -> On point
							if (previousPointType == PointType::CurveTagOn)
							{
								Svg::lineTo(res.svg, position);
							}
							// Bezier 2
							// On point -> Conic point -> On point
							else if (previousPointType == PointType::CurveTagConic)
							{
								Svg::bezier2To(
									res.svg,
									previousPosition,
									position
								);
							}
							// Bezier 3
							// On point -> Cubic point -> Cubic point -> On point
							else if (previousPointType == PointType::CurveTagCubic)
							{
								if (p >= 2)
								{
									char prevPrevFlag = flags[(p - 2) % numPoints];
									PointType prevPrevType = FT_getPointType(prevPrevFlag);
									g_logger_assert(prevPrevType == PointType::CurveTagCubic, "Invalid cubic bezier. Should be On Point -> Cubic Point -> Cubic Point -> On Point");
									Vec2 prevPrevPosition = FT_Vector_ToVec2(points[(p - 2) % numPoints], glyphLeft, descentY, upem, res.glyphHeight);
									Svg::bezier3To(
										res.svg,
										prevPrevPosition,
										previousPosition,
										position
									);
								}
								else
								{
									g_logger_error("A cubic curve requires at least 4 points.");
								}
							}
						}

						if (p > 0 && p != numPoints)
						{
							// Bezier 2
							// On Point -> Conic Point -> Conic Point
							//                          ^
							//                   Hidden On Point
							if (previousPointType == PointType::CurveTagConic && pointType == PointType::CurveTagConic)
							{
								// Two off points, hidden on point in between
								Vec2 hiddenPointPosition = Vec2{
									(position.x - previousPosition.x) / 2.0f + previousPosition.x,
									(position.y - previousPosition.y) / 2.0f + previousPosition.y
								};
								Svg::bezier2To(
									res.svg,
									previousPosition,
									hiddenPointPosition
								);
							}
						}

						previousPointType = pointType;
						previousPosition = position;
					}

					// Close the contour
					//Svg::lineTo(res.svg, previousPosition);
				}

				if (shouldCloseContour)
				{
					bool isHole = outline->n_contours > 1;
					Svg::closePath(res.svg, false, isHole);
				}
			}

			// Approximate contour lengths
			res.svg->calculateApproximatePerimeter();
			res.svg->calculateBBox();

			return res;
		}

		static std::string getSizedFontKey(const char* filepath, int fontSizePixels)
		{
			return getUnsizedFontKey(filepath) + "_Size_" + std::to_string(fontSizePixels);
		}

		static std::string getUnsizedFontKey(const char* filepath)
		{
			// Convert all path separators to '/'
			uint8 staticBuffer[_MAX_PATH + 1];
			int filepathSize = (int)std::strlen(filepath);

			uint8* buffer = staticBuffer;
			bool freeBuffer = false;
			if (filepathSize >= _MAX_PATH)
			{
				buffer = (uint8*)g_memory_allocate(sizeof(char) * filepathSize + 1);
				freeBuffer = true;
			}

			g_memory_copyMem(buffer, (void*)filepath, sizeof(char) * filepathSize);
			buffer[filepathSize] = '\0';

			for (int i = 0; i < filepathSize; i++)
			{
				// Switch all backslashes to forward slashes
				if (buffer[i] == '\\')
				{
					buffer[i] = '/';
				}
				// Convert the filepath to lowercase
				else if (buffer[i] >= 'A' && buffer[i] <= 'Z')
				{
					buffer[i] = (buffer[i] - 'A') + 'a';
				}
			}

			std::string res = std::string((char*)buffer, filepathSize);

			if (freeBuffer)
			{
				g_memory_free(buffer);
			}

			return res;
		}
	}
}