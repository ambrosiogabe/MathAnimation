#ifndef MATH_ANIM_TEXT_ANIMATIONS_H
#define MATH_ANIM_TEXT_ANIMATIONS_H

struct NVGcontext;

namespace MathAnim
{
	struct Font;
	struct AnimObject;

	struct TextObject
	{
		float fontSizePixels;
		char* text;
		Font* font;

		void render(NVGcontext* vg, const AnimObject* parent) const;
		void renderWriteInAnimation(NVGcontext* vg, float t, const AnimObject* parent) const;
	};

	// TODO: Create some sort of layout machine using MicroTex
	struct LaTexObject
	{
		float fontSizePixels;
		char* text;

		void render(NVGcontext* vg, const AnimObject* parent) const;
	};
}

#endif 