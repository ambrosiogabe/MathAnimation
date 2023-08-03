#include "animation/TextAnimations.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"
#include "svg/Svg.h"
#include "renderer/Renderer.h"
#include "renderer/Framebuffer.h"
#include "renderer/Fonts.h"
#include "core/Application.h"
#include "core/Serialization.hpp"
#include "latex/LaTexLayer.h"
#include "editor/panels/SceneHierarchyPanel.h"
#include "parsers/SyntaxTheme.h"

#include <nlohmann/json.hpp>

namespace MathAnim
{
	// Number of spaces for tabs. Make this configurable
	constexpr int tabDepth = 2;

	static void setTextHelper(char** ogText, int32* ogTextLength, const char* newText, size_t newTextLength)
	{
		*ogText = (char*)g_memory_realloc(*ogText, sizeof(char) * (newTextLength + 1));
		*ogTextLength = (int32_t)newTextLength;
		g_memory_copyMem(*ogText, (void*)newText, newTextLength * sizeof(char));
		(*ogText)[newTextLength] = '\0';
	}

	static void setTextHelper(char** ogText, int32* ogTextLength, const char* newText)
	{
		size_t newTextLength = std::strlen(newText);
		return setTextHelper(ogText, ogTextLength, newText, newTextLength);
	}

	static void setTextHelper(char** ogText, int32* ogTextLength, const std::string& newText)
	{
		return setTextHelper(ogText, ogTextLength, newText.c_str(), newText.length());
	}

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

				Vec2 finalOffset = offset + cursorPos;
				childObj._positionStart = Vec3{ finalOffset.x, finalOffset.y, 0.0f };

				// Copy the glyph as the svg object here
				childObj._svgObjectStart = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
				childObj.svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
				*(childObj._svgObjectStart) = Svg::createDefault();
				*(childObj.svgObject) = Svg::createDefault();
				Svg::copy(childObj._svgObjectStart, glyphOutline.svg);
				childObj.retargetSvgScale();

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

