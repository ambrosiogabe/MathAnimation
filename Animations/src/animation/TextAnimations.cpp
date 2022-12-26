#include "animation/TextAnimations.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"
#include "svg/Svg.h"
#include "renderer/Renderer.h"
#include "renderer/Framebuffer.h"
#include "renderer/Fonts.h"
#include "renderer/PerspectiveCamera.h"
#include "renderer/OrthoCamera.h"
#include "core/Application.h"
#include "latex/LaTexLayer.h"
#include "editor/SceneHierarchyPanel.h"
#include "parsers/SyntaxTheme.h"

namespace MathAnim
{
	// ------------- Internal Functions -------------
	static TextObject deserializeTextV1(RawMemory& memory);
	static LaTexObject deserializeLaTexV1(RawMemory& memory);
	static CodeBlock deserializeCodeBlockV1(RawMemory& memory);

	// Number of spaces for tabs. Make this configurable
	constexpr int tabDepth = 2;

	void TextObject::init(AnimationManagerData* am, AnimObjId parentId)
	{
		if (font == nullptr)
		{
			return;
		}

		std::string textStr = std::string(text);

		// Generate children that represent each character of the text object `obj`
		Vec2 cursorPos = Vec2{ 0, 0 };
		for (int i = 0; i < textStr.length(); i++)
		{
			if (textStr[i] == '\n')
			{
				cursorPos = Vec2{ 0.0f, cursorPos.y - font->lineHeight };
				continue;
			}

			uint8 codepoint = (uint8)textStr[i];
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

			if (textStr[i] != ' ' && textStr[i] != '\t')
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
		obj->generatedChildrenIds.clear();

		// Next init again which should regenerate the children
		init(am, obj->id);
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
		if (from.font)
		{
			// NOTE: We can pass nullptr for vg here because it shouldn't actually need it
			res.font = Fonts::loadFont(from.font->fontFilepath.c_str());
		}
		else
		{
			res.font = nullptr;
		}

		res.text = (char*)g_memory_allocate((from.textLength + 1) * sizeof(char));
		g_memory_copyMem(res.text, (void*)from.text, from.textLength);
		res.textLength = from.textLength;
		res.text[res.textLength] = '\0';
		return res;
	}

	void LaTexObject::update(AnimationManagerData* am, AnimObjId parentId)
	{
		if (isParsingLaTex)
		{
			if (LaTexLayer::laTexIsReady(text, isEquation))
			{
				AnimObject* parent = AnimationManager::getMutableObject(am, parentId);
				if (parent)
				{
					reInit(am, parent);
				}
				isParsingLaTex = false;
			}
		}
	}

	void LaTexObject::init(AnimationManagerData* am, AnimObjId parentId)
	{
		// TODO: Memory leak somewhere in here

		std::string filepath = "latex/" + LaTexLayer::getLaTexMd5(text) + ".svg";

		// Add this character as a child
		AnimObject childObj = AnimObject::createDefaultFromParent(am, AnimObjectTypeV1::SvgFileObject, parentId, true);
		childObj.parentId = parentId;
		childObj._positionStart = Vec3{ 0.0f, 0.0f, 0.0f };
		childObj.isGenerated = true;

		AnimationManager::addAnimObject(am, childObj);
		// TODO: Ugly what do I do???
		SceneHierarchyPanel::addNewAnimObject(childObj);

		childObj.as.svgFile.setFilepath(filepath);
		childObj.as.svgFile.init(am, childObj.id);
	}
		
	void LaTexObject::reInit(AnimationManagerData* am, AnimObject* obj)
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
		obj->generatedChildrenIds.clear();

		// Next init again which should regenerate the children
		init(am, obj->id);
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
		setText(std::string(cStr));
	}

	void LaTexObject::parseLaTex()
	{
		LaTexLayer::laTexToSvg(text, isEquation);
		isParsingLaTex = true;
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
		res.isParsingLaTex = false;
		res.parseLaTex();

		return res;
	}

