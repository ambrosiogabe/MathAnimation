#include "renderer/Fonts.h"
#include "renderer/Renderer.h"
#include "core/Application.h"

#include <nanovg.h>

#include <freetype/ftglyph.h>
#include <freetype/freetype.h>
#include <freetype/ftoutln.h>
#include <freetype/ftbbox.h>

namespace MathAnim
{
	CharRange CharRange::Ascii = { 32, 127 };

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
		if (contours)
		{
			for (int c = 0; c < numContours; c++)
			{
				if (contours[c].vertices)
				{
					g_memory_free(contours[c].vertices);
					contours[c].vertices = nullptr;
					contours[c].numVertices = 0;
					contours[c].numCurves = 0;
				}
			}
		}

		g_memory_free(contours);
		contours = nullptr;
		numContours = 0;
		totalCurveLengthApprox = 0.0f;
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
		static std::unordered_map<std::string, SharedFont> loadedFonts;
		static std::unordered_map<std::string, SharedSizedFont> loadedSizedFonts;

		static void generateDefaultCharset(Font& font, CharRange defaultCharset);
		static int getOutline(FT_Glyph glyph, FT_OutlineGlyph* Outg);
		static GlyphOutline createOutlineInternal(FT_OutlineGlyph outlineGlyph, FT_Face face);
		static std::string getSizedFontKey(const char* filepath, int fontSizePixels);

		void init()
		{
			int error = FT_Init_FreeType(&library);
			if (error)
			{
				g_logger_error("An error occurred during freetype initialization. Font rendering will not work.");
			}

			g_logger_info("Initialized freetype library.");
			initialized = true;
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

			SizedFont res;
			res.unsizedFont = loadFont(filepath, Application::getNvgContext(), defaultCharset);
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
				for (int y = 0; y < bitmap.rows; y++)
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
			res.texture.uploadSubImage(0, 0, textureWidth, textureHeight, textureMemory);

			g_memory_free(textureMemory);

			// All done now cache the result and return it
			loadedSizedFonts[sizedFontKey].font = res;
			loadedSizedFonts[sizedFontKey].referenceCount = 1;
			
			return &loadedSizedFonts[sizedFontKey].font;
		}

		void unloadSizedFont(const char* filepath)
		{
			auto iter = loadedSizedFonts.find(filepath);
			if (iter == loadedSizedFonts.end())
			{
				g_logger_warning("Tried to unload sized font that was not cached.");
				return;
			}

			SizedFont* font = &iter->second.font;
			iter->second.referenceCount--;
			if (iter->second.referenceCount <= 0)
			{
				g_logger_info("Unloading sized font '%s'.", font->unsizedFont->fontFilepath);
			}

			unloadFont(font->unsizedFont);
			if (iter->second.referenceCount <= 0)
			{
				// Really unload the font now
				g_logger_info("Unloading sized font '%s'.", font->unsizedFont->fontFilepath);
				font->texture.destroy();
				loadedSizedFonts.erase(iter);
			}
		}

		Font* loadFont(const char* filepath, NVGcontext* vg, CharRange defaultCharset)
		{
			g_logger_assert(initialized, "Font library must be initialized to load a font.");

			std::string filepathStr = std::string(filepath);
			{
				auto iter = loadedFonts.find(filepathStr);
				if (iter != loadedFonts.end())
				{
					iter->second.referenceCount++;
					return &iter->second.font;
				}
			}

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
			font.fontFilepath = filepathStr;
			font.fontFace = face;
			font.unitsPerEM = (float)face->units_per_EM;
			font.lineHeight = (float)face->height / font.unitsPerEM;

			// TODO: Turn the preset characters into a parameter
			generateDefaultCharset(font, defaultCharset);

			int vgFontError = nvgCreateFont(vg, font.fontFilepath.c_str(), font.fontFilepath.c_str());
			if (vgFontError == -1)
			{
				g_logger_error("Failed to create vgFont for font '%s'.", filepathStr.c_str());
				font.fontFilepath = "";
			}

			loadedFonts[filepathStr].font = font;
			loadedFonts[filepathStr].referenceCount = 1;

			return &loadedFonts[filepathStr].font;
		}

		void unloadFont(Font* font)
		{
			if (!font)
			{
				g_logger_warning("Tried to unload font that was nullptr.");
				return;
			}

			for (std::pair<const uint32, GlyphOutline>& kv : font->glyphMap)
			{
				GlyphOutline& outline = kv.second;
				outline.free();
			}

			FT_Done_Face(font->fontFace);
			font->fontFace = nullptr;

			{
				auto iter = loadedFonts.find(font->fontFilepath);
				if (iter != loadedFonts.end())
				{
					iter->second.referenceCount--;
					if (iter->second.referenceCount <= 0)
					{
						g_logger_info("Unloading font '%s'.", font->fontFilepath.c_str());
						loadedFonts.erase(iter);
					}
				}
				else
				{
					g_logger_warning("Tried to unload font '%s' that was not cached for some reason.", font->fontFilepath.c_str());
				}
			}
		}

