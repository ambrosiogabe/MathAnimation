#ifndef MATH_ANIM_DEBUG_PANEL_H
#define MATH_ANIM_DEBUG_PANEL_H

namespace MathAnim
{
	struct AnimationManagerData;

	namespace DebugPanel
	{
		void init();

		void update(AnimationManagerData* am);

		void free();
	}
}

#endif 