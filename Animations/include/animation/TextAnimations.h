#ifndef MATH_ANIM_TEXT_ANIMATIONS_H
#define MATH_ANIM_TEXT_ANIMATIONS_H
#include "core.h"
#include "renderer/OrthoCamera.h"

struct NVGcontext;

namespace MathAnim
{
	struct Font;
	struct AnimObject;
	
	namespace TextAnimations
	{
		void init(OrthoCamera& sceneCamera);
	}

	struct TextObject
	{
		float fontSizePixels;
		char* text;
		int32 textLength;
		Font* font;

		void render(NVGcontext* vg, const AnimObject* parent) const;
		void renderWriteInAnimation(NVGcontext* vg, float t, const AnimObject* parent) const;
		void serialize(RawMemory& memory) const;
		void free();

		static TextObject deserialize(RawMemory& memory, uint32 version);
		static TextObject createDefault();
	};

	// TODO: Create some sort of layout machine using MicroTex
	struct LaTexObject
	{
		float fontSizePixels;
		char* text;

		void render(NVGcontext* vg, const AnimObject* parent) const;
		void serialize(RawMemory& memory) const;
		void free();

		static LaTexObject deserialize(RawMemory& memory, uint32 version);
		static LaTexObject createDefault();
	};
}

#endif 