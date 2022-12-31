#include "svg/SvgParser.h"
#include "svg/Svg.h"

#include <../tinyxml2/tinyxml2.h>

using namespace tinyxml2;

#define PANIC(formatStr, ...) \
	snprintf(errorBuffer, errorBufferSize, formatStr, __VA_ARGS__); \
	g_logger_error(formatStr, __VA_ARGS__);
#define PANIC_NOFMT(str) \
	snprintf(errorBuffer, errorBufferSize, str); \
	g_logger_error(str);

namespace MathAnim
{
	enum class SvgElementType : uint8
	{
		None = 0,
		Path,
		Title,
		Desc,
		Use,
		Rect,
		Polygon,
		Style,
		Defs
	};

	struct ParserInfo
	{
		const char* text;
		size_t textLength;
		size_t cursor;
	};

	enum class PathTokenType : uint8
	{
		Panic = 0,
		MoveTo,
		ClosePath,
		LineTo,
		HzLineTo,
		VtLineTo,
		CurveTo,
		SmoothCurveTo,
		QuadCurveTo,
		SmoothQuadCurveTo,
		ArcTo,
		Number,
		EndOfFile,
		Length
	};

	struct PathToken
	{
		PathTokenType type;
		bool isAbsolute;
		union
		{
			float number;
		} as;
	};

	enum class StyleTokenType : uint8
	{
		Panic = 0,
		Identifier, // [a-zA-Z\-_]+
		HashtagIdentifier,
		DotIdentifier,
		String,
		Color,
		Number,
		NumberPixels,
		LeftCurlyBracket,
		RightCurlyBracket,
		LeftParen,
		RightParen,
		Colon,
		Semicolon,
		EndOfFile,
		Length
	};

	struct DumbString
	{
		char* text;
		size_t textLength;

		DumbString copy() const
		{
			DumbString res = {};
			res.text = (char*)g_memory_allocate(sizeof(char) * (textLength + 1));
			res.textLength = textLength;
			g_memory_copyMem(res.text, text, (textLength + 1) * sizeof(char));
			return res;
		}

		void free()
		{
			if (text)
			{
				g_memory_free(text);
			}

			text = nullptr;
			textLength = 0;
		}
	};

	struct StyleToken
	{
		StyleTokenType type;
		bool isAbsolute;
		union
		{
			float number;
			Vec4 color;
			DumbString identifier;
		} as;

		void free()
		{
			if (type == StyleTokenType::Identifier ||
				type == StyleTokenType::HashtagIdentifier ||
				type == StyleTokenType::DotIdentifier)
			{
				as.identifier.free();
			}
		}
	};

	struct ArcParams
	{
		Vec2 radius;
		float xAxisRotation;
		bool largeArcFlag;
		bool sweepFlag;
		Vec2 endpoint;
	};

	enum class StyleAttributeType : uint8
	{
		None = 0,
		Color,
		Identifier,
		Number,
		IdentifierOrString,
		Style
	};

	class StyleAttribute
	{
	public:
		StyleAttribute()
			: _name("UNDEFINED"), number(NAN), type(StyleAttributeType::None),
			color(Vec4{ NAN, NAN, NAN, NAN }), identifier("")
		{
		}

		StyleAttribute(const std::string& attributeName, float asNumber)
			: _name(attributeName), number(asNumber), type(StyleAttributeType::Number),
			color(Vec4{ NAN, NAN, NAN, NAN }), identifier("")
		{
		}

		StyleAttribute(const std::string& attributeName, Vec4 asColor)
			: _name(attributeName), color(asColor), type(StyleAttributeType::Color),
			number(NAN), identifier("")
		{
		}

		StyleAttribute(const std::string& attributeName, const std::string& asIdentifier)
			: _name(attributeName), identifier(asIdentifier), type(StyleAttributeType::Identifier),
			color(Vec4{ NAN, NAN, NAN, NAN }), number(NAN)
		{
		}

		std::string value() const
		{
			if (isIdentifier())
			{
				return identifier;
			}

			if (isColor())
			{
				return "Color<R:" + std::to_string((int)(color.r * 255.0f)) +
					", G:" + std::to_string((int)(color.g * 255.0f)) +
					", B:" + std::to_string((int)(color.b * 255.0f)) +
					", A:" + std::to_string((int)(color.a * 255.0f)) + ">";
			}

			if (isNumber())
			{
				return std::to_string(number);
			}

			return "UNDEFINED";
		}

		inline const std::string& name() const { return _name; }
		inline bool isIdentifier() const { return type == StyleAttributeType::Identifier; }
		inline bool isColor() const { return type == StyleAttributeType::Color; }
		inline bool isNumber() const { return type == StyleAttributeType::Number; }

		const std::string& asIdentifier() const
		{
			if (isIdentifier())
			{
				return identifier;
			}

			return "";
		}

		const Vec4& asColor() const
		{
			if (isColor())
			{
				return color;
			}

			return "#fc03f8"_hex;
		}

		float asNumber() const
		{
			if (isNumber())
			{
				return number;
			}

			return NAN;
		}

	private:
		StyleAttributeType type;
		std::string _name;
		std::string identifier;
		float number;
		Vec4 color;
	};

	struct Style
	{
		std::string className;
		std::string idName;
		std::vector<StyleAttribute> attributes;
	};

	struct Stylesheet
	{
		std::unordered_map<std::string, Style> idMap;
		std::unordered_map<std::string, Style> classMap;
	};

	namespace SvgParser
	{
		// Internal variables
		static constexpr size_t errorBufferSize = 1024;
		static char errorBuffer[errorBufferSize];

		// ----------- Internal Functions -----------
		static bool parseSvgStylesheet(XMLElement* element, Stylesheet* output);
		static bool parseSvgPathTag(XMLElement* element, SvgObject* output, const Stylesheet& styles);
		static bool parsePolygonTag(XMLElement* element, SvgObject* output, const Stylesheet& styles);
		static void applyElementStylesToSvg(XMLElement* element, SvgObject* output, const Stylesheet& styles);
		static void applyStylesTo(SvgObject* output, const Style& style);
		static void applyStyleAttributeTo(SvgObject* output, const StyleAttribute& attribute);
		static SvgElementType elementStringToEnum(const std::string& elementName);

		// -------- Path Parser --------
		static bool interpretCommand(const PathToken& token, ParserInfo& parserInfo, SvgObject* res);
		static bool parseVec2List(std::vector<Vec2>& list, ParserInfo& parserInfo);
		static bool parseHzNumberList(std::vector<float>& list, ParserInfo& parserInfo);
		static bool parseVtNumberList(std::vector<float>& list, ParserInfo& parserInfo);
		static bool parseArcParamsList(std::vector<ArcParams>& list, ParserInfo& parserInfo);
		static bool parseViewbox(Vec4* out, const char* viewboxStr);
		static PathToken parseNextPathToken(ParserInfo& parserInfo);
		static PathToken consume(PathTokenType expected, ParserInfo& parserInfo);

		// -------- Svg Style Parser --------
		static StyleToken parseNextStyleToken(ParserInfo& parserInfo);
		static bool parseStyle(ParserInfo& parserInfo, Stylesheet* res, const StyleToken& identifier);
		static bool parseInlineStyle(ParserInfo& parserInfo, Style* res, bool consumeClosingCurlyBracket);
		static StyleToken consume(StyleTokenType expected, ParserInfo& parserInfo);
		static StyleToken consumeChar(char expectedChar, StyleTokenType resType, ParserInfo& parserInfo);
		static void printStyleToken(StyleToken& token);
		static bool parseColor(const std::string& string, Vec4* output);
		static bool sanitizeAttribute(const StyleToken& attributeName, const StyleToken& attributeValue, StyleAttribute* output);
		static bool sanitizeAttribute(const XMLAttribute* attribute, StyleAttribute* output);