	void TextObject::serialize(nlohmann::json& memory) const
	{
		// TODO: This serialization is a bit weird and has special cases
		// See if you can standardize it by storing the font filepath directly or
		// something
		SERIALIZE_NULLABLE_CSTRING(memory, this, text, "Undefined");

		if (font)
		{
			SERIALIZE_NON_NULL_PROP(memory, font, fontFilepath);
		}
		else
		{
			SERIALIZE_NON_NULL_PROP_BY_VALUE(memory, font, fontFilepath, "NullFont");
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

	void TextObject::setText(const std::string& newText)
	{
		setTextHelper(&text, &textLength, newText);
	}

	void TextObject::setText(const char* newText, size_t newTextSize)
	{
		setTextHelper(&text, &textLength, newText, newTextSize);
	}

	void TextObject::setText(const char* newText)
	{
		setTextHelper(&text, &textLength, newText);
	}

	TextObject TextObject::deserialize(const nlohmann::json& j, uint32 version)
	{
		switch (version)
		{
		case 2:
		case 3:
		{
			TextObject res = {};

			DESERIALIZE_NULLABLE_CSTRING(&res, text, j);

			// TODO: This is gross, see note in serialization above
			const std::string& fontFilepathStr = DESERIALIZE_PROP_INLINE(res.font, fontFilepath, j, "NullFont");
			if (fontFilepathStr != "NullFont")
			{
				res.font = Fonts::loadFont(fontFilepathStr.c_str());
			}
			else
			{
				res.font = nullptr;
			}

			return res;
		}
		break;
		default:
			break;
		}

		g_logger_warning("TextObject serialized with unknown version '{}'.", version);
		return {};
	}

	TextObject TextObject::legacy_deserialize(RawMemory& memory, uint32 version)
	{
		if (version == 1)
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

		g_logger_error("Invalid version '{}' while deserializing text object.", version);
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
		childObj._positionStart = Vec3{ 0.0f, 0.0f, 0.0f };

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
		if (!isParsingLaTex)
		{
			setTextHelper(&text, &textLength, str);
		}
	}

	void LaTexObject::setText(const char* cStr)
	{
		if (!isParsingLaTex)
		{
			setTextHelper(&text, &textLength, cStr);
		}
	}

	void LaTexObject::parseLaTex()
	{
		if (!isParsingLaTex)
		{
			LaTexLayer::laTexToSvg(text, isEquation);
			isParsingLaTex = true;
		}
	}

	void LaTexObject::serialize(nlohmann::json& memory) const
	{
		SERIALIZE_NULLABLE_CSTRING(memory, this, text, "Undefined");
		SERIALIZE_NON_NULL_PROP(memory, this, isEquation);
	}

	LaTexObject LaTexObject::legacy_deserialize(RawMemory& memory, uint32 version)
	{
		if (version == 1)
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

		g_logger_error("Invalid version '{}' while deserializing text object.", version);
		LaTexObject res;
		g_memory_zeroMem(&res, sizeof(LaTexObject));
		return res;
	}

	void LaTexObject::free()
	{
		if (text)
		{
			g_memory_free(text);
			text = nullptr;
		}
	}

	LaTexObject LaTexObject::deserialize(const nlohmann::json& j, uint32 version)
	{
		switch (version)
		{
		case 2:
		case 3:
		{
			LaTexObject res = {};

			DESERIALIZE_NULLABLE_CSTRING(&res, text, j);
			DESERIALIZE_PROP(&res, isEquation, j, false);

			return res;
		}
		break;
		default:
			break;
		}

		g_logger_warning("LaTexObject serialized with unknown version '{}'.", version);
		return {};
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
			static bool loggedWarning = false;
			if (!loggedWarning)
			{
				g_logger_warning("No Default Mono Font found. Cannot generate code block.");
				loggedWarning = true;
			}
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
			Vec4 textColor = syntaxTheme->defaultForeground.color;
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

				Vec2 finalOffset = offset + cursorPos;
				childObj._positionStart = Vec3{ finalOffset.x, finalOffset.y, 0.0f };

				// Copy the glyph as the svg object here
				childObj._svgObjectStart = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
				childObj.svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
				*(childObj._svgObjectStart) = Svg::createDefault();
				*(childObj.svgObject) = Svg::createDefault();
				Svg::copy(childObj._svgObjectStart, glyphOutline.svg);
				childObj.retargetSvgScale();

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
		this->init(am, obj->id);
	}

	void CodeBlock::setText(const char* newText)
	{
		setTextHelper(&text, &textLength, newText);
	}

	void CodeBlock::setText(const std::string& newText)
	{
		setTextHelper(&text, &textLength, newText);
	}

	void CodeBlock::serialize(nlohmann::json& memory) const
	{
		SERIALIZE_ENUM(memory, this, theme, _highlighterThemeNames);
		SERIALIZE_ENUM(memory, this, language, _highlighterLanguageNames);
		SERIALIZE_NULLABLE_CSTRING(memory, this, text, "Undefined");
	}

	CodeBlock CodeBlock::legacy_deserialize(RawMemory& memory, uint32 version)
	{
		if (version == 1)
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

		g_logger_error("Invalid version '{}' while deserializing code object.", version);
		CodeBlock res;
		g_memory_zeroMem(&res, sizeof(CodeBlock));
		return res;
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

	CodeBlock CodeBlock::deserialize(const nlohmann::json& j, uint32 version)
	{
		switch (version)
		{
		case 2:
		case 3:
		{
			CodeBlock res = {};

			DESERIALIZE_ENUM(&res, theme, _highlighterThemeNames, HighlighterTheme, j);
			DESERIALIZE_ENUM(&res, language, _highlighterLanguageNames, HighlighterLanguage, j);
			DESERIALIZE_NULLABLE_CSTRING(&res, text, j);

			return res;
		}
		break;
		default:
			break;
		}

		g_logger_warning("CodeBlock serialized with unknown version '{}'.", version);
		return {};
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
}