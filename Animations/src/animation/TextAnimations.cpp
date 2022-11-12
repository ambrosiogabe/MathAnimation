#include "animation/TextAnimations.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"
#include "svg/SvgParser.h"
#include "svg/Svg.h"
#include "renderer/Renderer.h"
#include "renderer/Framebuffer.h"
#include "renderer/Fonts.h"
#include "renderer/PerspectiveCamera.h"
#include "renderer/OrthoCamera.h"
#include "core/Application.h"
#include "latex/LaTexLayer.h"
#include "editor/SceneHierarchyPanel.h"

#include "nanovg.h"

namespace MathAnim
{
	// ------------- Internal Functions -------------
	static TextObject deserializeTextV1(RawMemory& memory);
	static LaTexObject deserializeLaTexV1(RawMemory& memory);

	void TextObject::init(AnimationManagerData* am, AnimObjId parentId, float svgScale)
	{
		if (font == nullptr)
		{
			return;
		}

		float fontSizePixels = svgScale;
		std::string textStr = std::string(text);

		// Generate children that represent each character of the text object `obj`
		Vec2 cursorPos = Vec2{ 0, 0 };
		for (int i = 0; i < textStr.length(); i++)
		{
			uint32 codepoint = (uint32)textStr[i];
			const GlyphOutline& glyphOutline = font->getGlyphInfo(codepoint);
			if (!glyphOutline.svg)
			{
				continue;
			}

			float halfGlyphHeight = glyphOutline.glyphHeight / 2.0f;
			float halfGlyphWidth = glyphOutline.glyphWidth / 2.0f;
			Vec2 offset = Vec2{
				glyphOutline.bearingX + halfGlyphWidth,
				halfGlyphHeight - glyphOutline.descentY
			};

			if (textStr[i] != ' ' && textStr[i] != '\t' && textStr[i] != '\n')
			{
				// Add this character as a child
				AnimObject childObj = AnimObject::createDefaultFromParent(am, AnimObjectTypeV1::SvgObject, parentId, true);
				childObj.parentId = parentId;
				Vec2 finalOffset = offset + cursorPos;
				childObj._positionStart = Vec3{ finalOffset.x, finalOffset.y, 0.0f };
				childObj.isGenerated = true;
				// Copy the glyph as the svg object here
				childObj._svgObjectStart = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
				childObj.svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
				*(childObj._svgObjectStart) = Svg::createDefault();
				*(childObj.svgObject) = Svg::createDefault();
				Svg::copy(childObj._svgObjectStart, glyphOutline.svg);

				childObj.name = (uint8*)g_memory_realloc(childObj.name, sizeof(uint8) * 2);
				childObj.nameLength = 1;
				childObj.name[0] = codepoint;
				childObj.name[1] = '\0';

				AnimationManager::addAnimObject(am, childObj);
				// TODO: Ugly what do I do???
				SceneHierarchyPanel::addNewAnimObject(childObj);
			}

			// TODO: I may have to add kerning info here
			cursorPos += Vec2{ glyphOutline.advanceX, 0.0f };
		}
	}

	void TextObject::reInit(AnimationManagerData* am, AnimObject* obj)
	{
		// First remove all generated children, which were generated as a result
		// of this object (presumably)
		// NOTE: This is direct descendants, no recursive children here
		for (int i = 0; i < obj->generatedChildrenIds.size(); i++)
		{
			AnimObject* child = AnimationManager::getMutableObject(am, obj->generatedChildrenIds[i]);
			if (child)
			{
				SceneHierarchyPanel::deleteAnimObject(*child);
				AnimationManager::removeAnimObject(am, obj->generatedChildrenIds[i]);
			}
		}

		// Next init again which should regenerate the children
		init(am, obj->id, obj->svgScale);
	}

	void TextObject::serialize(RawMemory& memory) const
	{
		// textLength           -> int32
		// text                 -> char[textLength]
		// fontFilepathLength   -> int32
		// fontFilepath         -> char[fontFilepathLength]
		memory.write<int32>(&textLength);
		memory.writeDangerous((uint8*)text, sizeof(uint8) * textLength);

		// TODO: Overflow error checking would be good here
		if (font != nullptr)
		{
			int32 fontFilepathLength = (int32)font->fontFilepath.size();
			memory.write<int32>(&fontFilepathLength);
			memory.writeDangerous((uint8*)font->fontFilepath.c_str(), sizeof(uint8) * fontFilepathLength);
		}
		else
		{
			const char stringToWrite[] = "nullFont";
			int32 fontFilepathLength = (sizeof(stringToWrite) / sizeof(char));
			// Subtract null byte
			fontFilepathLength -= 1;
			memory.write<int32>(&fontFilepathLength);
			memory.writeDangerous((uint8*)stringToWrite, sizeof(uint8) * fontFilepathLength);
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

	TextObject TextObject::createCopy(const TextObject& from)
	{
		TextObject res;
		// NOTE: We can pass nullptr for vg here because it shouldn't actually need it
		res.font = Fonts::loadFont(from.font->fontFilepath.c_str(), nullptr);
		res.text = (char*)g_memory_allocate((from.textLength + 1) * sizeof(char));
		g_memory_copyMem(res.text, (void*)from.text, from.textLength);
		res.textLength = from.textLength;
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
		this->textLength = (int32)str.length();

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
		this->textLength = (int32)strLength;

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
			svgGroup->render(vg, (AnimObject*)parent);
		}
	}

	void LaTexObject::renderCreateAnimation(NVGcontext* vg, float t, const AnimObject* parent, bool reverse) const
	{
		if (svgGroup)
		{
			//svgGroup->renderCreateAnimation(vg, t, (AnimObject*)parent, reverse);
			static bool displayMessage = true;
			if (displayMessage)
			{
				g_logger_error("TODO: LaTexObject::renderCreateAnimation() is not implemented yet. This message will suppress itself.");
				displayMessage = false;
			}
		}
	}

	void LaTexObject::serialize(RawMemory& memory) const
	{
		// textLength       -> i32
		// text             -> char[textLength]
		// isEquation       -> u8 (bool)
		memory.write<int32>(&textLength);
		memory.writeDangerous((const uint8*)text, sizeof(uint8) * textLength);

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

		memory.read<int32>(&res.textLength);
		res.text = (char*)g_memory_allocate(sizeof(char) * (res.textLength + 1));
		memory.readDangerous((uint8*)res.text, sizeof(uint8) * res.textLength);
		res.text[res.textLength] = '\0';

		// TODO: Error checking would be good here
		int32 fontFilepathLength;
		memory.read<int32>(&fontFilepathLength);
		uint8* fontFilepath = (uint8*)g_memory_allocate(sizeof(uint8) * (fontFilepathLength + 1));
		memory.readDangerous((uint8*)fontFilepath, sizeof(uint8) * fontFilepathLength);
		fontFilepath[fontFilepathLength] = '\0';

		res.font = Fonts::loadFont((const char*)fontFilepath, Application::getNvgContext());
		g_memory_free(fontFilepath);

		return res;
	}

	static LaTexObject deserializeLaTexV1(RawMemory& memory)
	{
		LaTexObject res;

		// textLength       -> i32
		// text             -> char[textLength]
		// isEquation       -> u8 (bool)

		memory.read<int32>(&res.textLength);
		res.text = (char*)g_memory_allocate(sizeof(char) * (res.textLength + 1));
		memory.readDangerous((uint8*)res.text, res.textLength * sizeof(uint8));
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