		// -------- Generic Parser Stuff --------
		static bool parseNumber(ParserInfo& parserInfo, float* out);
		static bool parseString(ParserInfo& parserInfo, DumbString* string, char stringTerminator);
		static bool parseIdentifier(ParserInfo& parserInfo, DumbString* string, bool canStartWithNumber = false);
		static void skipWhitespaceAndCommas(ParserInfo& parserInfo);
		static void skipWhitespace(ParserInfo& parserInfo);

		// -------- Helpers --------
		static inline char advance(ParserInfo& parserInfo) { char c = parserInfo.cursor < parserInfo.textLength ? parserInfo.text[parserInfo.cursor] : '\0'; parserInfo.cursor++; return c; }
		static inline char peek(const ParserInfo& parserInfo, int advance = 0) { return parserInfo.cursor + advance > parserInfo.textLength - 1 ? '\0' : parserInfo.text[parserInfo.cursor + advance]; }
		static inline bool isDigit(char c) { return (c >= '0' && c <= '9'); }
		static inline bool isNumberPart(char c) { return isDigit(c) || c == '-' || c == '.'; }
		static inline bool isAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
		static inline bool isWhitespace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\0'; }
		static inline bool isStyleIdentifierPart(char c) { return isAlpha(c) || c == '-'; }

		void init()
		{
			errorBuffer[0] = '\0';
		}

		SvgGroup* parseSvgDoc(const char* filepath)
		{
			XMLDocument doc;
			int res = doc.LoadFile(filepath);
			if (res != XML_SUCCESS)
			{
				PANIC("Failed to parse XML in SVG file '%s'.", filepath);
				return nullptr;
			}

			XMLElement* svgElement = doc.FirstChildElement("svg");
			if (!svgElement)
			{
				PANIC("No <svg> element found in document '%s'.", filepath);
				return nullptr;
			}

			const XMLAttribute* viewboxAttribute = svgElement->FindAttribute("viewBox");
			if (!viewboxAttribute)
			{
				PANIC("SVG '%s' has no viewBox attribute.", filepath);
				return nullptr;
			}

			Vec4 viewbox;
			if (!parseViewbox(&viewbox, viewboxAttribute->Value()))
			{
				PANIC("Failed to parse viewBox attribute for SVG '%s'.", filepath);
				return nullptr;
			}

			// NOTE: <defs> and <g> elements are not required apparently
			// The SVG paths can be embedded as children of <svg> directly
			XMLElement* defsElement = svgElement->FirstChildElement("defs");
			XMLElement* groupElement = svgElement->FirstChildElement("g");
			// TODO: This isn't the best way to do this
			XMLElement* styleElement = svgElement->FirstChildElement("style");

			Stylesheet rootStylesheet;
			if (styleElement)
			{
				if (!parseSvgStylesheet(styleElement, &rootStylesheet))
				{
					PANIC("Parsing SVG style failed: %s", styleElement->GetText());
					return nullptr;
				}
			}

			std::unordered_map<std::string, SvgObject> objIds;

			if (defsElement)
			{
				XMLElement* childEl = defsElement->FirstChildElement();
				// Loop through all the definitions and save the svg objects
				while (childEl != nullptr)
				{
					std::string elementName = childEl->Name();
					SvgElementType elementType = elementStringToEnum(elementName);

					switch (elementType)
					{
					case SvgElementType::Path:
					{
						const XMLAttribute* id = childEl->FindAttribute("id");
						if (id == nullptr) g_logger_warning("Child element '%s' had no id attribute.", childEl->Name());

						const XMLAttribute* pathAttribute = childEl->FindAttribute("d");
						SvgObject obj;
						if (parseSvgPathTag(childEl, &obj, rootStylesheet))
						{
							std::string idStr = std::string(id->Value());
							auto iter = objIds.find(idStr);
							g_logger_assert(iter == objIds.end(), "Tried to insert duplicate ID '%s' in SVG object map", idStr.c_str());
							obj.calculateBBox();
							objIds[idStr] = obj;
						}
					}
					break;
					case SvgElementType::Style:
					{
						if (styleElement != nullptr)
						{
							PANIC_NOFMT("We don't support multi-stylesheet SVGs yet. This SVG has multiple embedded stylesheets.");
						}
						else
						{
							parseSvgStylesheet(childEl, &rootStylesheet);
						}
					}
					break;
					case SvgElementType::Defs:
						g_logger_warning("TODO: Implement me.");
						break;
					case SvgElementType::Desc:
						g_logger_warning("TODO: Implement me.");
						break;
					case SvgElementType::Polygon:
						g_logger_warning("TODO: Implement me.");
						break;
					case SvgElementType::Rect:
						g_logger_warning("TODO: Implement me.");
						break;
					case SvgElementType::Title:
						g_logger_warning("TODO: Implement me.");
						break;
					case SvgElementType::Use:
						g_logger_warning("TODO: Implement me.");
						break;
					case SvgElementType::None:
						break;
					}

					childEl = childEl->NextSiblingElement();
				}
			}

			// Create a group using the svg objects and the viewbox attribute
			SvgGroup* group = (SvgGroup*)g_memory_allocate(sizeof(SvgGroup));
			*group = Svg::createDefaultGroup();
			Svg::beginSvgGroup(group);

			{
				XMLElement* childEl = groupElement != nullptr
					? groupElement->FirstChildElement()
					: svgElement->FirstChildElement();
				while (childEl != nullptr)
				{
					SvgElementType elementType = elementStringToEnum(childEl->Name());

					switch (elementType)
					{
					case SvgElementType::Use:
					{
						const XMLAttribute* xAttr = childEl->FindAttribute("x");
						const XMLAttribute* yAttr = childEl->FindAttribute("y");
						const XMLAttribute* linkAttr = childEl->FindAttribute("xlink:href");

						if (!xAttr) g_logger_warning("Child element '%s' had no x attribute.", childEl->Name());
						if (!yAttr) g_logger_warning("Child element '%s' had no y attribute.", childEl->Name());

						if (!linkAttr) g_logger_warning("Child element '%s' had no xlink:href attribute.", childEl->Name());

						if (xAttr && yAttr && linkAttr)
						{
							float x = xAttr->FloatValue();
							float y = yAttr->FloatValue();
							const char* linkText = linkAttr->Value();
							if (linkText[0] != '#')
							{
								g_logger_warning("Child element '%s' link attribute '%s' did not begin with '#'.", childEl->Name(), linkText);
								continue;
							}

							std::string id = std::string(linkText + 1);
							auto iter = objIds.find(id);
							if (iter == objIds.end())
							{
								g_logger_warning("Could not find link to svg path '%s' for child element '%s'", id.c_str(), childEl->Name());
								continue;
							}

							// Flip y-coords to be consistent with everything else positioning from 
							// the bottom-left
							// NOTE: I'm not exactly sure why I have to "flip" the coordinate like this
							// but it works and that's good enough for me...
							y = y + iter->second.bbox.min.y - viewbox.values[3];
							Svg::pushSvgToGroup(group, iter->second, iter->first, Vec2{ x, y });
						}
					}
					break;
					case SvgElementType::Rect:
					{
						const XMLAttribute* xAttr = childEl->FindAttribute("x");
						const XMLAttribute* yAttr = childEl->FindAttribute("y");

						if (!xAttr) g_logger_warning("Child element '%s' had no x attribute.", childEl->Name());
						if (!yAttr) g_logger_warning("Child element '%s' had no y attribute.", childEl->Name());

						const XMLAttribute* wAttr = childEl->FindAttribute("width");
						const XMLAttribute* hAttr = childEl->FindAttribute("height");

						if (!wAttr) g_logger_warning("Child element '%s' had no width attribute.", childEl->Name());
						if (!hAttr) g_logger_warning("Child element '%s' had no height attribute.", childEl->Name());

						if (wAttr && hAttr && xAttr && yAttr)
						{
							float x = xAttr->FloatValue();
							float y = yAttr->FloatValue();
							float w = wAttr->FloatValue();
							float h = hAttr->FloatValue();

							SvgObject rect = Svg::createDefault();
							Svg::beginPath(&rect, { 0, h });
							Svg::lineTo(&rect, { w, h });
							Svg::lineTo(&rect, { w, 0 });
							Svg::lineTo(&rect, { 0, 0 });
							Svg::lineTo(&rect, { 0, h });
							Svg::closePath(&rect);

							static uint64 rCounter = 0;
							rCounter++;
							std::string rCounterStr = "rect-" + rCounter;
							// Flip y-coords to be consistent with everything else positioning from 
							// the bottom-left
							y = y - viewbox.values[3];
							Svg::pushSvgToGroup(group, rect, rCounterStr, Vec2{ x, y });
						}
					}
					break;
					case SvgElementType::Path:
					{
						SvgObject obj;
						if (parseSvgPathTag(childEl, &obj, rootStylesheet))
						{
							static uint64 uniqueName = 0;
							uniqueName++;
							std::string name = std::to_string(uniqueName);
							Svg::pushSvgToGroup(group, obj, name);
						}
						else
						{
							g_logger_warning("Failed to parse path tag in SVG '%s'", filepath);
							goto error_cleanup;
						}
					}
					break;
					case SvgElementType::Polygon:
					{
						SvgObject obj;
						if (parsePolygonTag(childEl, &obj, rootStylesheet))
						{
							static uint64 uniqueName = 0;
							uniqueName++;
							std::string name = std::to_string(uniqueName);
							Svg::pushSvgToGroup(group, obj, name);
						}
						else
						{
							g_logger_warning("Failed to parse path tag in SVG '%s'", filepath);
							goto error_cleanup;
						}
					}
					break;
					case SvgElementType::Defs:
					{
						// NOP; Don't care about the defs block that's handled above.
					}
					break;
					case SvgElementType::Title:
					{
						// FIXME: Title is necessary for accesibility
					}
					break;
					case SvgElementType::Desc:
					{
						g_logger_warning("TODO: Implement me.");
					}
					case SvgElementType::Style:
					{
						g_logger_warning("TODO: Implement me.");
					}
					break;
					case SvgElementType::None:
						break;
					}

					childEl = childEl->NextSiblingElement();
				}
			}

			if (group->numObjects <= 0)
			{
			error_cleanup:
				PANIC("Did not find any <path> elements or other SVG elements in file '%s'. Check the logs for more information.", filepath);
				group->free();
				g_memory_free(group);
				return nullptr;
			}

			Svg::endSvgGroup(group);

			return group;
		}

