#include "latex/LaTexGraphics2DImpl.h"
#include "latex/LaTexFontImpl.h"
#include "renderer/Renderer.h"
#include "renderer/Fonts.h"

#include <nanovg.h>

namespace tex
{
	Graphics2DImpl::Graphics2DImpl(NVGcontext* vg)
	{
		this->vg = vg;
	}

	void Graphics2DImpl::setColor(color c)
	{
		uint8 r = (c >> 24) & 0xFF;
		uint8 g = (c >> 16) & 0xFF;
		uint8 b = (c >> 8) & 0xFF;
		uint8 a = c & 0xFF;
		nvgFillColor(vg, nvgRGBA(r, g, b, a));
		currentColor = c;
	}

	/** Get the color of the context */
	color Graphics2DImpl::getColor() const
	{
		return currentColor;
	}

	/** Set the stroke of the context */
	void Graphics2DImpl::setStroke(const Stroke& s)
	{
		currentStroke = s;
		g_logger_warning("TODO: Implement me.");
	}

	/** Get the stroke of the context */
	const Stroke& Graphics2DImpl::getStroke() const 
	{
		return currentStroke;
	}

	/** Set the stroke width of the context */
	void Graphics2DImpl::setStrokeWidth(float w)
	{
		nvgStrokeWidth(vg, w);
		currentStrokeWidth = w;
	}

	/** Get the current font */
	const Font* Graphics2DImpl::getFont() const
	{
		return currentFont;
	}

	/** Set the font of the context */
	void Graphics2DImpl::setFont(const Font* font)
	{
		// This is gross, but the author of the library does it this way, so it should be fine right?
		currentFont = static_cast<const FontImpl*>(font);
	}

	/**
	 * Translate the context with distance dx, dy
	 *
	 * @param dx distance in x-direction to translate
	 * @param dy distance in y-direction to translate
	 */
	void Graphics2DImpl::translate(float dx, float dy)
	{
		nvgTranslate(vg, dx, dy);
	}

	/**
	 * Scale the context with sx, sy
	 *
	 * @param sx scale ratio in x-direction
	 * @param sy scale ratio in y-direction
	 */
	void Graphics2DImpl::scale(float sx, float sy)
	{
		currentScaleX = sx;
		currentScaleY = sy;
		nvgScale(vg, sx, sy);
	}

	/**
	 * Rotate the context with the given angle (in radian), with pivot (0, 0).
	 *
	 * @param angle angle (in radian) amount to rotate
	 */
	void Graphics2DImpl::rotate(float angle)
	{
		nvgRotate(vg, angle);
	}

	/**
	 * Rotate the context with the given angle (in radian), with pivot (px, py).
	 *
	 * @param angle angle (in radian) amount to rotate
	 * @param px pivot in x-direction
	 * @param py pivot in y-direction
	 */
	void Graphics2DImpl::rotate(float angle, float px, float py)
	{
		g_logger_warning("TODO: This may not work correcly.");
		nvgTranslate(vg, px, py);
		nvgRotate(vg, angle);
	}

	/** Reset transformations */
	void Graphics2DImpl::reset()
	{
		nvgResetTransform(vg);
	}

	/**
	 * Get the scale of the context in x-direction
	 *
	 * @return the scale in x-direction
	 */
	float Graphics2DImpl::sx() const
	{
		return currentScaleX;
	}

	/**
	 * Get the scale of the context in y-direction
	 *
	 * @return the scale in y-direction
	 */
	float Graphics2DImpl::sy() const
	{
		return currentScaleY;
	}

	/**
	 * Draw character, is baseline aligned
	 *
	 * @param c specified character
	 * @param x x-coordinate
	 * @param y y-coordinate, is baseline aligned
	 */
	void Graphics2DImpl::drawChar(wchar_t c, float x, float y)
	{
		std::string str = std::string("") + (char)c;
		//g_logger_warning("TODO: Add unicode support, or implement font rendering using my own library.");
		//MathAnim::Renderer::pushFont(currentFont->_font);
		//MathAnim::Renderer::drawString(std::string("") + asciiC, MathAnim::Vec2{x, y});
		//MathAnim::Renderer::popFont();
		//nvgFontFace(vg, currentFont->_font->unsizedFont->fontFilepath.c_str());
		//nvgFontSize(vg, 128.0f);
		//nvgText(vg, 300.0f, 300.0f, str.c_str(), nullptr);
	}

	/**
	 * Draw text, is baseline aligned
	 *
	 * @param c specified text
	 * @param x x-coordinate
	 * @param y y-coordinate, is baseline aligned
	 */
	void Graphics2DImpl::drawText(const std::wstring& c, float x, float y)
	{
		//g_logger_warning("TODO: Add unicode support, or implement font rendering using my own library.");
		//MathAnim::Renderer::pushFont(currentFont->_font);
		std::string str;
		for (int i = 0; i < c.size(); i++)
		{
			str += c[i];
		}
		//MathAnim::Renderer::drawString(str, MathAnim::Vec2{ x, y });
		//MathAnim::Renderer::popFont();

		nvgFontFace(vg, currentFont->_font->unsizedFont->fontFilepath.c_str());
		nvgText(vg, x, y, str.c_str(), nullptr);
	}

	/**
	 * Draw line
	 *
	 * @param x1 start point in x-direction
	 * @param y1 start point in y-direction
	 * @param x2 end point in x-direction
	 * @param y2 end point in y-direction
	 */
	void Graphics2DImpl::drawLine(float x1, float y1, float x2, float y2)
	{
		g_logger_warning("TODO: Implement me.");
	}

	/**
	 * Draw rectangle
	 *
	 * @param x left position
	 * @param y top position
	 * @param w width
	 * @param h height
	 */
	void Graphics2DImpl::drawRect(float x, float y, float w, float h)
	{
		g_logger_warning("TODO: Implement me.");
	}

	/**
	 * Fill rectangle
	 *
	 * @param x left position
	 * @param y top position
	 * @param w width
	 * @param h height
	 */
	void Graphics2DImpl::fillRect(float x, float y, float w, float h)
	{
		g_logger_warning("TODO: Implement me.");
	}

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
	void Graphics2DImpl::drawRoundRect(float x, float y, float w, float h, float rx, float ry)
	{
		g_logger_warning("TODO: Implement me.");
	}

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
	void Graphics2DImpl::fillRoundRect(float x, float y, float w, float h, float rx, float ry)
	{
		g_logger_warning("TODO: Implement me.");
	}
}  // namespace tex