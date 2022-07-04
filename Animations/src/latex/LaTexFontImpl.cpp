#include "latex/LaTexFontImpl.h"

namespace tex
{

	FontImpl::FontImpl(const std::string& file, float size)
	{
		// load platform-specific font from given file and size
	}

	FontImpl::FontImpl(const std::string& name, int style, float size)
	{
		// create platform-specific font with given name, style
		// and size
	}

	/** Get the font size */
	float FontImpl::getSize() const
	{
		return -1;
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
		return nullptr;
	}

	/** Check if current font equals another */
	bool FontImpl::operator==(const Font& f) const
	{
		return false;
	}

	/** Check if current font not equals another */
	bool FontImpl::operator!=(const Font& f) const
	{
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