		bool parseSvgPath(const char* pathText, size_t pathTextLength, SvgObject* output)
		{
			SvgObject res = Svg::createDefault();
			if (pathTextLength <= 0)
			{
				PANIC_NOFMT("Cannot parse an SVG path that has no text.");
				return false;
			}

			ParserInfo parserInfo;
			parserInfo.cursor = 0;
			parserInfo.text = pathText;
			parserInfo.textLength = pathTextLength;

			PathToken token = parseNextPathToken(parserInfo);
			bool lastPathTokenWasClosePath = false;
			while (token.type != PathTokenType::EndOfFile)
			{
				// panic if we fail to interpret a command
				bool panic = !interpretCommand(token, parserInfo, &res);
				if (!panic)
				{
					PathToken nextPathToken = parseNextPathToken(parserInfo);
					if (nextPathToken.type == PathTokenType::EndOfFile)
					{
						lastPathTokenWasClosePath = token.type == PathTokenType::ClosePath;
					}
					else if (nextPathToken.type == PathTokenType::Panic)
					{
						panic = true;
					}
					token = nextPathToken;
				}

				if (panic)
				{
					res.free();
					g_logger_error("Had an error while parsing svg path and panicked");
					return false;
				}
			}

			// We should only do this if the path didn't end with a close_path command
			if (!lastPathTokenWasClosePath)
			{
				bool isHole = res.numPaths > 1;
				Svg::closePath(&res, false, isHole);
			}

			res.calculateApproximatePerimeter();
			res.calculateBBox();
			*output = res;

			return true;
		}

		const char* getLastError()
		{
			return errorBuffer;
		}

		// ----------- Internal Functions -----------
		static bool parseSvgStylesheet(XMLElement* element, Stylesheet* output)
		{
			// Stylesheet: 
			//   Style | Stylesheet
			// 

			const char* styleText = element->GetText();
			size_t styleTextLength = std::strlen(styleText);

			Stylesheet res = {};
			if (styleTextLength <= 0)
			{
				PANIC_NOFMT("Cannot parse an SVG style that has no text.");
				return false;
			}

			ParserInfo parserInfo;
			parserInfo.cursor = 0;
			parserInfo.text = styleText;
			parserInfo.textLength = styleTextLength;

			StyleToken token = {};
			while (token.type != StyleTokenType::EndOfFile)
			{
				token = parseNextStyleToken(parserInfo);
				if (token.type == StyleTokenType::Panic)
				{
					g_logger_error("Had an error while parsing svg path and panicked");
					token.free();
					return false;
				}

				// Root level objects must be #id or .id
				if (token.type == StyleTokenType::HashtagIdentifier || token.type == StyleTokenType::DotIdentifier)
				{
					if (!parseStyle(parserInfo, &res, token))
					{
						PANIC_NOFMT("Error while parsing embedded style in SVG doc. See logs for more info.");
						token.free();
						return false;
					}
				}

				token.free();
			}

			*output = res;

			return true;
		}

		static bool parseSvgPathTag(XMLElement* element, SvgObject* output, const Stylesheet& styles)
		{
			if (std::strcmp(element->Name(), "path") != 0)
			{
				PANIC("Tried to parse tag <%s> as SVG path, but this tag is not a path tag.", element->Name());
				return false;
			}

			const XMLAttribute* pathAttribute = element->FindAttribute("d");
			if (pathAttribute == nullptr)
			{
				PANIC("Element '%s' had no path attribute.", element->Name());
				return false;

			}

			size_t textLength = std::strlen(pathAttribute->Value());
			if (parseSvgPath(pathAttribute->Value(), textLength, output))
			{
				applyElementStylesToSvg(element, output, styles);
				return true;
			}

			return false;
		}

		static bool parsePolygonTag(XMLElement* element, SvgObject* output, const Stylesheet& styles)
		{
			if (std::strcmp(element->Name(), "polygon") != 0)
			{
				PANIC("Tried to parse tag <%s> as polygon, but this tag is not a polygon.", element->Name());
				return false;
			}

			const XMLAttribute* points = element->FindAttribute("points");
			if (points == nullptr)
			{
				PANIC("Element '%s' had no points attribute.", element->Name());
				return false;

			}
			size_t textLength = std::strlen(points->Value());

			ParserInfo pointsParser;
			pointsParser.cursor = 0;
			pointsParser.text = points->Value();
			pointsParser.textLength = textLength;
			std::vector<Vec2> pointsList;

			// Points are only valid if they have an even number of points as specified here: 
			// https://www.w3.org/TR/SVG2/shapes.html#PolygonElement
			// The parseVec2List should fail if there are an odd number of elements
			if (!parseVec2List(pointsList, pointsParser))
			{
				PANIC("Element '%s' had an odd number of points.", element->Name());
				return false;
			}

			// Create an SVG object using absolute moveTo commands and lineTo commands
			// to each point in the list
			if (pointsList.size() > 0)
			{
				*output = Svg::createDefault();
				Svg::moveTo(output, pointsList[0]);

				for (size_t i = 1; i < pointsList.size(); i++)
				{
					Svg::lineTo(output, pointsList[i]);
				}
				Svg::closePath(output, true);
			}

			applyElementStylesToSvg(element, output, styles);

			return true;
		}