		void unloadFont(const char* filepath)
		{
			auto iter = loadedFonts.find(filepath);
			if (iter == loadedFonts.end())
			{
				g_logger_warning("Tried to load font that was never cached '%s'.", filepath);
				return;
			}
			
			unloadFont(&iter->second.font);
		}

		void unloadAllFonts()
		{
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

		Font* getFont(const char* filepath)
		{
			auto iter = loadedFonts.find(std::string(filepath));
			if (iter != loadedFonts.end())
			{
				return &iter->second.font;
			}

			return nullptr;
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

			// Iterate once to count the number of total vertices
			{
				int start = 0;                          //start index of contour
				int end = 0;                            //end index of contour
				short* contourEnd = outline->contours;  //pointer to contour end
				FT_Vector* point = outline->points;     //pointer to outline point
				char* flags = outline->tags;            //pointer to flags

				res.numContours = outline->n_contours;
				res.totalCurveLengthApprox = 0.0f;
				res.contours = (GlyphContour*)g_memory_allocate(sizeof(GlyphContour) * res.numContours);

				for (int c = 0; c < outline->n_contours; c++)
				{
					// printf("\nContour %d:\n", c);
					end = *contourEnd;

					res.contours[c].numVertices = 0;
					res.contours[c].numCurves = 0;
					res.contours[c].vertices = nullptr;

					bool previousPointWasControlPoint = false;
					for (int p = start; p <= end + 1; p++)
					{
						char flag = *flags;
						bool bezierControlPoint = !(flag & 0x1);
						bool thirdOrderControlPoint = bezierControlPoint && (flag & 0x2);
						bool secondOrderControlPoint = bezierControlPoint && !(flag & 0x2);
						bool dropoutEnabled = bezierControlPoint && (flag & 0x4);
						// float pointx = ((float)(point->x - glyphLeft) / upem);
						// float pointy = ((float)(point->y + descentY) / upem);
						// Vec2 position = Vec2{ pointx, pointy };

						res.contours[c].numVertices++;
						res.contours[c].numCurves++;
						if (p > start && p != end + 1)
						{
							if (previousPointWasControlPoint && bezierControlPoint)
							{
								res.contours[c].numVertices += 2;
							}
							else if (!bezierControlPoint)
							{
								res.contours[c].numVertices++;
							}
						}
						else
						{
							g_logger_assert(!bezierControlPoint, "Should start off and end off every contour");
						}

						previousPointWasControlPoint = bezierControlPoint;

						// printf("Point %d: X=%2.3f Y=%2.3f Control_Point=%d 2_Order=%d 3_Order=%d Dropout=%d\n",
						// 	p, position.x, position.y, bezierControlPoint, secondOrderControlPoint, thirdOrderControlPoint, dropoutEnabled);

						static char* flagsEndPtr;
						static FT_Vector* pointEndPtr;
						if (p == end)
						{
							// Set flags to the first flags and save the position it should be at
							flagsEndPtr = flags + 1;
							flags = flags - (end - start);

							// Set the point to the first point and save the position it should be at
							pointEndPtr = point + 1;
							point = point - (end - start);
						}
						else
						{
							flags++;
							point++;
						}

						if (p == end + 1)
						{
							// Reset the flags to the last flags pointer
							// and the point to the last point pointer
							flags = flagsEndPtr;
							point = pointEndPtr;
						}
					}
					contourEnd++;
					start = end + 1;

					// printf("Number points: %d", res.contours[c].numVertices);
				}
			}

			// Iterate second time to calculate all the positions
			{
				int start = 0;                          //start index of contour
				int end = 0;                            //end index of contour
				short* contourEnd = outline->contours;  //pointer to contour end
				FT_Vector* point = outline->points;     //pointer to outline point
				char* flags = outline->tags;            //pointer to flag

				for (int c = 0; c < outline->n_contours; c++)
				{
					end = *contourEnd;

					res.contours[c].vertices = (GlyphVertex*)g_memory_allocate(sizeof(GlyphVertex) * res.contours[c].numVertices);

					bool previousPointWasControlPoint = false;
					Vec2 previousPosition;
					int vertexStackPointer = 0;
					for (int p = start; p <= end + 1; p++)
					{
						char flag = *flags;
						bool bezierControlPoint = !(flag & 0x1);
						bool thirdOrderControlPoint = bezierControlPoint && (flag & 0x2);
						bool secondOrderControlPoint = bezierControlPoint && !(flag & 0x2);
						bool dropoutEnabled = bezierControlPoint && (flag & 0x4);
						float pointx = ((float)(point->x - glyphLeft) / upem);
						float pointy = ((float)(point->y + descentY) / upem);
						// Flip the point y since the glyph is "upside down"
						pointy = res.glyphHeight - pointy;
						Vec2 position = Vec2{ pointx, pointy };

						g_logger_assert(!thirdOrderControlPoint, "No Support.");
						g_logger_assert(!dropoutEnabled, "No Support.");

						if (bezierControlPoint && !previousPointWasControlPoint)
						{
							res.contours[c].vertices[vertexStackPointer].position = position;
							res.contours[c].vertices[vertexStackPointer].controlPoint = true;
							vertexStackPointer++;
						}
						else if (!bezierControlPoint)
						{
							res.contours[c].vertices[vertexStackPointer].position = position;
							res.contours[c].vertices[vertexStackPointer].controlPoint = false;
							vertexStackPointer++;
						}

						if (p > start && p != end + 1)
						{
							if (previousPointWasControlPoint && bezierControlPoint)
							{
								// Two off points, hidden on point in between
								Vec2 hiddenPointPosition = Vec2{
									(position.x - previousPosition.x) / 2.0f + previousPosition.x,
									(position.y - previousPosition.y) / 2.0f + previousPosition.y
								};
								res.contours[c].vertices[vertexStackPointer].position = hiddenPointPosition;
								res.contours[c].vertices[vertexStackPointer].controlPoint = false;
								vertexStackPointer++;
								res.contours[c].vertices[vertexStackPointer].position = hiddenPointPosition;
								res.contours[c].vertices[vertexStackPointer].controlPoint = false;
								vertexStackPointer++;
							}
							else if (!bezierControlPoint)
							{
								// Starting and ending, push another copy of this vertex
								res.contours[c].vertices[vertexStackPointer].position = position;
								res.contours[c].vertices[vertexStackPointer].controlPoint = false;
								vertexStackPointer++;
							}
						}
						else if (p == start)
						{
							g_logger_assert(!bezierControlPoint, "Should start with an off point for every contour.");
						}

						if (bezierControlPoint && previousPointWasControlPoint)
						{
							res.contours[c].vertices[vertexStackPointer].position = position;
							res.contours[c].vertices[vertexStackPointer].controlPoint = true;
							vertexStackPointer++;
						}

						previousPointWasControlPoint = bezierControlPoint;
						previousPosition = position;

						static char* flagsEndPtr;
						static FT_Vector* pointEndPtr;
						if (p == end)
						{
							// Set flags to the first flags and save the position it should be at
							flagsEndPtr = flags + 1;
							flags = flags - (end - start);

							// Set the point to the first point and save the position it should be at
							pointEndPtr = point + 1;
							point = point - (end - start);
						}
						else
						{
							flags++;
							point++;
						}

						if (p == end + 1)
						{
							// Reset the flags to the last flags pointer
							// and the point to the last point pointer
							flags = flagsEndPtr;
							point = pointEndPtr;
						}
					}

					contourEnd++;
					start = end + 1;
				}
			}

			// Approximate contour lengths
			for (int c = 0; c < res.numContours; c++)
			{
				for (int vi = 0; vi < res.contours[c].numVertices; vi += 2)
				{
					bool isLine = !res.contours[c].vertices[vi + 1].controlPoint;
					if (isLine)
					{
						glm::vec4 p0 = glm::vec4(
							res.contours[c].vertices[vi].position.x,
							res.contours[c].vertices[vi].position.y,
							0.0f,
							1.0f
						);
						glm::vec4 pos = glm::vec4(
							res.contours[c].vertices[vi + 1].position.x,
							res.contours[c].vertices[vi + 1].position.y,
							0.0f,
							1.0f
						);

						res.totalCurveLengthApprox += glm::length(pos - p0);
					}
					else
					{
						const Vec2& p0 = res.contours[c].vertices[vi + 0].position;
						const Vec2& p1 = res.contours[c].vertices[vi + 1].position;
						const Vec2& p1a = res.contours[c].vertices[vi + 1].position;
						const Vec2& p2 = res.contours[c].vertices[vi + 2].position;

						// Degree elevated quadratic bezier curve
						glm::vec4 pr0 = glm::vec4(p0.x, p0.y, 0.0f, 1.0f);
						glm::vec4 pr1 = (1.0f / 3.0f) * glm::vec4(p0.x, p0.y, 0.0f, 1.0f) +
							(2.0f / 3.0f) * glm::vec4(p1.x, p1.y, 0.0f, 1.0f);
						glm::vec4 pr2 = (2.0f / 3.0f) * glm::vec4(p1.x, p1.y, 0.0f, 1.0f) +
							(1.0f / 3.0f) * glm::vec4(p2.x, p2.y, 0.0f, 1.0f);
						glm::vec4 pr3 = glm::vec4(p2.x, p2.y, 0.0f, 1.0f);

						float chordLength = glm::length(pr3 - pr0);
						float controlNetlength = glm::length(pr1 - pr0) + glm::length(pr2 - pr1) + glm::length(pr3 - pr2);

						// Approximate the length of the bezier curve
						res.totalCurveLengthApprox += (chordLength + controlNetlength) / 2.0f;

						vi++;
					}
				}
			}

			return res;
		}

		static std::string getSizedFontKey(const char* filepath, int fontSizePixels)
		{
			return std::string(filepath) + "_Size_" + std::to_string(fontSizePixels);
		}
	}
}