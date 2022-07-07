#ifndef LA_TEX_GRAPHICS_2D_IMPL_H
#define LA_TEX_GRAPHICS_2D_IMPL_H
#include "core.h"

#include <../microtex/src/latex.h>
#include <../microtex/src/graphic/graphic.h>

struct NVGcontext;

namespace tex
{
	class FontImpl;

	/**
	 * Abstract class to represents a graphics (2D) context, all the TeX drawing operations will on it.
	 * It must have scale, translation, and rotation support. You should notice that the scaling on
	 * y-direction will be selected as the base if they are different on x and y-direction when drawing
	 * characters. In most cases, you should never use different scalings, unless you are really sure the
	 * coordinates are correct (i.e. draw a hyphen).
	 */
	class Graphics2DImpl : public tex::Graphics2D
	{
	public:
		Graphics2DImpl(NVGcontext* vg);

		/**
		 * Set the color of the context
		 *
		 * @param c required color
		 */
		virtual void setColor(color c) override;

		/** Get the color of the context */
		virtual color getColor() const override;

		/** Set the stroke of the context */
		virtual void setStroke(const Stroke& s) override;

		/** Get the stroke of the context */
		virtual const Stroke& getStroke() const override;

		/** Set the stroke width of the context */
		virtual void setStrokeWidth(float w) override;

		/** Get the current font */
		virtual const Font* getFont() const override;

		/** Set the font of the context */
		virtual void setFont(const Font* font) override;

		/**
		 * Translate the context with distance dx, dy
		 *
		 * @param dx distance in x-direction to translate
		 * @param dy distance in y-direction to translate
		 */
		virtual void translate(float dx, float dy) override;

		/**
		 * Scale the context with sx, sy
		 *
		 * @param sx scale ratio in x-direction
		 * @param sy scale ratio in y-direction
		 */
		virtual void scale(float sx, float sy) override;

		/**
		 * Rotate the context with the given angle (in radian), with pivot (0, 0).
		 *
		 * @param angle angle (in radian) amount to rotate
		 */
		virtual void rotate(float angle) override;

		/**
		 * Rotate the context with the given angle (in radian), with pivot (px, py).
		 *
		 * @param angle angle (in radian) amount to rotate
		 * @param px pivot in x-direction
		 * @param py pivot in y-direction
		 */
		virtual void rotate(float angle, float px, float py) override;

		/** Reset transformations */
		virtual void reset() override;

		/**
		 * Get the scale of the context in x-direction
		 *
		 * @return the scale in x-direction
		 */
		virtual float sx() const override;

		/**
		 * Get the scale of the context in y-direction
		 *
		 * @return the scale in y-direction
		 */
		virtual float sy() const override;

		/**
		 * Draw character, is baseline aligned
		 *
		 * @param c specified character
		 * @param x x-coordinate
		 * @param y y-coordinate, is baseline aligned
		 */
		virtual void drawChar(wchar_t c, float x, float y) override;

		/**
		 * Draw text, is baseline aligned
		 *
		 * @param c specified text
		 * @param x x-coordinate
		 * @param y y-coordinate, is baseline aligned
		 */
		virtual void drawText(const std::wstring& c, float x, float y) override;

		/**
		 * Draw line
		 *
		 * @param x1 start point in x-direction
		 * @param y1 start point in y-direction
		 * @param x2 end point in x-direction
		 * @param y2 end point in y-direction
		 */
		virtual void drawLine(float x1, float y1, float x2, float y2) override;

		/**
		 * Draw rectangle
		 *
		 * @param x left position
		 * @param y top position
		 * @param w width
		 * @param h height
		 */
		virtual void drawRect(float x, float y, float w, float h) override;

		/**
		 * Fill rectangle
		 *
		 * @param x left position
		 * @param y top position
		 * @param w width
		 * @param h height
		 */
		virtual void fillRect(float x, float y, float w, float h) override;

		/**
		 * Draw round rectangle
		 *
		 * @param x left position
		 * @param y top position
		 * @param w width
		 * @param h height
		 * @param rx radius in x-direction
		 * @param ry radius in y-direction
		 */
		virtual void drawRoundRect(float x, float y, float w, float h, float rx, float ry) override;

		/**
		 * Fill round rectangle
		 *
		 * @param x left position
		 * @param y top position
		 * @param w width
		 * @param h height
		 * @param rx radius in x-direction
		 * @param ry radius in y-direction
		 */
		virtual void fillRoundRect(float x, float y, float w, float h, float rx, float ry) override;

	private:
		NVGcontext* vg;
		color currentColor;
		Stroke currentStroke;
		float currentStrokeWidth;
		float currentScaleX;
		float currentScaleY;
		const FontImpl* currentFont = nullptr;
	};

}  // namespace tex

#endif