		static void applyElementStylesToSvg(XMLElement* element, SvgObject* output, const Stylesheet& styles)
		{
			// First add any styles specified by the class and/or id this element has

			// Add styles to this SVG path
			const XMLAttribute* classNameAttribute = element->FindAttribute("class");
			if (classNameAttribute)
			{
				const std::string& className = classNameAttribute->Value();
				auto iter = styles.classMap.find(className);
				if (iter != styles.classMap.end())
				{
					const Style& style = iter->second;
					// Apply each attribute to our SVG object
					applyStylesTo(output, style);
				}
			}

			const XMLAttribute* idNameAttribute = element->FindAttribute("id");
			if (idNameAttribute)
			{
				const std::string& idName = idNameAttribute->Value();
				auto iter = styles.idMap.find(idName);
				if (iter != styles.idMap.end())
				{
					const Style& style = iter->second;
					// Apply each attribute to our SVG object
					applyStylesTo(output, style);
				}
			}

			// Next apply any styles specified in attributes like this:
			//    <path d="..." fill-rule="nonzero"></path>
			//                  ^- Style specified in the attribute
			const XMLAttribute* attribute = element->FirstAttribute();

			// These are a list of attributes that get skipped since they're not styles
			const std::unordered_set<std::string> attributeBlacklist = {
				"d",
				"class",
				"id",
				"points"
			};

			while (attribute != nullptr)
			{
				const std::string& name = attribute->Name();
				auto blacklistIter = attributeBlacklist.find(name);
				if (blacklistIter == attributeBlacklist.end())
				{
					StyleAttribute sanitizedAttribute;
					if (name == "style")
					{
						// If there's an inline style embedded in this attribute, parse the style then
						// apply it to the path
						ParserInfo inlineStylePi;
						inlineStylePi.text = attribute->Value();
						inlineStylePi.textLength = std::strlen(attribute->Value());
						inlineStylePi.cursor = 0;
						Style inlineStyle;
						if (parseInlineStyle(inlineStylePi, &inlineStyle, false))
						{
							applyStylesTo(output, inlineStyle);
						}
						else
						{
							PANIC_NOFMT("Failed to parse inline style.");
						}
					}
					else if (sanitizeAttribute(attribute, &sanitizedAttribute))
					{
						applyStyleAttributeTo(output, sanitizedAttribute);
					}
					else
					{
						PANIC("Unknown inline style attribute for SVG path '%s'", name.c_str());
					}
				}

				attribute = attribute->Next();
			}
		}

		static void applyStylesTo(SvgObject* output, const Style& style)
		{
			for (const StyleAttribute& attribute : style.attributes)
			{
				applyStyleAttributeTo(output, attribute);
			}
		}

		static void applyStyleAttributeTo(SvgObject* output, const StyleAttribute& attribute)
		{
			const std::string& attributeName = attribute.name();
			if (attributeName == "fill-rule")
			{
				std::string value = attribute.value();
				if (value == "evenodd")
				{
					output->fillType = FillType::EvenOddFillType;
				}
				else if (value == "nonzero")
				{
					output->fillType = FillType::NonZeroFillType;
				}
				else
				{
					g_logger_warning("Unknown fill-rule type: %s", value.c_str());
				}
			}
			else if (attributeName == "fill")
			{
				if (!(attribute.isColor()))
				{
					PANIC("Invalid fill type '%s' in SVG document. Must be color.", attribute.value().c_str());
				}
				output->fillColor = attribute.asColor();
			}
		}

		static SvgElementType elementStringToEnum(const std::string& elementName)
		{
			static const std::unordered_map<std::string, SvgElementType> map = {
				{ "path",    SvgElementType::Path },
				{ "title",   SvgElementType::Title },
				{ "desc",    SvgElementType::Desc },
				{ "use",     SvgElementType::Use },
				{ "rect",    SvgElementType::Rect },
				{ "polygon", SvgElementType::Polygon },
				{ "style",   SvgElementType::Style },
				{ "defs",    SvgElementType::Defs }
			};

			auto iter = map.find(elementName);
			if (iter != map.end())
			{
				return iter->second;
			}

			return SvgElementType::None;
		}

