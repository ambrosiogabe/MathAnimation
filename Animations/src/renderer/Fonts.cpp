#include "renderer/Fonts.h"

namespace MathAnim
{
	namespace Fonts
	{
		static bool initialized = false;
		static const int hzPadding = 2;
		static const int vtPadding = 2;
		static FT_Library library;
		static std::unordered_map<std::string, Font> loadedFonts;

		static std::string getFormattedFilepath(const char* filepath, FontSize fontSize);
		static void generateAsciiTexture(Font& font);

		void init()
		{
			int error = FT_Init_FreeType(&library);
			if (error)
			{
				Logger::Error("An error occurred during freetype initialization. Font rendering will not work.");
			}

			Logger::Info("Initialized freetype library.");
			initialized = true;
		}

		Font* loadFont(const char* filepath, FontSize fontSize)
		{
			Logger::Assert(initialized, "Font library must be initialized to load a font.");

			// If the font is loaded already, return that font
			Font* possibleFont = getFont(filepath, fontSize);
			if (possibleFont != nullptr)
			{
				return possibleFont;
			}

			// Load the new font into freetype.
			FT_Face face;
			int error = FT_New_Face(library, filepath, 0, &face);
			if (error == FT_Err_Unknown_File_Format)
			{
				Logger::Error("Unsupported font file format for '%s'. Could not load font.", filepath);
				return nullptr;
			}
			else if (error)
			{
				Logger::Error("Font could not be opened or read or is broken '%s'. Could not load font.", filepath);
				return nullptr;
			}

			error = FT_Set_Char_Size(face, 0, fontSize * 64, 300, 300);
			if (error)
			{
				Logger::Error("Could not set font size appropriately for '%s'. Is this font non-scalable?", filepath);
				return nullptr;
			}

			// Generate a texture for the font and initialize the font structure
			Font font;
			font.fontFace = face;
			font.fontSize = fontSize;

			std::string formattedFilepath = getFormattedFilepath(filepath, fontSize);
			// The default is 1K texture size if the color is RGBA
			// Since we're using Red component only, we can store 1K x 4 for the same cost
			font.texture = TextureBuilder()
				.setFormat(ByteFormat::R8_UI)
				.setWidth(1024 * 4)
				.setHeight(1024 * 4)
				.setFilepath(formattedFilepath.c_str())
				.generate();

			// TODO: Turn the preset characters into a parameter
			generateAsciiTexture(font);

			loadedFonts[formattedFilepath] = font;
			
			return &loadedFonts[formattedFilepath];
		}

		void unloadFont(Font* font)
		{
			if (!font)
			{
				return;
			}

			std::string formattedPath = getFormattedFilepath(font->texture.path.string().c_str(), font->fontSize);
			if (font->texture.graphicsId != UINT32_MAX)
			{
				font->texture.destroy();
			}

			loadedFonts.erase(formattedPath);
		}

		void unloadFont(const char* filepath, FontSize fontSize)
		{
			Font* font = getFont(filepath, fontSize);
			if (font)
			{
				unloadFont(font);
			}
		}

		Font* getFont(const char* filepath, FontSize fontSize)
		{
			std::string formattedFilepath = getFormattedFilepath(filepath, fontSize);
			auto iter = loadedFonts.find(formattedFilepath);
			if (iter != loadedFonts.end())
			{
				return &iter->second;
			}

			return nullptr;
		}

		static std::string getFormattedFilepath(const char* filepath, FontSize fontSize)
		{
			return std::string(filepath) + std::to_string(fontSize);
		}

		static void generateAsciiTexture(Font& font)
		{
			uint8* fontBuffer = (uint8*)AllocMem(sizeof(uint8) * font.texture.width * font.texture.height);
			uint32 currentLineHeight = 0;
			uint32 currentX = 0;
			uint32 currentY = 0;

			for (int i = 0; i < 128; i++)
			{
				FT_UInt glyphIndex = FT_Get_Char_Index(font.fontFace, i);
				if (glyphIndex == 0)
				{
					Logger::Warning("Character code '%d' not found. Missing glyph.", i);
					continue;
				}

				// Load the glyph
				int error = FT_Load_Glyph(font.fontFace, glyphIndex, FT_LOAD_DEFAULT);
				if (error)
				{
					Logger::Error("Freetype could not load glyph for character code '%d'.", i);
					continue;
				}

				// Render the glyph to the bitmap
				// FT_RENDER_MODE_NORMAL renders a 256 (8-bit) grayscale for each pixel
				error = FT_Render_Glyph(font.fontFace->glyph, FT_RENDER_MODE_NORMAL);
				if (error)
				{
					Logger::Error("Freetype could not render glyph for character code '%d'.", i);
					continue;
				}

				currentLineHeight = glm::max(currentLineHeight, font.fontFace->glyph->bitmap.rows);
				if (currentX + font.fontFace->glyph->bitmap.width > font.texture.width)
				{
					currentX = 0;
					currentY += currentLineHeight + vtPadding;
					currentLineHeight = font.fontFace->glyph->bitmap.rows;
				}

				if (currentY + currentLineHeight > font.texture.height)
				{
					Logger::Error("Cannot continue adding to font. Stopped at char '%d' '%c' because we overran the texture height.", i, i);
					break;
				}

				// Add the glyph to our texture map
				for (int y = 0; y < font.fontFace->glyph->bitmap.rows; y++)
				{
					for (int x = 0; x < font.fontFace->glyph->bitmap.width; x++)
					{
						// Copy the glyph data to our bitmap
						int bufferX = x + currentX;
						int bufferY = font.texture.height - (y + currentY + 1);
						Logger::Assert(bufferX < font.texture.width && bufferY < font.texture.height, "Invalid bufferX, bufferY. Out of bounds greater than tex size.");
						Logger::Assert(bufferX >= 0 && bufferY >= 0, "Invalid bufferX, bufferY. Out of bounds less than 0.");
						fontBuffer[bufferX + (bufferY * font.texture.width)] = 
							font.fontFace->glyph->bitmap.buffer[x + (y * font.fontFace->glyph->bitmap.width)];
					}
				}

				currentX += font.fontFace->glyph->bitmap.width + hzPadding;
			}

			font.texture.uploadSubImage(0, 0, font.texture.width, font.texture.height, fontBuffer);
			FreeMem(fontBuffer);
		}
	}

	FontSize operator""_px(uint64 numPixels)
	{
		return (uint32)numPixels;
	}

	FontSize operator""_em(long double emSize)
	{
		return (uint32)emSize;
	}
}