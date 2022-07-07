#include "latex/LaTexLayer.h"
#include "latex/LaTexFontImpl.h"
#include "latex/LaTexGraphics2DImpl.h"
#include "latex/LaTexTextLayoutImpl.h"
#include "core/Application.h"

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
		static tex::Graphics2DImpl* g2;
		static tex::TeXRender* tmpTexRender;

		void init()
		{
			LaTeX::init("./Animations/vendor/microtex/res");
			g2 = new tex::Graphics2DImpl(Application::getNvgContext());

			std::wstring code = L"\\int_{now}^{+\\infty} \\text{Keep trying}";
			tmpTexRender = LaTeX::parse(
				code,
				720,
				20,
				10,
				BLACK
			);
		}

		void update()
		{
			tmpTexRender->draw(*g2, 20, 20);
		}

		void free()
		{
			delete g2;
			LaTeX::release();
		}
	}
}
