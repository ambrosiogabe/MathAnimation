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
#include "core/Serialization.hpp"
#include "latex/LaTexLayer.h"
#include "editor/panels/SceneHierarchyPanel.h"
#include "parsers/SyntaxTheme.h"

#include <nlohmann/json.hpp>

namespace MathAnim
{
	// ------------- Internal Functions -------------
	static TextObject deserializeTextV2(const nlohmann::json& j);
	static LaTexObject deserializeLaTexV2(const nlohmann::json& j);
	static CodeBlock deserializeCodeBlockV2(const nlohmann::json& j);

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
		memory["Text"] = text;


		if (font != nullptr)
		{
			SERIALIZE_NULLABLE_CSTRING_PROPERTY(memory, font, fontFilepath);
		}
		else
		{
			memory["FontFilepath"] = "nullFont";
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

	TextObject TextObject::deserialize(const nlohmann::json& memory, uint32 version)
	{
		if (version == 2)
		{
			return deserializeTextV2(memory);
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

	void LaTexObject::serialize(nlohmann::json& memory) const
	{
		memory["Text"] = text;
		memory["IsEquation"] = isEquation;
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
		if (version == 2)
		{
			return deserializeLaTexV2(j);
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
		init(am, obj->id);
	}

	void CodeBlock::serialize(nlohmann::json& memory) const
	{
		memory["Theme"] = _highlighterThemeNames[(uint8)theme];
		memory["Language"] = _highlighterLanguageNames[(uint8)language];
		memory["Text"] = text;
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
		if (version == 2)
		{
			return deserializeCodeBlockV2(j);
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
	static TextObject deserializeTextV2(const nlohmann::json& j)
	{
		TextObject res;

		const std::string& textStr = j.contains("Text") ? j["Text"] : "Undefined";
		res.textLength = (int32)textStr.length();
		res.text = (char*)g_memory_allocate(sizeof(char) * (res.textLength + 1));
		g_memory_copyMem(res.text, (void*)textStr.c_str(), sizeof(char) * (res.textLength + 1));

		// TODO: Error checking would be good here
		const std::string& fontFilepathStr = j.contains("FontFilepath") ? j["FontFilepath"] : "nullFont";
		if (fontFilepathStr != "nullFont")
		{
			res.font = Fonts::loadFont(fontFilepathStr.c_str());
		}
		else
		{
			res.font = nullptr;
		}

		return res;
	}

	static LaTexObject deserializeLaTexV2(const nlohmann::json& j)
	{
		LaTexObject res;

		const std::string& textStr = j.contains("Text") ? j["Text"] : "Undefined";
		res.textLength = (int32)textStr.length();
		res.text = (char*)g_memory_allocate(sizeof(char) * (res.textLength + 1));
		g_memory_copyMem(res.text, (void*)textStr.c_str(), sizeof(char) * (res.textLength + 1));

		res.isEquation = j.contains("IsEquation") ? j["IsEquation"] : false;

		return res;
	}

	static CodeBlock deserializeCodeBlockV2(const nlohmann::json& j)
	{
		CodeBlock res;

		const std::string& themeStr = j.contains("Theme") ? j["Theme"] : "Undefined";
		res.theme = findMatchingEnum<HighlighterTheme, (size_t)HighlighterTheme::Length>(_highlighterThemeNames, themeStr);
		const std::string languageStr = j.contains("Language") ? j["Language"] : "Undefined";
		res.language = findMatchingEnum<HighlighterLanguage, (size_t)HighlighterLanguage::Length>(_highlighterLanguageNames, languageStr);

		const std::string& textStr = j.contains("Text") ? j["Text"] : "Undefined";
		res.textLength = (int32)textStr.length();
		res.text = (char*)g_memory_allocate(sizeof(char) * (res.textLength + 1));
		g_memory_copyMem(res.text, (void*)textStr.c_str(), sizeof(char) * (res.textLength + 1));

		return res;
	}
}