		static bool interpretCommand(const PathToken& token, ParserInfo& parserInfo, SvgObject* res)
		{
			PathTokenType commandType = token.type;
			bool isAbsolute = token.isAbsolute;
			if (commandType == PathTokenType::Panic)
			{
				return false;
			}

			// Parse as many {x, y} pairs as possible
			switch (commandType)
			{
			case PathTokenType::MoveTo:
			{
				std::vector<Vec2> vec2List;
				if (!parseVec2List(vec2List, parserInfo))
				{
					PANIC_NOFMT("Error interpreting move to command. Invalid coordinate encountered.");
					return false;
				}
				if (vec2List.size() <= 0)
				{
					PANIC_NOFMT("Error interpreting move to command. No coordinates provided.");
					return false;
				}

				const Vec2& firstPoint = vec2List[0];
				Svg::moveTo(res, Vec2{ firstPoint.x, firstPoint.y }, isAbsolute);
				for (int i = 1; i < vec2List.size(); i++)
				{
					Svg::lineTo(res, Vec2{ vec2List[i].x, vec2List[i].y }, isAbsolute);
				}
			}
			break;
			case PathTokenType::ClosePath:
			{
				bool isHole = res->numPaths > 1;
				Svg::closePath(res, true, isHole);
			}
			break;
			case PathTokenType::LineTo:
			{
				std::vector<Vec2> vec2List;
				if (!parseVec2List(vec2List, parserInfo))
				{
					PANIC_NOFMT("Error interpreting line to command. Invalid coordinate encountered.");
					return false;
				}
				if (vec2List.size() <= 0)
				{
					PANIC_NOFMT("Error interpreting line to command. No coordinates provided.");
					return false;
				}

				for (int i = 0; i < vec2List.size(); i++)
				{
					Svg::lineTo(res, Vec2{ vec2List[i].x, vec2List[i].y }, isAbsolute);
				}
			}
			break;
			case PathTokenType::HzLineTo:
			{
				std::vector<float> numberList;
				if (!parseHzNumberList(numberList, parserInfo))
				{
					PANIC_NOFMT("Error interpreting line to command. Invalid coordinate encountered.");
					return false;
				}
				if (numberList.size() <= 0)
				{
					PANIC_NOFMT("Error interpreting line to command. No coordinates provided.");
					return false;
				}

				for (int i = 0; i < numberList.size(); i++)
				{
					Svg::hzLineTo(res, numberList[i], isAbsolute);
				}
			}
			break;
			case PathTokenType::VtLineTo:
			{
				std::vector<float> numberList;
				if (!parseVtNumberList(numberList, parserInfo))
				{
					PANIC_NOFMT("Error interpreting line to command. Invalid coordinate encountered.");
					return false;
				}
				if (numberList.size() <= 0)
				{
					PANIC_NOFMT("Error interpreting line to command. No coordinates provided.");
					return false;
				}

				for (int i = 0; i < numberList.size(); i++)
				{
					Svg::vtLineTo(res, numberList[i], isAbsolute);
				}
			}
			break;
			case PathTokenType::CurveTo:
			{
				std::vector<Vec2> vec2List;
				if (!parseVec2List(vec2List, parserInfo))
				{
					PANIC_NOFMT("Error interpreting move to command. Invalid coordinate encountered.");
					return false;
				}
				if (vec2List.size() <= 0)
				{
					PANIC_NOFMT("Error interpreting move to command. No coordinates provided.");
					return false;
				}

				if (vec2List.size() % 3 != 0)
				{
					PANIC_NOFMT("Cubic polybezier curve must have a multiple of 3 coordinates, otherwise it's not a valid polybezier curve.");
					return false;
				}

				for (size_t i = 0; i < vec2List.size(); i += 3)
				{
					g_logger_assert(i + 2 < vec2List.size(), "Somehow ended up with a non-multiple of 3.");
					Svg::bezier3To(res, vec2List[i + 0], vec2List[i + 1], vec2List[i + 2], isAbsolute);
				}
			}
			break;
			case PathTokenType::SmoothCurveTo:
			{
				std::vector<Vec2> vec2List;
				if (!parseVec2List(vec2List, parserInfo))
				{
					PANIC_NOFMT("Error interpreting move to command. Invalid coordinate encountered.");
					return false;
				}
				if (vec2List.size() <= 0)
				{
					PANIC_NOFMT("Error interpreting move to command. No coordinates provided.");
					return false;
				}

				if (vec2List.size() % 2 != 0)
				{
					PANIC_NOFMT("Smooth cubic polybezier curve must have a multiple of 2 coordinates, otherwise it's not a valid polybezier curve.");
					return false;
				}

				for (size_t i = 0; i < vec2List.size(); i += 2)
				{
					g_logger_assert(i + 1 < vec2List.size(), "Somehow ended up with a non-multiple of 2.");
					Svg::smoothBezier3To(res, vec2List[i + 0], vec2List[i + 1], isAbsolute);
				}
			}
			break;
			case PathTokenType::QuadCurveTo:
			{
				std::vector<Vec2> vec2List;
				if (!parseVec2List(vec2List, parserInfo))
				{
					PANIC_NOFMT("Error interpreting move to command. Invalid coordinate encountered.");
					return false;
				}
				if (vec2List.size() <= 0)
				{
					PANIC_NOFMT("Error interpreting move to command. No coordinates provided.");
					return false;
				}

				if (vec2List.size() % 2 != 0)
				{
					PANIC_NOFMT("Quadratic polybezier curve must have a multiple of 2 coordinates, otherwise it's not a valid polybezier curve.");
					return false;
				}

				for (size_t i = 0; i < vec2List.size(); i += 2)
				{
					g_logger_assert(i + 1 < vec2List.size(), "Somehow ended up with a non-multiple of 2.");
					Svg::bezier2To(res, vec2List[i + 0], vec2List[i + 1], isAbsolute);
				}
			}
			break;
			case PathTokenType::SmoothQuadCurveTo:
			{
				std::vector<Vec2> vec2List;
				if (!parseVec2List(vec2List, parserInfo))
				{
					PANIC_NOFMT("Error interpreting move to command. Invalid coordinate encountered.");
					return false;
				}
				if (vec2List.size() <= 0)
				{
					PANIC_NOFMT("Error interpreting move to command. No coordinates provided.");
					return false;
				}

				for (size_t i = 0; i < vec2List.size(); i++)
				{
					Svg::smoothBezier2To(res, vec2List[i], isAbsolute);
				}
			}
			break;
			case PathTokenType::ArcTo:
			{
				std::vector<ArcParams> arcParamsList;
				if (!parseArcParamsList(arcParamsList, parserInfo))
				{
					PANIC_NOFMT("Error interpreting move to command. Invalid coordinate encountered.");
					return false;
				}
				if (arcParamsList.size() <= 0)
				{
					PANIC_NOFMT("Error interpreting arc to command. No coordinates provided.");
					return false;
				}

				for (size_t i = 0; i < arcParamsList.size(); i++)
				{
					Svg::arcTo(
						res,
						arcParamsList[i].radius,
						arcParamsList[i].xAxisRotation,
						arcParamsList[i].largeArcFlag,
						arcParamsList[i].sweepFlag,
						arcParamsList[i].endpoint,
						isAbsolute
					);
				}
			}
			break;
			case PathTokenType::EndOfFile:
				PANIC_NOFMT("Interpreting SVG EOF as a command. Something must have gone wrong. Check logs.");
				break;
			case PathTokenType::Length:
			case PathTokenType::Number:
			case PathTokenType::Panic:
				break;
			}

			return true;
		}

		static bool parseVec2List(std::vector<Vec2>& list, ParserInfo& parserInfo)
		{
			do
			{
				// TODO: Revisit this with a match(tokenType) statement
				PathToken x = consume(PathTokenType::Number, parserInfo);
				PathToken y = consume(PathTokenType::Number, parserInfo);

				if (x.type == PathTokenType::Number && y.type == PathTokenType::Number)
				{
					float xVal = x.as.number;
					float yVal = y.as.number;
					list.emplace_back(Vec2{ xVal, yVal });
				}
				else
				{
					return false;
				}
			} while (isNumberPart(peek(parserInfo)));

			return true;
		}

		static bool parseHzNumberList(std::vector<float>& list, ParserInfo& parserInfo)
		{
			do
			{
				// TODO: Revisit this with a match(tokenType) statement
				PathToken x = consume(PathTokenType::Number, parserInfo);

				if (x.type == PathTokenType::Number)
				{
					float xVal = x.as.number;
					list.emplace_back(xVal);
				}
				else
				{
					return false;
				}
			} while (isNumberPart(peek(parserInfo)));

			return true;
		}

		static bool parseVtNumberList(std::vector<float>& list, ParserInfo& parserInfo)
		{
			do
			{
				// TODO: Revisit this with a match(tokenType) statement
				PathToken y = consume(PathTokenType::Number, parserInfo);

				if (y.type == PathTokenType::Number)
				{
					float yVal = y.as.number;
					list.emplace_back(yVal);
				}
				else
				{
					return false;
				}
			} while (isNumberPart(peek(parserInfo)));

			return true;
		}

		static bool parseArcParamsList(std::vector<ArcParams>& list, ParserInfo& parserInfo)
		{
			do
			{
				// TODO: Revisit this with a match(tokenType) statement
				PathToken rx = consume(PathTokenType::Number, parserInfo);
				PathToken ry = consume(PathTokenType::Number, parserInfo);
				PathToken xAxisRotation = consume(PathTokenType::Number, parserInfo);
				PathToken largeArcFlag = consume(PathTokenType::Number, parserInfo);
				PathToken sweepFlag = consume(PathTokenType::Number, parserInfo);
				PathToken dstX = consume(PathTokenType::Number, parserInfo);
				PathToken dstY = consume(PathTokenType::Number, parserInfo);

				if (rx.type == PathTokenType::Number && ry.type == PathTokenType::Number &&
					xAxisRotation.type == PathTokenType::Number &&
					largeArcFlag.type == PathTokenType::Number && sweepFlag.type == PathTokenType::Number &&
					dstX.type == PathTokenType::Number && dstY.type == PathTokenType::Number)
				{
					ArcParams res;
					res.radius.x = rx.as.number;
					res.radius.y = ry.as.number;
					res.xAxisRotation = xAxisRotation.as.number;
					res.largeArcFlag = largeArcFlag.as.number != 0.0f;
					res.sweepFlag = sweepFlag.as.number != 0.0f;
					res.endpoint.x = dstX.as.number;
					res.endpoint.y = dstY.as.number;

					list.emplace_back(res);
				}
				else
				{
					return false;
				}
			} while (isNumberPart(peek(parserInfo)));

			return true;
		}

