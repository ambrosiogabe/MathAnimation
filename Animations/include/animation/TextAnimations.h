#ifndef MATH_ANIM_TEXT_ANIMATIONS_H
#define MATH_ANIM_TEXT_ANIMATIONS_H
#include "core.h"
#include "renderer/OrthoCamera.h"
#include "parsers/SyntaxHighlighter.h"

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
		void serialize(RawMemory& memory) const;
		void free();

		static TextObject deserialize(RawMemory& memory, uint32 version);
		static TextObject createDefault();
		static TextObject createCopy(const TextObject& from);
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
		void serialize(RawMemory& memory) const;
		void free();

		static LaTexObject deserialize(RawMemory& memory, uint32 version);
		static LaTexObject createDefault();
	};

	struct CodeBlock
	{
		char* text;
		int32 textLength;
		HighlighterLanguage language;
		HighlighterTheme theme;

		void init(AnimationManagerData* am, AnimObjId parentId);
		void reInit(AnimationManagerData* am, AnimObject* obj);
		void serialize(RawMemory& memory) const;
		void free();

		static CodeBlock deserialize(RawMemory& memory, uint32 version);
		static CodeBlock createDefault();
	};
}

#endif 