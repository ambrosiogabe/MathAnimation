#ifndef MATH_ANIM_TEXT_ANIMATIONS_H
#define MATH_ANIM_TEXT_ANIMATIONS_H
#include "core.h"
#include "renderer/OrthoCamera.h"

struct NVGcontext;

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
		int32 numChildrenIds;
		AnimObjId* childrenIds;

		void init(AnimationManagerData* am, AnimObject* obj);
		void reInit(AnimationManagerData* am, AnimObject* obj);
		void serialize(RawMemory& memory) const;
		void free();

		static TextObject deserialize(RawMemory& memory, uint32 version);
		static TextObject createDefault();
	};

	// TODO: Create some sort of layout machine using MicroTex
	struct LaTexObject
	{
		char* text;
		int32 textLength;
		// TODO: Move this into the main animation object and fix everything else
		// to use that
		SvgGroup* svgGroup;
		bool isEquation;
		bool isParsingLaTex;

		// NOTE: This update function is just used to check if it's
		// in the middle of parsing latex. If it is, then it checks
		// if it's ready and then assigns it to the SVG group when it's done
		// generating the svg
		void update();

		void setText(const std::string& str);
		void setText(const char* str);
		void parseLaTex();
		void render(NVGcontext* vg, const AnimObject* parent) const;
		void renderCreateAnimation(NVGcontext* vg, float t, const AnimObject* parent, bool reverse) const;
		void serialize(RawMemory& memory) const;
		void free();

		static LaTexObject deserialize(RawMemory& memory, uint32 version);
		static LaTexObject createDefault();
	};
}

#endif 