#ifndef MATH_ANIM_LA_TEX_FONT_IMPL_H
#define MATH_ANIM_LA_TEX_FONT_IMPL_H
#include "core.h"

#include <../microtex/src/latex.h>
#include <../microtex/src/graphic/graphic.h>

namespace MathAnim
{
	struct SizedFont;
}

namespace tex
{
	class FontImpl : public tex::Font
	{
	public:

		FontImpl(const std::string& file, float size);
		FontImpl(const std::string& name, int style, float size);
		virtual ~FontImpl() override;

		/** Get the font size */
		virtual float getSize() const override;

		/**
		 * Derive font from current font with given style
		 *
		 * @param style
		 *      required style, defined in enum TypefaceStyle,
		 *      we use integer type to represents style here
		 */
		virtual tex::sptr<tex::Font> deriveFont(int style) const override;

		/** Check if current font equals another */
		virtual bool operator==(const tex::Font& f) const override;

		/** Check if current font not equals another */
		virtual bool operator!=(const tex::Font& f) const override;

	public:
		MathAnim::SizedFont* _font;
	};
}

#endif 