		static bool parseViewbox(Vec4* out, const char* viewboxStr)
		{
			ParserInfo pi;
			pi.cursor = 0;
			pi.text = viewboxStr;
			pi.textLength = std::strlen(viewboxStr);

			PathToken x = parseNextPathToken(pi);
			PathToken y = parseNextPathToken(pi);
			PathToken w = parseNextPathToken(pi);
			PathToken h = parseNextPathToken(pi);

			if (x.type != PathTokenType::Number || y.type != PathTokenType::Number || w.type != PathTokenType::Number || h.type != PathTokenType::Number)
			{
				PANIC("Malformed viewBox '%s'", viewboxStr);
				return false;
			}

			out->values[0] = x.as.number;
			out->values[1] = y.as.number;
			out->values[2] = w.as.number;
			out->values[3] = h.as.number;
			return true;
		}

		static PathToken parseNextPathToken(ParserInfo& parserInfo)
		{
			PathToken result;
			result.type = PathTokenType::Panic;

			if (isAlpha(peek(parserInfo)))
			{
				char commandLetter = advance(parserInfo);
				switch (commandLetter)
				{
				case 'M':
				case 'm':
					result.type = PathTokenType::MoveTo;
					result.isAbsolute = commandLetter == 'M';
					break;
				case 'Z':
				case 'z':
					result.type = PathTokenType::ClosePath;
					break;
				case 'L':
				case 'l':
					result.type = PathTokenType::LineTo;
					result.isAbsolute = commandLetter == 'L';
					break;
				case 'H':
				case 'h':
					result.type = PathTokenType::HzLineTo;
					result.isAbsolute = commandLetter == 'H';
					break;
				case 'V':
				case 'v':
					result.type = PathTokenType::VtLineTo;
					result.isAbsolute = commandLetter == 'V';
					break;
				case 'C':
				case 'c':
					result.type = PathTokenType::CurveTo;
					result.isAbsolute = commandLetter == 'C';
					break;
				case 'S':
				case 's':
					result.type = PathTokenType::SmoothCurveTo;
					result.isAbsolute = commandLetter == 'S';
					break;
				case 'Q':
				case 'q':
					result.type = PathTokenType::QuadCurveTo;
					result.isAbsolute = commandLetter == 'Q';
					break;
				case 'T':
				case 't':
					result.type = PathTokenType::SmoothQuadCurveTo;
					result.isAbsolute = commandLetter == 'T';
					break;
				case 'A':
				case 'a':
					result.type = PathTokenType::ArcTo;
					result.isAbsolute = commandLetter == 'A';
					break;
				}
			}
			else if (isNumberPart(peek(parserInfo)))
			{
				float number;
				size_t cursorStart = parserInfo.cursor;
				if (!parseNumber(parserInfo, &number))
				{
					size_t previewEnd = glm::min(cursorStart + 10, parserInfo.textLength);
					std::string errorPreview = std::string(&parserInfo.text[cursorStart], &parserInfo.text[previewEnd]);
					PANIC("Could not parse number while parsing SVG path: '%s'", errorPreview.c_str());
				}
				else
				{
					result.type = PathTokenType::Number;
					result.as.number = number;
				}
			}
			else if (parserInfo.cursor >= parserInfo.textLength)
			{
				result.type = PathTokenType::EndOfFile;
			}
			else
			{
				PANIC("Unknown symbol encountered while parsing SVG path. ParserInfo[%u/%u]:'%c'", parserInfo.cursor, parserInfo.textLength, peek(parserInfo));
			}

			skipWhitespaceAndCommas(parserInfo);
			return result;
		}

		static PathToken consume(PathTokenType expected, ParserInfo& parserInfo)
		{
			PathToken token = parseNextPathToken(parserInfo);
			if (token.type != expected)
			{
				token.type = PathTokenType::Panic;
				parserInfo.cursor = parserInfo.textLength;
			}

			return token;
		}

		// -------- Svg Style Parser --------

		static StyleToken parseNextStyleToken(ParserInfo& parserInfo)
		{
			skipWhitespace(parserInfo);

			/*
				Color # identifier, // Handled later phase of parsing
			*/
			StyleToken res;
			res.type = StyleTokenType::Panic;

			switch (peek(parserInfo))
			{
			case '{': return consumeChar('{', StyleTokenType::LeftCurlyBracket, parserInfo);
			case '}': return consumeChar('}', StyleTokenType::RightCurlyBracket, parserInfo);
			case'(': return consumeChar('(', StyleTokenType::LeftParen, parserInfo);
			case ')': return consumeChar(')', StyleTokenType::RightParen, parserInfo);
			case '\0': return consumeChar('\0', StyleTokenType::EndOfFile, parserInfo);
			case ':': return consumeChar(':', StyleTokenType::Colon, parserInfo);
			case ';': return consumeChar(';', StyleTokenType::Semicolon, parserInfo);
			case '#':
			{
				// We have #something don't know if it's a color or identifier until
				// we start interpreting
				char hashtag = advance(parserInfo);
				g_logger_assert(hashtag == '#', "Something went horribly wrong.");

				if (parseIdentifier(parserInfo, &res.as.identifier, true))
				{
					res.type = StyleTokenType::HashtagIdentifier;
				}
			}
			break;
			case '.':
			{
				// We have a number or a .identifier

				if (!isNumberPart(peek(parserInfo, 1)))
				{
					char hashtag = advance(parserInfo);
					g_logger_assert(hashtag == '.', "Something went horribly wrong.");

					if (parseIdentifier(parserInfo, &res.as.identifier))
					{
						res.type = StyleTokenType::DotIdentifier;
					}

					break;
				}

				if (parseNumber(parserInfo, &res.as.number))
				{
					res.type = StyleTokenType::Number;
					break;
				}
			}
			break;
			case '\'':
			case '"':
			{
				// Pass through whether this is a single quote string or 
				// double quote string
				if (parseString(parserInfo, &res.as.identifier, peek(parserInfo)))
				{
					res.type = StyleTokenType::String;
					break;
				}
			}
			break;
			case '-':
			{
				// We have a -1.2 negative number or an identifier -moz-blah

				// This must be a number
				if (peek(parserInfo, 1) == '.' || isDigit(peek(parserInfo, 1)))
				{
					if (parseNumber(parserInfo, &res.as.number))
					{
						res.type = StyleTokenType::Number;
					}

					break;
				}

				if (parseIdentifier(parserInfo, &res.as.identifier))
				{
					res.type = StyleTokenType::Identifier;
					break;
				}
			}
			break;
			default:
			{
				// We have a number or identifier or unknown

				if (isDigit(peek(parserInfo)))
				{
					if (parseNumber(parserInfo, &res.as.number))
					{
						res.type = StyleTokenType::Number;
					}

					break;
				}

				if (parseIdentifier(parserInfo, &res.as.identifier))
				{
					res.type = StyleTokenType::Identifier;
					break;
				}
			}
			break;
			}

			if (res.type == StyleTokenType::Number)
			{
				// Check if this has a unit of measurement afterwards and adjust appropriately if necessary
				char c1 = peek(parserInfo);
				char c2 = peek(parserInfo, 1);
				char c3 = peek(parserInfo, 2);
				if (c1 == 'p' && c2 == 'x' && (isWhitespace(c3) || c3 == ';'))
				{
					// This is a number of pixels
					// Skip past the px and reset our type then continue
					advance(parserInfo); advance(parserInfo);
					res.type = StyleTokenType::NumberPixels;
				}
			}

			return res;
		}

