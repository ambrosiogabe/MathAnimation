#include "animation/TextAnimations.h"
#include "animation/Animation.h"
#include "renderer/Fonts.h"

#include "nanovg.h"

namespace MathAnim
{
	// ------------- Internal Functions -------------
	static void renderWriteInCodepointAnimation(NVGcontext* vg, uint32 codepoint, float t, Font* font, float fontScale, const glm::vec4& glyphPos);
	
	static TextObject deserializeTextV1(RawMemory& memory);

	void TextObject::render(NVGcontext* vg, const AnimObject* parent) const
	{
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgFontFace(vg, font->vgFontFace.c_str());
		nvgFontSize(vg, fontSizePixels);
		nvgText(vg, parent->position.x, (font->lineHeight * 0.5f * fontSizePixels) + parent->position.y, text, NULL);
	}

	void TextObject::renderWriteInAnimation(NVGcontext* vg, float t, const AnimObject* parent) const
	{
		std::string textStr = std::string(text);
		glm::vec4 cursorPos = glm::vec4(parent->position.x, parent->position.y, 0.0f, 1.0f);
		int numNonWhitespaceCharacters = 0;
		for (int i = 0; i < textStr.length(); i++)
		{
			if (textStr[i] != ' ' && textStr[i] != '\t' && textStr[i] != '\n')
			{
				numNonWhitespaceCharacters++;
			}
		}

		float numberLettersToDraw = t * (float)numNonWhitespaceCharacters;
		int numNonWhitespaceLettersDrawn = 0;
		for (int i = 0; i < textStr.length(); i++)
		{
			uint32 codepoint = (uint32)textStr[i];
			const GlyphOutline& glyphOutline = font->getGlyphInfo(codepoint);

			float percentOfLetterToDraw = numberLettersToDraw - (float)numNonWhitespaceLettersDrawn;
			glm::vec4 glyphPos = cursorPos;
			// TODO: Do some debugging to make sure this is accurate
			// The bounding box of the text's top left corner should
			// be where the text gets positioned from
			glyphPos.y -= (font->lineHeight / 2.0f) * fontSizePixels;
			renderWriteInCodepointAnimation(vg, codepoint, percentOfLetterToDraw, font, fontSizePixels, glyphPos);

			// TODO: I may have to add kerning info here
			cursorPos += glm::vec4(glyphOutline.advanceX * fontSizePixels, 0.0f, 0.0f, 1.0f);

			if (textStr[i] != ' ' && textStr[i] != '\t' && textStr[i] != '\n')
			{
				numNonWhitespaceLettersDrawn++;
			}

			if ((float)numNonWhitespaceLettersDrawn >= numberLettersToDraw)
			{
				break;
			}
		}
	}

	void TextObject::serialize(RawMemory& memory) const
	{
		// fontSizePixels       -> float
		// textLength           -> int32
		// text                 -> char[textLength]
		// fontFilepathLength   -> int32
		// fontFilepath         -> char[fontFilepathLength]
		memory.write<float>(&fontSizePixels);

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
		int32 fontFilepathLength = font->vgFontFace.size();
		memory.write<int32>(&fontFilepathLength);
		for (int i = 0; i < fontFilepathLength; i++)
		{
			memory.write<char>(&font->vgFontFace[i]);
		}
	}

	void TextObject::free()
	{
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
		res.font = Fonts::getFont("C:/Windows/Fonts/BASKVILL.TTF");
		res.fontSizePixels = 128;
		static const char defaultText[] = "Text Object";
		res.text = (char*)g_memory_allocate(sizeof(defaultText) / sizeof(char));
		g_memory_copyMem(res.text, (void*)defaultText, sizeof(defaultText) / sizeof(char));
		res.textLength = (sizeof(defaultText) / sizeof(char)) - 1;
		res.text[res.textLength] = '\0';
		return res;
	}

