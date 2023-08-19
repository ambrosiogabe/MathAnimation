#ifndef MATH_ANIM_GLAD_LAYER_H
#define MATH_ANIM_GLAD_LAYER_H

namespace MathAnim
{
	struct GlVersion
	{
		int major;
		int minor;
	};

	namespace GladLayer
	{
		void initGlfw();

		GlVersion init();

		void deinit();
	}
}

#endif 