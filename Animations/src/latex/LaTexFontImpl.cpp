#include "latex/LaTexFontImpl.h"
#include "renderer/Fonts.h"

namespace tex
{

	FontImpl::FontImpl(const std::string& file, float size)
	{
		// load platform-specific font from given file and size
		//_font = MathAnim::Fonts::loadSizedFont(file.c_str(), (int)size);
		_font = nullptr;
	}

	FontImpl::FontImpl(const std::string& name, int style, float size)
	{
		// create platform-specific font with given name, style
		// and size
		g_logger_warning("TODO: Implement loading font with styles!");
	}

	FontImpl::~FontImpl()
	{
		//if (_font)
		//{
		//	MathAnim::Fonts::unloadSizedFont(_font);
		//	_font = nullptr;
		//}
	}

	float FontImpl::getSize() const
	{
		return (float)_font->fontSizePixels;
	}

	/**
	 * Derive font from current font with given style
	 *
	 * @param style
	 *      required style, defined in enum TypefaceStyle,
	 *      we use integer type to represents style here
	 */
	sptr<Font> FontImpl::deriveFont(int style) const
	{
		g_logger_warning("TODO: Implement font styles!");
		return nullptr;
	}

	bool FontImpl::operator==(const tex::Font& f) const
	{
		// TODO: This may become a performance bottleneck... I hate it
		if (const FontImpl* font = dynamic_cast<const FontImpl*>(&f); font != nullptr)
		{
			return font->_font == this->_font;
		}

		return false;
	}

	bool FontImpl::operator!=(const tex::Font& f) const
	{
		// TODO: I may be able to just do a static_cast here because the author does
		// but I hate that even more...
		// TODO: This may become a performance bottleneck... I hate it
		if (const FontImpl* font = dynamic_cast<const FontImpl*>(&f); font != nullptr)
		{
			return font->_font != this->_font;
		}

		return false;
	}

	/**
	 * IMPORTANT: do not forget to implement the 2 static methods below,
	 * it is the factory methods to create a new font.
	 */

	Font* Font::create(const std::string& file, float size)
	{
		return new FontImpl(file, size);
	}

	sptr<Font> Font::_create(const std::string& name, int style, float size)
	{
		return sptrOf<FontImpl>(name, style, size);
	}
}