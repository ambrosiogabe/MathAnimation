#ifndef MATH_ANIM_LA_TEX_LAYER_H
#define MATH_ANIM_LA_TEX_LAYER_H

namespace MathAnim
{
	namespace LaTexLayer
	{
		void init();

		void parseLaTeX(const char* latex, bool isMathTex = false);

		void update();

		void free();
	}
}

#endif 