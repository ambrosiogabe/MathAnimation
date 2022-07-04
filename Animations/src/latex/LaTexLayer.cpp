#include "latex/LaTexLayer.h"

// NOTE: I really hate this, but this library doesn't provide an include
// directory so...
#include <../microtex/src/latex.h>
#include <../microtex/src/render.h>
#include <../microtex/src/graphic/graphic.h>

using namespace tex;

namespace MathAnim
{
	namespace LaTexLayer
	{
		void init()
		{
			LaTeX::init("./Animations/vendor/microtex/res");
			std::wstring code = L"\\int_{now}^{+\\infty} \\text{Keep trying}";
			auto r = LaTeX::parse(
				code,
				720,
				20,
				10,
				BLACK
			);
		}

		void update()
		{

		}

		void free()
		{
			LaTeX::release();
		}
	}
}