		static bool parseInlineStyle(ParserInfo& parserInfo, Style* res, bool consumeClosingCurlyBracket)
		{
			StyleToken token = parseNextStyleToken(parserInfo);
			while (token.type != StyleTokenType::RightCurlyBracket && token.type != StyleTokenType::EndOfFile)
			{
				if (token.type != StyleTokenType::Identifier)
				{
					PANIC("Expected identifier. Instead got token of type: %d", token.type);
					token.free();
					return false;
				}

				StyleToken attributeName = token;

				token = consume(StyleTokenType::Colon, parserInfo);
				if (token.type == StyleTokenType::Panic)
				{
					return false;
				}

				StyleToken attributeValue = parseNextStyleToken(parserInfo);

				token = parseNextStyleToken(parserInfo);
				if (token.type != StyleTokenType::Semicolon && token.type != StyleTokenType::EndOfFile)
				{
					PANIC("Expected ';' or 'EOF' to end a style attribute. Instead got token of type %d", token.type);
					token.free();
					return false;
				}

				StyleAttribute sanitizedAttribute;
				if (sanitizeAttribute(attributeName, attributeValue, &sanitizedAttribute))
				{
					res->attributes.emplace_back(sanitizedAttribute);
				}
				else
				{
					PANIC("Failed to sanitize attribute <%s=...>", attributeName.as.identifier.text);
				}

				attributeName.free();
				attributeValue.free();
				token.free();
				token = parseNextStyleToken(parserInfo);
			}

			if (consumeClosingCurlyBracket && token.type != StyleTokenType::RightCurlyBracket)
			{
				token.free();
				PANIC("Expected '}' to end a style. Instead got token of type: %d", token.type);
				return false;
			}
			else if (!consumeClosingCurlyBracket && token.type != StyleTokenType::EndOfFile)
			{
				token.free();
				PANIC("Expected 'EOF' to end an inline-style. Instead got token of type: %d", token.type);
				return false;
			}

			token.free();
			return true;
		}

		static bool parseStyle(ParserInfo& parserInfo, Stylesheet* res, const StyleToken& identifier)
		{
			Style newStyle;

			StyleToken token = consume(StyleTokenType::LeftCurlyBracket, parserInfo);
			if (token.type == StyleTokenType::Panic)
			{
				return false;
			}

			if (!parseInlineStyle(parserInfo, &newStyle, true))
			{
				return false;
			}

			newStyle.className = "";
			newStyle.idName = "";

			if (identifier.type == StyleTokenType::HashtagIdentifier)
			{
				newStyle.idName = identifier.as.identifier.text;
				res->idMap[identifier.as.identifier.text] = newStyle;
			}
			else if (identifier.type == StyleTokenType::DotIdentifier)
			{
				newStyle.className = identifier.as.identifier.text;
				res->classMap[identifier.as.identifier.text] = newStyle;
			}
			else
			{
				PANIC("Cannot assign identifier of type %d as a style attribute. Must be a HashtagIdentifier or a DotIdentifier", identifier.type);
				return false;
			}

			return true;
		}

		static StyleToken consumeChar(char expectedChar, StyleTokenType resType, ParserInfo& parserInfo)
		{
			StyleToken token;
			token.type = resType;

			char c = advance(parserInfo);
			if (c != expectedChar)
			{
				token.type = StyleTokenType::Panic;
				parserInfo.cursor = parserInfo.textLength - 1;
			}

			return token;
		}

		static StyleToken consume(StyleTokenType expected, ParserInfo& parserInfo)
		{
			StyleToken token = parseNextStyleToken(parserInfo);
			if (token.type != expected)
			{
				token.free();
				PANIC("Expected token of type '%d' but got token of type '%d'.", expected, token.type);
				token.type = StyleTokenType::Panic;
				parserInfo.cursor = parserInfo.textLength;
			}

			return token;
		}

		static void printStyleToken(StyleToken& token)
		{
			switch (token.type)
			{
			case StyleTokenType::Panic:
				g_logger_info("Token<Panic>");
				break;
			case StyleTokenType::EndOfFile:
				g_logger_info("Token<EOF>: \\0");
				break;
			case StyleTokenType::Identifier:
				g_logger_info("Token<Identifier>: %s", token.as.identifier.text);
				break;
			case StyleTokenType::DotIdentifier:
				g_logger_info("Token<DotIdentifier>: .%s", token.as.identifier.text);
				break;
			case StyleTokenType::HashtagIdentifier:
				g_logger_info("Token<HashtagIdentifier>: #%s", token.as.identifier.text);
				break;
			case StyleTokenType::LeftCurlyBracket:
				g_logger_info("Token<LCurlyBracket>: {");
				break;
			case StyleTokenType::LeftParen:
				g_logger_info("Token<LParen>: (");
				break;
			case StyleTokenType::Number:
				g_logger_info("Token<Number>: %2.3f", token.as.number);
				break;
			case StyleTokenType::RightCurlyBracket:
				g_logger_info("Token<RCurlyBracket>: }");
				break;
			case StyleTokenType::RightParen:
				g_logger_info("Token<RParen>: )");
				break;
			case StyleTokenType::String:
				g_logger_info("Token<String>: \"%s\"", token.as.identifier.text);
				break;
			case StyleTokenType::Colon:
				g_logger_info("Token<Colon>: :");
				break;
			case StyleTokenType::Semicolon:
				g_logger_info("Token<Semicolon>: ;");
				break;
			case StyleTokenType::Color:
			case StyleTokenType::Length:
			case StyleTokenType::NumberPixels:
				break;
			}
		}

		static bool parseColor(const std::string& color, Vec4* output)
		{
			const std::unordered_map<std::string, Vec4> presetColors = {
				{ "none", Vec4{ 0, 0, 0, 0 } }
			};

			auto iter = presetColors.find(color);
			if (iter != presetColors.end())
			{
				*output = iter->second;
				return true;
			}

			std::string colorAsStr = color;
			if (colorAsStr[0] == '#')
			{
				colorAsStr = colorAsStr.substr(1);
			}

			for (size_t i = 0; i < colorAsStr.size(); i++)
			{
				// If the character's aren't all hex characters, this isn't a valid color
				// string
				if (!(
					(colorAsStr[i] >= '0' && colorAsStr[i] <= '9') ||
					(colorAsStr[i] >= 'a' && colorAsStr[i] <= 'f') ||
					(colorAsStr[i] >= 'A' && colorAsStr[i] <= 'F')
					))
				{
					PANIC("Color '#%s' has invalid hex character", colorAsStr.c_str());
					return false;
				}
			}

			if (colorAsStr.length() == 3)
			{
				colorAsStr += colorAsStr;
			}

			if (colorAsStr.length() == 6 || colorAsStr.length() == 8)
			{
				*output = toHex(colorAsStr);
				return true;
			}

			return false;
		}

