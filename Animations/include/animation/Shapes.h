#ifndef MATH_ANIM_SHAPES_H
#define MATH_ANIM_SHAPES_H

struct NVGcontext;

namespace MathAnim
{
	struct AnimObject;

	struct Square
	{
		float sideLength;

		void init(AnimObject* parent);
	};
}

#endif 