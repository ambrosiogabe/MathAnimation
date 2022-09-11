#include "animation/TextAnimations.h"
#include "animation/Animation.h"
#include "animation/SvgParser.h"
#include "animation/Svg.h"
#include "renderer/Renderer.h"
#include "renderer/Framebuffer.h"
#include "renderer/Fonts.h"
#include "renderer/PerspectiveCamera.h"
#include "renderer/OrthoCamera.h"
#include "core/Application.h"
#include "latex/LaTexLayer.h"

#include "nanovg.h"

namespace MathAnim
{
	// ------------- Internal Functions -------------
	static TextObject deserializeTextV1(RawMemory& memory);
	static LaTexObject deserializeLaTexV1(RawMemory& memory);

	namespace TextAnimations
	{
		static OrthoCamera* camera;
		void init(OrthoCamera& sceneCamera)
		{
			camera = &sceneCamera;
		}
	}


	void TextObject::render(NVGcontext* vg, const AnimObject* parent) const
	{
		// TODO: This may lead to performance degradation with larger projects because
		// we can't just stroke an object we have to manually draw the svg curves
		// then do a stroke, then repeat with a fill. I should consider looking into
		// caching this stuff with nvgSave() or something which looks like it might cache
		// drawings
		renderWriteInAnimation(vg, 1.01f, parent);
	}

