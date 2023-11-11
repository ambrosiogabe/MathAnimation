#ifndef MATH_ANIM_SYNTAX_THEME_H
#define MATH_ANIM_SYNTAX_THEME_H
#include "parsers/Common.h"
#include "svg/Styles.h"

#include <cppUtils/cppPrint.hpp>
#include <cppUtils/cppUtils.hpp>

namespace MathAnim
{
	struct PackedSyntaxStyle
	{
		/**
		 * Modified from: https://code.visualstudio.com/blogs/2017/02/08/syntax-highlighting-optimizations#_changes-to-tokenization
		 * - -------------------------------------------
		 *     xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx
		 *     bbbb bbbb bfff ffff ffFF FTTT LLLL LLLL
		 * - -------------------------------------------
		 *  - L = LanguageId (8 bits)        -> Lookup from language map
		 *  - T = StandardTokenType (3 bits) -> Maps to standard token type enum
		 *  - F = FontStyle (3 bits)         -> Maps to font style enum
		 *  - f = foreground color (9 bits)  -> Lookup from color map
		 *  - b = background color (9 bits)  -> Lookup from color map
		 * 
		 * For colors there are two reserved indices:
		 *   0 = bad color
		 *   1 = inherited
		 */
		struct bits
		{
			uint32 _backgroundColor : 9;
			uint32 _foregroundColor : 9;
			uint32 _fontStyle : 3;
			uint32 _standardTokenType : 3;
			uint32 _languageId : 8;
		};
		struct bits metadata;

#ifndef UNDEFINED_TOKEN_BITFIELD
		static_assert(sizeof(bits) == 4, "This compiler is packing this bitfield weird, expected 32 bits and got something else. Define the macro UNDEFINED_TOKEN_BITFIELD to compile this program and avoid this error.");
#endif

		void mergeWith(PackedSyntaxStyle other);
		void overwriteMergeWith(PackedSyntaxStyle other);

		// TODO: This is probably unsafe, find a portable way to check if the bitfield is 0
		inline bool isEmpty() { return *((uint32*)&metadata) == 0; }

		inline void setBackgroundColorInherited() { metadata._backgroundColor = 1; }
		inline void setForegroundColorInherited() { metadata._foregroundColor = 1; }

		inline void setBackgroundColor(uint32 color)
		{
			g_logger_assert(color < (1 << 9), "Invalid background color. Maximum of '{}' colors only is allowed.", (1 << 9));
			metadata._backgroundColor = color;
		}

		inline void setForegroundColor(uint32 color)
		{
			g_logger_assert(color < (1 << 9), "Invalid foreground color. Maximum of '{}' colors only is allowed.", (1 << 9));
			metadata._foregroundColor = color;
		}

		inline void setFontStyle(CssFontStyle fontStyle)
		{
			g_logger_assert((uint32)fontStyle < (1 << 3), "Invalid font style '{}'. Out of range for metadata, can only be 8 unique font styles.", fontStyle);
			metadata._fontStyle = (uint32)fontStyle;
		}

		inline void setStandardTokenType(uint32 tokenType)
		{
			g_logger_assert(tokenType < (1 << 3), "Invalid standard token type. Maximum allowed is '{}'.", (1 << 3));
			metadata._standardTokenType = tokenType;
		}

		inline void setLanguageId(uint32 languageId)
		{
			g_logger_assert(languageId < (1 << 8), "Invalid language ID. Maximum allowed is '{}'.", (1 << 8));
			metadata._languageId = languageId;
		}

		inline uint32 getBackgroundColor() const { return metadata._backgroundColor; }
		inline uint32 getForegroundColor() const { return metadata._foregroundColor; }
		inline bool isBackgroundInherited() const { return metadata._backgroundColor == 1; }
		inline bool isForegroundInherited() const { return metadata._foregroundColor == 1; }

		inline CssFontStyle getFontStyle() const
		{
			g_logger_assert(metadata._fontStyle >= 0 && metadata._fontStyle < (uint32)CssFontStyle::Length, "Invalid font style '{}' set on metadata.", metadata._fontStyle);
			return (CssFontStyle)metadata._fontStyle;
		}

		inline uint32 getStandardTokenType() const { return metadata._standardTokenType; }
		inline uint32 getLanguageId() const { return metadata._languageId; }
	};

	struct SyntaxTrieParentRule
	{
		std::vector<ScopeSelector> ancestors;
		PackedSyntaxStyle style;
		PackedSyntaxStyle inheritedStyle;
	};

	struct SyntaxTheme;
	struct SyntaxTrieNode
	{
		std::string name;
		PackedSyntaxStyle inheritedStyle;
		PackedSyntaxStyle style;
		std::vector<SyntaxTrieParentRule> parentRules;

		// Map from sub-scope to child
		// Ex. In the scope "var.identifier", the var node would have one child in the map:
		//       <"identifier", Node>
		std::unordered_map<std::string, SyntaxTrieNode> children;

		void insert(std::string const& name, ScopeSelector const& selector, PackedSyntaxStyle const& style, std::vector<ScopeSelector> const& ancestors = {}, size_t subScopeIndex = 0);
		void calculateInheritedStyles(PackedSyntaxStyle inInheritedStyle = {});

		void print(SyntaxTheme const& theme) const;
	};

	struct DebugPackedSyntaxStyle
	{
		PackedSyntaxStyle style;
		std::string styleMatched;
	};

	struct SyntaxTheme
	{
		uint32 defaultForeground;
		uint32 defaultBackground;

		// TODO: Switch to this, once we verify it's working correctly
		SyntaxTrieNode root;
		std::unordered_map<std::string, uint32> colorMap;
		std::vector<Vec4> colors;

		PackedSyntaxStyle match(const std::vector<ScopedName>& ancestorScopes) const;
		DebugPackedSyntaxStyle debugMatch(const std::vector<ScopedName>& ancestorScopes, bool* usingDefaultSettings = nullptr) const;

		uint32 getOrCreateColorIndex(std::string const& colorStr, Vec4 const& color);
		Vec4 const& getColor(uint32 id) const;

		static SyntaxTheme* importTheme(const char* filepath);
		static void free(SyntaxTheme* theme);
	};
}

#endif 