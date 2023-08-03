#ifndef MATH_ANIM_TEXT_ANIMATIONS_H
#define MATH_ANIM_TEXT_ANIMATIONS_H
#include "core.h"
#include "parsers/SyntaxHighlighter.h"

#include <nlohmann/json_fwd.hpp>

namespace MathAnim
{
	struct Font;
	struct AnimObject;
	struct SvgGroup;
	struct AnimationManagerData;

	struct TextObject
	{
		char* text;
		int32 textLength;
		Font* font;

		void init(AnimationManagerData* am, AnimObjId parentId);
		void reInit(AnimationManagerData* am, AnimObject* obj);
		void free();

		void setText(const std::string& newText);
		void setText(const char* newText, size_t newTextSize);
		void setText(const char* newText);

		void serialize(nlohmann::json& j) const;
		static TextObject deserialize(const nlohmann::json& j, uint32 version);

		static TextObject createDefault();

		[[deprecated("This is for upgrading legacy projects developed in beta")]]
		static TextObject legacy_deserialize(RawMemory& memory, uint32 version);
	};

	// TODO: Create some sort of layout machine using MicroTex
	struct LaTexObject
	{
		char* text;
		int32 textLength;
		bool isEquation;
		bool isParsingLaTex;

		// NOTE: This update function is just used to check if it's
		// in the middle of parsing latex. If it is, then it checks
		// if it's ready and then assigns it to the SVG group when it's done
		// generating the svg
		void update(AnimationManagerData* am, AnimObjId parentId);

		void init(AnimationManagerData* am, AnimObjId parentId);
		void reInit(AnimationManagerData* am, AnimObject* obj);

		void setText(const std::string& str);
		void setText(const char* str);
		void parseLaTex();
		void free();

		void serialize(nlohmann::json& j) const;
		static LaTexObject deserialize(const nlohmann::json& j, uint32 version);

		static LaTexObject createDefault();

		[[deprecated("This is for upgrading legacy projects developed in beta")]]
		static LaTexObject legacy_deserialize(RawMemory& memory, uint32 version);
	};

	struct CodeBlock
	{
		char* text;
		int32 textLength;
		HighlighterLanguage language;
		HighlighterTheme theme;

		void init(AnimationManagerData* am, AnimObjId parentId);
		void reInit(AnimationManagerData* am, AnimObject* obj);

		void setText(const char* newText);
		void setText(const std::string& newText);

		void free();

		void serialize(nlohmann::json& j) const;
		static CodeBlock deserialize(const nlohmann::json& j, uint32 version);

		static CodeBlock createDefault();

		[[deprecated("This is for upgrading legacy projects developed in beta")]]
		static CodeBlock legacy_deserialize(RawMemory& memory, uint32 version);
	};
}

#endif 