	void TextObject::renderWriteInAnimation(NVGcontext* vg, float t, const AnimObject* parent, bool reverse) const
	{
		if (parent->as.textObject.font == nullptr)
		{
			return;
		}

		float fontSizePixels = parent->svgScale;

		// Calculate the string size
		std::string textStr = std::string(text);
		Vec2 strSize = Vec2{ 0, font->lineHeight };
		for (int i = 0; i < textStr.length(); i++)
		{
			uint32 codepoint = (uint32)textStr[i];
			const GlyphOutline& glyphOutline = font->getGlyphInfo(codepoint);

			Vec2 glyphPos = strSize;
			Vec2 offset = Vec2{
				glyphOutline.bearingX,
				font->lineHeight - glyphOutline.bearingY
			};

			// TODO: I may have to add kerning info here
			strSize += Vec2{ glyphOutline.advanceX, 0.0f };
			
			float newMaxY = font->lineHeight + glyphOutline.descentY;
			strSize.y = glm::max(newMaxY, strSize.y);
		}

		// TODO: Offload all this stuff into some sort of TexturePacker data structure
		{
			Vec2 svgTextureOffset = Vec2{
				(float)Svg::getCacheCurrentPos().x + parent->strokeWidth * 0.5f,
				(float)Svg::getCacheCurrentPos().y + parent->strokeWidth * 0.5f
			};

			// Check if the SVG cache needs to regenerate
			float svgTotalWidth = (strSize.x * parent->svgScale) + parent->strokeWidth;
			float svgTotalHeight = (strSize.y * parent->svgScale) + parent->strokeWidth;
			{
				float newRightX = svgTextureOffset.x + svgTotalWidth;
				if (newRightX >= Svg::getSvgCache().width)
				{
					// Move to the newline
					Svg::incrementCacheCurrentY();
				}

				float newBottomY = svgTextureOffset.y + svgTotalHeight;
				if (newBottomY >= Svg::getSvgCache().height)
				{
					// TODO: Get position on new cache if needed
					Svg::growCache();
				}
			}
		}

		// Draw the string to the cache
		int numNonWhitespaceCharacters = 0;
		for (int i = 0; i < textStr.length(); i++)
		{
			if (textStr[i] != ' ' && textStr[i] != '\t' && textStr[i] != '\n')
			{
				numNonWhitespaceCharacters++;
			}
		}

		Vec2 cursorPos = Vec2{ 0, 0 };
		float numberLettersToDraw = t * (float)numNonWhitespaceCharacters;
		int numNonWhitespaceLettersDrawn = 0;
		constexpr float numLettersToLag = 2.0f;
		for (int i = 0; i < textStr.length(); i++)
		{
			uint32 codepoint = (uint32)textStr[i];
			const GlyphOutline& glyphOutline = font->getGlyphInfo(codepoint);

			float denominator = i == textStr.length() - 1 ? 1.0f : numLettersToLag;
			float percentOfLetterToDraw = (numberLettersToDraw - (float)numNonWhitespaceLettersDrawn) / denominator;
			Vec2 glyphPos = cursorPos;
			Vec2 offset = Vec2{
				glyphOutline.bearingX,
				font->lineHeight - glyphOutline.bearingY
			};

			if (textStr[i] != ' ' && textStr[i] != '\t' && textStr[i] != '\n')
			{
				glyphOutline.svg->renderCreateAnimation(vg, t, parent, offset + glyphPos, reverse, true);
				numNonWhitespaceLettersDrawn++;
			}

			// TODO: I may have to add kerning info here
			cursorPos += Vec2{ glyphOutline.advanceX, 0.0f };

			if ((float)numNonWhitespaceLettersDrawn >= numberLettersToDraw)
			{
				break;
			}
		}

		// Blit the cached quad to the screen
		{
			Vec2 svgTextureOffset = Vec2 {
				(float)Svg::getCacheCurrentPos().x + parent->strokeWidth * 0.5f,
				(float)Svg::getCacheCurrentPos().y + parent->strokeWidth * 0.5f
			};
			float svgTotalWidth = (strSize.x * parent->svgScale) + parent->strokeWidth;
			float svgTotalHeight = (strSize.y * parent->svgScale) + parent->strokeWidth;

			Vec2 cacheUvMin = Vec2{
				svgTextureOffset.x / Svg::getSvgCache().width,
				1.0f - (svgTextureOffset.y / Svg::getSvgCache().height) - (svgTotalHeight / Svg::getSvgCache().height)
			};
			Vec2 cacheUvMax = cacheUvMin +
				Vec2{
					svgTotalWidth / Svg::getSvgCache().width,
					svgTotalHeight / Svg::getSvgCache().height
			};

			if (parent->drawDebugBoxes)
			{
				// First render to the cache
				Svg::getSvgCacheFb().bind();
				glViewport(0, 0, Svg::getSvgCache().width, Svg::getSvgCache().height);

				// Reset the draw buffers to draw to FB_attachment_0
				GLenum compositeDrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_NONE, GL_NONE };
				glDrawBuffers(3, compositeDrawBuffers);

				nvgBeginFrame(vg, Svg::getSvgCache().width, Svg::getSvgCache().height, 1.0f);

				float strokeWidthCorrectionPos = Svg::getCachePadding().x * 0.5f;
				float strokeWidthCorrectionNeg = -Svg::getCachePadding().x;
				nvgBeginPath(vg);
				nvgStrokeColor(vg, nvgRGBA(0, 255, 255, 255));
				nvgStrokeWidth(vg, Svg::getCachePadding().x);
				nvgMoveTo(vg,
					cacheUvMin.x * Svg::getSvgCache().width + strokeWidthCorrectionPos,
					(1.0f - cacheUvMax.y) * Svg::getSvgCache().height + strokeWidthCorrectionPos
				);
				nvgRect(vg,
					cacheUvMin.x * Svg::getSvgCache().width + strokeWidthCorrectionPos,
					(1.0f - cacheUvMax.y) * Svg::getSvgCache().height + strokeWidthCorrectionPos,
					(cacheUvMax.x - cacheUvMin.x) * Svg::getSvgCache().width + strokeWidthCorrectionNeg,
					(cacheUvMax.y - cacheUvMin.y) * Svg::getSvgCache().height + strokeWidthCorrectionNeg
				);
				nvgClosePath(vg);
				nvgStroke(vg);
				nvgEndFrame(vg);
			}

			if (parent->is3D)
			{
				glm::mat4 transform = glm::identity<glm::mat4>();
				transform = glm::translate(
					transform,
					glm::vec3(
						parent->position.x,
						parent->position.y,
						parent->position.z
					)
				);
				transform = transform * glm::orientate4(glm::radians(glm::vec3(parent->rotation.x, parent->rotation.y, parent->rotation.z)));
				transform = glm::scale(transform, glm::vec3(parent->scale.x, parent->scale.y, parent->scale.z));

				Renderer::drawTexturedQuad3D(
					Svg::getSvgCache(),
					Vec2{ svgTotalWidth / parent->svgScale, svgTotalHeight / parent->svgScale },
					cacheUvMin,
					cacheUvMax,
					transform,
					parent->isTransparent
				);
			}
			else
			{
				glm::mat4 transform = glm::identity<glm::mat4>();
				Vec2 cameraCenteredPos = Svg::getOrthoCamera().projectionSize / 2.0f - Svg::getOrthoCamera().position;
				transform = glm::translate(
					transform,
					glm::vec3(
						parent->position.x,
						parent->position.y,
						0.0f
					)
				);
				if (!CMath::compare(parent->rotation.z, 0.0f))
				{
					transform = glm::rotate(transform, parent->rotation.z, glm::vec3(0, 0, 1));
				}
				transform = glm::scale(transform, glm::vec3(parent->scale.x, parent->scale.y, parent->scale.z));

				Renderer::drawTexturedQuad(
					Svg::getSvgCache(),
					Vec2{ svgTotalWidth / parent->svgScale, svgTotalHeight / parent->svgScale },
					cacheUvMin,
					cacheUvMax,
					transform
				);
			}

			Svg::incrementCacheCurrentX(strSize.x * parent->svgScale + parent->strokeWidth + Svg::getCachePadding().x);
			Svg::checkLineHeight(strSize.y * parent->svgScale + parent->strokeWidth);
		}
	}

	void TextObject::serialize(RawMemory& memory) const
	{
		// textLength           -> int32
		// text                 -> char[textLength]
		// fontFilepathLength   -> int32
		// fontFilepath         -> char[fontFilepathLength]

		// TODO: Specialize std::string or const char* in template so
		// we don't have to write out text char by char
		memory.write<int32>(&textLength);
		for (int i = 0; i < textLength; i++)
		{
			memory.write<char>(&text[i]);
		}
		constexpr char nullByte = '\0';
		memory.write<char>(&nullByte);

		// TODO: Overflow error checking would be good here
		if (font != nullptr)
		{
			int32 fontFilepathLength = (int32)font->fontFilepath.size();
			memory.write<int32>(&fontFilepathLength);
			for (int i = 0; i < fontFilepathLength; i++)
			{
				memory.write<char>(&font->fontFilepath[i]);
			}
		}
		else
		{
			const char stringToWrite[] = "nullFont";
			int32 fontFilepathLength = (sizeof(stringToWrite) / sizeof(char));
			// Subtract null byte
			fontFilepathLength -= 1;
			memory.write<int32>(&fontFilepathLength);
			for (int i = 0; i < fontFilepathLength; i++)
			{
				memory.write<char>(&stringToWrite[i]);
			}
		}
	}

	void TextObject::free()
	{
		if (this->font)
		{
			Fonts::unloadFont(this->font);
			this->font = nullptr;
		}

		if (this->text)
		{
			g_memory_free(this->text);
			this->text = nullptr;
			this->textLength = 0;
		}
	}

	TextObject TextObject::deserialize(RawMemory& memory, uint32 version)
	{
		if (version == 1)
		{
			return deserializeTextV1(memory);
		}

		g_logger_error("Invalid version '%d' while deserializing text object.", version);
		TextObject res;
		g_memory_zeroMem(&res, sizeof(TextObject));
		return res;
	}

	TextObject TextObject::createDefault()
	{
		TextObject res;
		// TODO: Come up with application default font
		res.font = nullptr;
		static const char defaultText[] = "Text Object";
		res.text = (char*)g_memory_allocate(sizeof(defaultText) / sizeof(char));
		g_memory_copyMem(res.text, (void*)defaultText, sizeof(defaultText) / sizeof(char));
		res.textLength = (sizeof(defaultText) / sizeof(char)) - 1;
		res.text[res.textLength] = '\0';
		return res;
	}

	void LaTexObject::update()
	{
		if (isParsingLaTex)
		{
			if (LaTexLayer::laTexIsReady(text, isEquation))
			{
				if (svgGroup)
				{
					svgGroup->free();
					svgGroup = nullptr;
				}

				std::string filepath = "latex/" + LaTexLayer::getLaTexMd5(text) + ".svg";
				svgGroup = SvgParser::parseSvgDoc(filepath.c_str());
				isParsingLaTex = false;
			}
		}
	}

	void LaTexObject::setText(const std::string& str)
	{
		if (text)
		{
			g_memory_free(text);
			text = nullptr;
			textLength = 0;
		}

		this->text = (char*)g_memory_allocate(sizeof(char) * (str.length() + 1));
		this->textLength = str.length();

		g_memory_copyMem(this->text, (void*)str.c_str(), sizeof(char) * str.length());
		this->text[this->textLength] = '\0';
	}

	void LaTexObject::setText(const char* cStr)
	{
		if (text)
		{
			g_memory_free(text);
			text = nullptr;
			textLength = 0;
		}

		size_t strLength = std::strlen(cStr);
		this->text = (char*)g_memory_allocate(sizeof(char) * (strLength + 1));
		this->textLength = strLength;

		g_memory_copyMem(this->text, (void*)cStr, sizeof(char) * strLength);
		this->text[this->textLength] = '\0';
	}

	void LaTexObject::parseLaTex()
	{
		LaTexLayer::laTexToSvg(text, isEquation);
		isParsingLaTex = true;
	}

	void LaTexObject::render(NVGcontext* vg, const AnimObject* parent) const
	{
		if (svgGroup)
		{
			// TODO: Ugly hack
			svgGroup->render(vg, (AnimObject*)parent);
		}
	}

	void LaTexObject::renderCreateAnimation(NVGcontext* vg, float t, const AnimObject* parent, bool reverse) const
	{
		if (svgGroup)
		{
			// TODO: Super ugly hack...
			svgGroup->renderCreateAnimation(vg, t, (AnimObject*)parent, reverse);
		}
	}

	void LaTexObject::serialize(RawMemory& memory) const
	{
		// textLength       -> i32
		// text             -> char[textLength]
		// isEquation       -> u8 (bool)

		// TODO: Specialize std::string or const char* in template so
		// we don't have to write out text char by char
		memory.write<int32>(&textLength);
		for (int i = 0; i < textLength; i++)
		{
			memory.write<char>(&text[i]);
		}
		constexpr char nullByte = '\0';
		memory.write<char>(&nullByte);

		uint8 isEquationU8 = isEquation ? 1 : 0;
		memory.write<uint8>(&isEquationU8);
	}

	void LaTexObject::free()
	{
		if (svgGroup)
		{
			svgGroup->free();
			g_memory_free(svgGroup);
			svgGroup = nullptr;
		}

		if (text)
		{
			g_memory_free(text);
			text = nullptr;
		}
	}

	LaTexObject LaTexObject::deserialize(RawMemory& memory, uint32 version)
	{
		if (version == 1)
		{
			return deserializeLaTexV1(memory);
		}

		g_logger_error("Invalid version '%d' while deserializing text object.", version);
		LaTexObject res;
		g_memory_zeroMem(&res, sizeof(LaTexObject));
		return res;
	}

	LaTexObject LaTexObject::createDefault()
	{
		LaTexObject res;
		// Alternating harmonic series
		static const char defaultLatex[] = R"raw(\sum _{k=1}^{\infty }{\frac {(-1)^{k+1}}{k}}={\frac {1}{1}}-{\frac {1}{2}}+{\frac {1}{3}}-{\frac {1}{4}}+\cdots =\ln 2)raw";
		res.text = (char*)g_memory_allocate(sizeof(defaultLatex) / sizeof(char));
		g_memory_copyMem(res.text, (void*)defaultLatex, sizeof(defaultLatex) / sizeof(char));
		res.textLength = (sizeof(defaultLatex) / sizeof(char)) - 1;
		res.text[res.textLength] = '\0';
		res.isEquation = true;
		res.svgGroup = nullptr;
		res.isParsingLaTex = false;
		res.parseLaTex();

		return res;
	}

	// ------------- Internal Functions -------------
	static TextObject deserializeTextV1(RawMemory& memory)
	{
		TextObject res;

		// textLength           -> int32
		// text                 -> char[textLength]
		// fontFilepathLength   -> int32
		// fontFilepath         -> char[fontFilepathLength]

		// TODO: Specialize std::string or const char* in template so
		// we don't have to read in text char by char
		memory.read<int32>(&res.textLength);
		res.text = (char*)g_memory_allocate(sizeof(char) * (res.textLength + 1));
		for (int i = 0; i < res.textLength; i++)
		{
			memory.read<char>(&res.text[i]);
		}
		memory.read<char>(&res.text[res.textLength]);

		// TODO: Error checking would be good here
		int32 fontFilepathLength;
		memory.read<int32>(&fontFilepathLength);
		// Initialize filepath to XXXX...
		std::string fontFilepath = std::string(fontFilepathLength, 'X');
		for (int i = 0; i < fontFilepathLength; i++)
		{
			memory.read<char>(&fontFilepath[i]);
		}

		res.font = Fonts::loadFont(fontFilepath.c_str(), Application::getNvgContext());

		return res;
	}

	static LaTexObject deserializeLaTexV1(RawMemory& memory)
	{
		LaTexObject res;

		// textLength       -> i32
		// text             -> char[textLength]
		// isEquation       -> u8 (bool)

		// TODO: Specialize std::string or const char* in template so
		// we don't have to read in text char by char

		memory.read<int32>(&res.textLength);
		res.text = (char*)g_memory_allocate(sizeof(char) * (res.textLength + 1));
		for (int i = 0; i < res.textLength; i++)
		{
			memory.read<char>(&res.text[i]);
		}
		memory.read<char>(&res.text[res.textLength]);

		uint8 isEquationU8;
		memory.read<uint8>(&isEquationU8);
		res.isEquation = isEquationU8 == 1;
		res.isParsingLaTex = false;
		res.svgGroup = nullptr;
		res.parseLaTex();

		return res;
	}
}