#ifndef MATH_ANIM_LA_TEX_LAYER_H
#define MATH_ANIM_LA_TEX_LAYER_H
#include "core.h"
#include "multithreading/Promise.hpp"

namespace MathAnim
{
	struct SvgObject;

	namespace LaTexLayer
	{
		void init();

		void laTexToSvg(const char* latex, bool isMathTex = false);

		bool laTexIsReady(const char* latex, bool isMathTex = false);

		bool laTexIsReady(const std::string& latex);

		std::string getLaTexMd5(const char* latex);

		std::string getLaTexMd5(const std::string& latex);

		void update();

		void free();
	}
}

#endif 