	void CodeBlock::init(AnimationManagerData* am, AnimObjId parentId)
	{
		Font* font = Fonts::getDefaultMonoFont();
		if (font == nullptr)
		{
			return;
		}

		const SyntaxHighlighter* highlighter = Highlighters::getHighlighter(this->language);
		if (!highlighter)
		{
			return;
		}

		const SyntaxTheme* syntaxTheme = Highlighters::getTheme(this->theme);
		if (!syntaxTheme)
		{
			return;
		}

		// First parse the code block and get the code in segmented form with highlight information
		CodeHighlights highlights = highlighter->parse(text, *syntaxTheme);

		// Generate children that represent each character of the text object `obj`
		Vec2 cursorPos = Vec2{ 0, 0 };
		size_t codeBlockCursor = 0;
		for (size_t textIndex = 0; textIndex < (size_t)textLength; textIndex++)
		{
			Vec4 textColor = syntaxTheme->defaultForeground;
			if (codeBlockCursor < highlights.segments.size())
			{
				if (textIndex >= highlights.segments[codeBlockCursor].endPos)
				{
					codeBlockCursor++;
				}
			}

			if (codeBlockCursor < highlights.segments.size())
			{
				textColor = highlights.segments[codeBlockCursor].color;
			}

			if (text[textIndex] == '\n')
			{
				cursorPos = Vec2{ 0.0f, cursorPos.y - font->lineHeight };
				continue;
			}

			uint8 codepoint = (uint8)text[textIndex];
			bool isTab = codepoint == '\t';
			if (codepoint == '\t')
			{
				codepoint = ' ';
			}

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

			if (text[textIndex] != ' ' && text[textIndex] != '\t')
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

				childObj._fillColorStart = glm::u8vec4(
					(uint8)(textColor.r * 255.0f),
					(uint8)(textColor.g * 255.0f),
					(uint8)(textColor.b * 255.0f),
					(uint8)(textColor.a * 255.0f)
				);
				childObj.fillColor = childObj._fillColorStart;
				childObj._strokeColorStart = childObj._fillColorStart;
				childObj.strokeColor = childObj._fillColorStart;

				childObj.name = (uint8*)g_memory_realloc(childObj.name, sizeof(uint8) * 2);
				childObj.nameLength = 1;
				childObj.name[0] = codepoint;
				childObj.name[1] = '\0';

				AnimationManager::addAnimObject(am, childObj);
				// TODO: Ugly what do I do???
				SceneHierarchyPanel::addNewAnimObject(childObj);
			}

			// TODO: I may have to add kerning info here
			float advance = glyphOutline.advanceX;
			if (isTab)
			{
				advance *= tabDepth;
			}
			cursorPos += Vec2{ advance, 0.0f };
		}
	}

	void CodeBlock::reInit(AnimationManagerData* am, AnimObject* obj)
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
		obj->generatedChildrenIds.clear();

		// Next init again which should regenerate the children
		init(am, obj->id);
	}

	void CodeBlock::serialize(RawMemory& memory) const
	{
		// theme                -> uint8
		// language             -> uint8
		// textLength           -> int32
		// text                 -> char[textLength]
		memory.write<HighlighterTheme>(&theme);
		memory.write<HighlighterLanguage>(&language);

		memory.write<int32>(&textLength);
		memory.writeDangerous((uint8*)text, sizeof(uint8) * textLength);
	}

	void CodeBlock::free()
	{
		if (this->text)
		{
			g_memory_free(this->text);
			this->text = nullptr;
			this->textLength = 0;
		}
	}

	CodeBlock CodeBlock::deserialize(RawMemory& memory, uint32 version)
	{
		if (version == 1)
		{
			return deserializeCodeBlockV1(memory);
		}

		g_logger_error("Invalid version '%d' while deserializing code object.", version);
		CodeBlock res;
		g_memory_zeroMem(&res, sizeof(CodeBlock));
		return res;
	}

	CodeBlock CodeBlock::createDefault()
	{
		CodeBlock res;
		res.language = HighlighterLanguage::Cpp;
		res.theme = HighlighterTheme::MonokaiNight;
		static const char defaultText[] = R"DEFAULT_LANG(#include <stdio.h>

int main()
{
	printf("Hello World!");
	return 0;
}
)DEFAULT_LANG";
		res.text = (char*)g_memory_allocate(sizeof(defaultText) / sizeof(char));
		g_memory_copyMem(res.text, (void*)defaultText, sizeof(defaultText) / sizeof(char));
		res.textLength = (sizeof(defaultText) / sizeof(char)) - 1;
		res.text[res.textLength] = '\0';
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

		res.font = Fonts::loadFont((const char*)fontFilepath);
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
		res.text[res.textLength] = '\0';

		uint8 isEquationU8;
		memory.read<uint8>(&isEquationU8);
		res.isEquation = isEquationU8 == 1;
		res.isParsingLaTex = false;

		return res;
	}

	static CodeBlock deserializeCodeBlockV1(RawMemory& memory)
	{
		CodeBlock res;

		// theme                -> uint8
		// language             -> uint8
		// textLength           -> int32
		// text                 -> char[textLength]

		memory.read<HighlighterTheme>(&res.theme);
		memory.read<HighlighterLanguage>(&res.language);

		memory.read<int32>(&res.textLength);
		res.text = (char*)g_memory_allocate(sizeof(char) * (res.textLength + 1));
		memory.readDangerous((uint8*)res.text, sizeof(uint8) * res.textLength);
		res.text[res.textLength] = '\0';

		return res;
	}
}