	void LaTexObject::render(NVGcontext* vg, const AnimObject* parent) const
	{
		g_logger_warning("TODO: Implement me");
	}

	void LaTexObject::serialize(RawMemory& memory) const
	{
		g_logger_warning("TODO: Implement me");
	}

	void LaTexObject::free()
	{
		g_logger_warning("TODO: Implement me");
	}

	LaTexObject LaTexObject::deserialize(RawMemory& memory, uint32 version)
	{
		g_logger_warning("TODO: Implement me");
		return {};
	}

	LaTexObject LaTexObject::createDefault()
	{
		g_logger_warning("TODO: Implement me");
		return {};
	}

	// ------------- Internal Functions -------------
	static void renderWriteInCodepointAnimation(NVGcontext* vg, uint32 codepoint, float t, Font* font, float fontScale, const glm::vec4& glyphPos)
	{
		const GlyphOutline& glyphOutline = font->getGlyphInfo(codepoint);

		// Start the fade in after 80% of the codepoint is drawn
		constexpr float fadeInStart = 0.8f;
		float lengthToDraw = t * (float)glyphOutline.totalCurveLengthApprox;
		float amountToFadeIn = ((t - fadeInStart) / (1.0f - fadeInStart));
		float percentToFadeIn = glm::max(glm::min(amountToFadeIn, 1.0f), 0.0f);

		if (lengthToDraw > 0)
		{
			float lengthDrawn = 0.0f;
			for (int c = 0; c < glyphOutline.numContours; c++)
			{
				nvgBeginPath(vg);
				// Fade the stroke out as the font fades in
				nvgStrokeColor(vg, nvgRGBA(255, 255, 255, (unsigned char)(255.0f * (1.0f - percentToFadeIn))));
				nvgStrokeWidth(vg, 5.0f);

				if (glyphOutline.contours[c].numVertices > 0)
				{
					glm::vec4 pos = glm::vec4(
						glyphOutline.contours[c].vertices[0].position.x + glyphOutline.bearingX,
						glyphOutline.contours[c].vertices[0].position.y - glyphOutline.descentY,
						0.0f,
						1.0f
					);
					nvgMoveTo(
						vg,
						pos.x * fontScale + glyphPos.x,
						(1.0f - pos.y) * fontScale + glyphPos.y
					);
				}

				bool stoppedEarly = false;
				for (int vi = 0; vi < glyphOutline.contours[c].numVertices; vi += 2)
				{
					float lengthLeft = lengthToDraw - (float)lengthDrawn;
					if (lengthLeft < 0.0f)
					{
						break;
					}

					bool isLine = !glyphOutline.contours[c].vertices[vi + 1].controlPoint;
					if (isLine)
					{
						glm::vec4 p0 = glm::vec4(
							glyphOutline.contours[c].vertices[vi].position.x + glyphOutline.bearingX,
							glyphOutline.contours[c].vertices[vi].position.y - glyphOutline.descentY,
							0.0f,
							1.0f
						);
						glm::vec4 p1 = glm::vec4(
							glyphOutline.contours[c].vertices[vi + 1].position.x + glyphOutline.bearingX,
							glyphOutline.contours[c].vertices[vi + 1].position.y - glyphOutline.descentY,
							0.0f,
							1.0f
						);
						float curveLength = glm::length(p1 - p0);
						lengthDrawn += curveLength;

						if (lengthLeft < curveLength)
						{
							float percentOfCurveToDraw = lengthLeft / curveLength;
							p1 = (p1 - p0) * percentOfCurveToDraw + p0;
						}

						glm::vec4 projectedPos = p1;
						nvgLineTo(
							vg,
							projectedPos.x * fontScale + glyphPos.x,
							(1.0f - projectedPos.y) * fontScale + glyphPos.y
						);
					}
					else
					{
						const Vec2& p0 = glyphOutline.contours[c].vertices[vi + 0].position;
						const Vec2& p1 = glyphOutline.contours[c].vertices[vi + 1].position;
						const Vec2& p1a = glyphOutline.contours[c].vertices[vi + 1].position;
						const Vec2& p2 = glyphOutline.contours[c].vertices[vi + 2].position;

						// Degree elevated quadratic bezier curve
						glm::vec4 pr0 = glm::vec4(
							p0.x + glyphOutline.bearingX,
							p0.y - glyphOutline.descentY,
							0.0f,
							1.0f
						);
						glm::vec4 pr1 = (1.0f / 3.0f) * glm::vec4(
							p0.x + glyphOutline.bearingX,
							p0.y - glyphOutline.descentY,
							0.0f,
							1.0f
						) + (2.0f / 3.0f) * glm::vec4(
							p1.x + glyphOutline.bearingX,
							p1.y - glyphOutline.descentY,
							0.0f,
							1.0f
						);
						glm::vec4 pr2 = (2.0f / 3.0f) * glm::vec4(
							p1.x + glyphOutline.bearingX,
							p1.y - glyphOutline.descentY,
							0.0f,
							1.0f
						) + (1.0f / 3.0f) * glm::vec4(
							p2.x + glyphOutline.bearingX,
							p2.y - glyphOutline.descentY,
							0.0f,
							1.0f
						);
						glm::vec4 pr3 = glm::vec4(
							p2.x + glyphOutline.bearingX,
							p2.y - glyphOutline.descentY,
							0.0f,
							1.0f
						);

						float chordLength = glm::length(pr3 - pr0);
						float controlNetLength = glm::length(pr1 - pr0) + glm::length(pr2 - pr1) + glm::length(pr3 - pr2);
						float approxLength = (chordLength + controlNetLength) / 2.0f;
						lengthDrawn += approxLength;

						if (lengthLeft < approxLength)
						{
							// Interpolate the curve
							float percentOfCurveToDraw = lengthLeft / approxLength;

							pr1 = (pr1 - pr0) * percentOfCurveToDraw + pr0;
							pr2 = (pr2 - pr1) * percentOfCurveToDraw + pr1;
							pr3 = (pr3 - pr2) * percentOfCurveToDraw + pr2;
						}

						// Project the verts
						pr1 = pr1;
						pr2 = pr2;
						pr3 = pr3;

						nvgBezierTo(
							vg,
							pr1.x * fontScale + glyphPos.x,
							(1.0f - pr1.y) * fontScale + glyphPos.y,
							pr2.x * fontScale + glyphPos.x,
							(1.0f - pr2.y) * fontScale + glyphPos.y,
							pr3.x * fontScale + glyphPos.x,
							(1.0f - pr3.y) * fontScale + glyphPos.y
						);

						vi++;
					}

					if (lengthDrawn > lengthToDraw)
					{
						stoppedEarly = true;
						break;
					}
				}

				nvgStroke(vg);

				if (lengthDrawn > lengthToDraw)
				{
					break;
				}
			}
		}

		if (amountToFadeIn > 0)
		{
			std::string str = std::string("") + (char)codepoint;
			nvgFillColor(vg, nvgRGBA(255, 255, 255, (unsigned char)(255.0f * percentToFadeIn)));
			nvgFontFace(vg, font->vgFontFace.c_str());
			nvgFontSize(vg, fontScale);
			nvgText(vg, glyphPos.x, fontScale + glyphPos.y, str.c_str(), NULL);
		}
	}

	static TextObject deserializeTextV1(RawMemory& memory)
	{
		TextObject res;

		// fontSizePixels       -> float
		// textLength           -> int32
		// text                 -> char[textLength]
		// fontFilepathLength   -> int32
		// fontFilepath         -> char[fontFilepathLength]
		memory.read<float>(&res.fontSizePixels);

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

		// Now load the font somehow...? :think_face:
		res.font = Fonts::getFont(fontFilepath.c_str());

		return res;
	}
}