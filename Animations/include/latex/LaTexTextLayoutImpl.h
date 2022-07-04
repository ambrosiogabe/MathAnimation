#ifndef MATH_ANIM_LA_TEX_TEXT_LAYOUT_IMPL_H
#define MATH_ANIM_LA_TEX_TEXT_LAYOUT_IMPL_H
#include "core.h"

#include <../microtex/src/latex.h>
#include <../microtex/src/graphic/graphic.h>

namespace tex
{

	/** To layout the text that the program cannot recognize. */
	class TextLayoutImpl : tex::TextLayout
	{
	public:
		/**
		 * Get the layout bounds with current text and font
		 *
		 * @param bounds rectangle to retrieve the bounds
		 */
		virtual void getBounds(Rect& bounds) override;

		/**
		 * Draw the layout
		 *
		 * @param g2 the graphics (2D) context
		 * @param x the x coordinate
		 * @param y the y coordinate, is baseline aligned
		 */
		virtual void draw(Graphics2D& g2, float x, float y) override;

		/**
		 * Create a TextLayout with given text and font
		 *
		 * @param src the text to layout
		 * @param font the specified font
		 * @return new TextLayout
		 */
		static sptr<TextLayout> create(const std::wstring& src, const sptr<tex::Font>& font);
	};
}

#endif 