		static bool sanitizeAttribute(const StyleToken& attributeName, const StyleToken& attributeValue, StyleAttribute* output)
		{
			if (attributeName.type != StyleTokenType::Identifier)
			{
				PANIC("Attribute Name must be an identifier. This attribute name was of type %d", attributeName.type);
				return false;
			}

			const std::unordered_map<std::string, StyleAttributeType> validAttributeType = {
				// TODO: Have enums for stuff like this
				{ "clip-rule", StyleAttributeType::Identifier },
				{ "fill", StyleAttributeType::Color },
				{ "fill-rule", StyleAttributeType::Identifier },
				{ "font-family", StyleAttributeType::IdentifierOrString },
				{ "font-size", StyleAttributeType::Number },
				{ "stroke", StyleAttributeType::Color },
				{ "stroke-dasharray", StyleAttributeType::Identifier },
				{ "stroke-linecap", StyleAttributeType::Identifier },
				{ "stroke-linejoin", StyleAttributeType::Identifier },
				{ "stroke-miterlimit", StyleAttributeType::Number },
				{ "stroke-width", StyleAttributeType::Number },
			};

			auto iter = validAttributeType.find(attributeName.as.identifier.text);
			if (iter != validAttributeType.end())
			{
				switch (iter->second)
				{
				case StyleAttributeType::Identifier:
				{
					if (attributeValue.type != StyleTokenType::Identifier)
					{
						return false;
					}

					*output = StyleAttribute(
						std::string(attributeName.as.identifier.text),
						std::string(attributeValue.as.identifier.text)
					);
				}
				break;
				case StyleAttributeType::IdentifierOrString:
				{
					if (attributeValue.type != StyleTokenType::Identifier && attributeValue.type != StyleTokenType::String)
					{
						return false;
					}

					*output = StyleAttribute(
						std::string(attributeName.as.identifier.text),
						std::string(attributeValue.as.identifier.text)
					);
				}
				break;
				case StyleAttributeType::Number:
				{
					if (attributeValue.type != StyleTokenType::NumberPixels && attributeValue.type != StyleTokenType::Number)
					{
						return false;
					}

					// TODO: Convert this to some internal measurement before emplacing the number
					// if the number is of type pixels or some other unit
					*output = StyleAttribute(
						std::string(attributeName.as.identifier.text),
						attributeValue.as.number
					);
				}
				break;
				case StyleAttributeType::Color:
				{
					if (attributeValue.type != StyleTokenType::HashtagIdentifier && attributeValue.type != StyleTokenType::Identifier)
					{
						return false;
					}

					// This should be a color if parsing fails then log that and move on
					Vec4 color;
					if (parseColor(attributeValue.as.identifier.text, &color))
					{
						*output = StyleAttribute(
							std::string(attributeName.as.identifier.text),
							color
						);
					}
					else
					{
						PANIC("Invalid color as attribute value '%s'. This is not a color, for attribute: %s", attributeName.as.identifier.text, attributeValue.as.identifier.text);
						return false;
					}
				}
				break;
				case StyleAttributeType::None:
				case StyleAttributeType::Style:
					break;
				}
			}
			else
			{
				return false;
			}

			// If we fall through to here then sanitization succeeded
			return true;
		}

		static bool sanitizeAttribute(const XMLAttribute* attribute, StyleAttribute* output)
		{
			// TODO: I really don't like this, but it might be the best way to leverage
			// the parser to get the appropriate types and stuff...
			ParserInfo pi;
			pi.cursor = 0;
			pi.text = attribute->Name();
			pi.textLength = std::strlen(attribute->Name());

			StyleToken attributeNameToken = parseNextStyleToken(pi);

			pi.cursor = 0;
			pi.text = attribute->Value();
			pi.textLength = std::strlen(attribute->Value());
			StyleToken attributeValueToken = parseNextStyleToken(pi);

			bool res = sanitizeAttribute(attributeNameToken, attributeValueToken, output);

			attributeNameToken.free();
			attributeValueToken.free();

			return res;
		}

		// -------- Generic Parser Stuff --------

		static bool parseNumber(ParserInfo& parserInfo, float* out)
		{
			size_t numberStart = parserInfo.cursor;
			size_t numberEnd = parserInfo.cursor;

			bool seenDot = false;
			if (peek(parserInfo) == '-')
			{
				numberEnd++;
				advance(parserInfo);
			}

			while (isDigit(peek(parserInfo)) || (peek(parserInfo) == '.' && !seenDot))
			{
				seenDot = seenDot || peek(parserInfo) == '.';
				numberEnd++;
				advance(parserInfo);
			}

			if (numberEnd > numberStart)
			{
				constexpr int maxSmallBufferSize = 32;
				char smallBuffer[maxSmallBufferSize];
				g_logger_assert(numberEnd - numberStart <= maxSmallBufferSize, "Cannot parse number greater than %d characters big.", maxSmallBufferSize);
				g_memory_copyMem(smallBuffer, (void*)(parserInfo.text + numberStart), sizeof(char) * (numberEnd - numberStart));
				smallBuffer[numberEnd - numberStart] = '\0';
				// TODO: atof is not safe use a safer modern alternative
				*out = (float)atof(smallBuffer);
				return true;
			}

			*out = 0.0f;
			return false;
		}

		static bool parseString(ParserInfo& parserInfo, DumbString* string, char stringTerminator)
		{
			char stringStart = advance(parserInfo);
			if (stringStart != stringTerminator)
			{
				*string = {};
				return false;
			}

			size_t stringStartIndex = parserInfo.cursor;
			size_t stringEndIndex = parserInfo.cursor;

			while (peek(parserInfo) != stringTerminator)
			{
				advance(parserInfo);
				stringEndIndex++;
			}

			char stringEnd = advance(parserInfo);
			if (stringEnd != stringTerminator)
			{
				*string = {};
				return false;
			}

			g_logger_assert(stringStartIndex < parserInfo.textLength, "Invalid start index.");
			g_logger_assert(stringEndIndex < parserInfo.textLength&& stringEndIndex >= stringStartIndex, "Invalid end index.");

			string->textLength = (stringEndIndex - stringStartIndex);
			string->text = (char*)g_memory_allocate(sizeof(char) * (string->textLength + 1));
			g_memory_copyMem(string->text, (void*)(parserInfo.text + stringStartIndex), sizeof(char) * string->textLength);
			string->text[string->textLength] = '\0';

			return true;
		}

		static bool parseIdentifier(ParserInfo& parserInfo, DumbString* string, bool canStartWithNumber)
		{
			size_t stringStartIndex = parserInfo.cursor;
			size_t stringEndIndex = parserInfo.cursor;

			char idStart = advance(parserInfo);
			stringEndIndex++;
			if (!isStyleIdentifierPart(idStart))
			{
				if (!(isDigit(idStart) && canStartWithNumber))
				{
					goto cleanupReturnFalse;
				}
			}

			while (isStyleIdentifierPart(peek(parserInfo)) || isDigit(peek(parserInfo)))
			{
				advance(parserInfo);
				stringEndIndex++;
			}

			g_logger_assert(stringStartIndex < parserInfo.textLength, "Invalid start index.");
			g_logger_assert(stringEndIndex <= parserInfo.textLength && stringEndIndex > stringStartIndex, "Invalid end index.");
			g_logger_assert(stringEndIndex <= parserInfo.textLength && stringEndIndex > stringStartIndex, "Invalid end index.");

			if (stringEndIndex - stringStartIndex == 1 && parserInfo.text[stringStartIndex] == '-')
			{
				PANIC_NOFMT("Cannot have an identifier that is just a dash like this '-'.");
				goto cleanupReturnFalse;
			}

			string->textLength = (stringEndIndex - stringStartIndex);
			string->text = (char*)g_memory_allocate(sizeof(char) * (string->textLength + 1));
			g_memory_copyMem(string->text, (void*)(parserInfo.text + stringStartIndex), sizeof(char) * string->textLength);
			string->text[string->textLength] = '\0';

			return true;

		cleanupReturnFalse:
			*string = {};
			return false;
		}

		static void skipWhitespaceAndCommas(ParserInfo& parserInfo)
		{
			while (isWhitespace(peek(parserInfo)) || peek(parserInfo) == ',')
			{
				advance(parserInfo);
				if (peek(parserInfo) == '\0')
					break;
			}
		}

		static void skipWhitespace(ParserInfo& parserInfo)
		{
			while (isWhitespace(peek(parserInfo)))
			{
				advance(parserInfo);
				if (peek(parserInfo) == '\0')
					break;
			}
		}
	}
}

#undef PANIC;