#include "latex/LaTexTextLayoutImpl.h"
#include "latex/LaTexFontImpl.h"
#include "renderer/Fonts.h"

namespace tex
{
	TextLayoutImpl::TextLayoutImpl(const std::wstring& src, const sptr<tex::Font>& font)
	{
		//_layout->set_text(wide2utf8(src));
		//_layout->set_font_description(fd);
		this->font = font;
		this->text = wide2utf8(src);
		wtext = src;
	}

	void TextLayoutImpl::getBounds(Rect& r)
	{
		int w, h;
		FontImpl* f = (FontImpl*)(&font);
		glm::vec2 size = f->_font->getSizeOfString(text);
		r.x = 0;
		r.y = 0;
		r.w = size.x;
		r.h = size.y;
	}

	void TextLayoutImpl::draw(Graphics2D& g2, float x, float y)
	{
		g2.drawLine(x, y, x + 1, y);
		g2.drawText(wtext, x, y);
	}

	sptr<TextLayout> TextLayout::create(const std::wstring& src, const sptr<tex::Font>& font)
	{
		return sptrOf<TextLayoutImpl>(src, font);
	}
};