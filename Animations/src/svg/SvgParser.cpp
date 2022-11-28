#include "svg/SvgParser.h"
#include "svg/Svg.h"

#include <../tinyxml2/tinyxml2.h>

using namespace tinyxml2;

#define PANIC(formatStr, ...) \
	snprintf(errorBuffer, errorBufferSize, formatStr, __VA_ARGS__); \
	g_logger_error(formatStr, __VA_ARGS__);

namespace MathAnim
{
	struct ParserInfo
	{
		const char* text;
		size_t textLength;
		size_t cursor;
	};

	enum class TokenType : uint8
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

	struct Token
	{
		TokenType type;
		bool isAbsolute;
		union
		{
			float number;
		} as;
	};

	struct ArcParams
	{
		Vec2 radius;
		float xAxisRotation;
		bool largeArcFlag;
		bool sweepFlag;
		Vec2 endpoint;
	};

	namespace SvgParser
	{
		// Internal variables
		static constexpr size_t errorBufferSize = 1024;
		static char errorBuffer[errorBufferSize];

		// ----------- Internal Functions -----------
		static bool parseSvgPathTag(XMLElement* element, SvgObject* output);
		static bool interpretCommand(const Token& token, ParserInfo& parserInfo, SvgObject* res);
		static bool parseVec2List(std::vector<Vec2>& list, ParserInfo& parserInfo);
		static bool parseHzNumberList(std::vector<float>& list, ParserInfo& parserInfo);
		static bool parseVtNumberList(std::vector<float>& list, ParserInfo& parserInfo);
		static bool parseArcParamsList(std::vector<ArcParams>& list, ParserInfo& parserInfo);
		static bool parseViewbox(Vec4* out, const char* viewboxStr);
		static Token parseNextToken(ParserInfo& parserInfo);
		static bool parseNumber(ParserInfo& parserInfo, float* out);
		static Token consume(TokenType expected, ParserInfo& parserInfo);
		static void skipWhitespaceAndCommas(ParserInfo& parserInfo);
		static inline char advance(ParserInfo& parserInfo) { char c = parserInfo.cursor < parserInfo.textLength ? parserInfo.text[parserInfo.cursor] : '\0'; parserInfo.cursor++; return c; }
		static inline char peek(const ParserInfo& parserInfo, int advance = 0) { return parserInfo.cursor + advance > parserInfo.textLength - 1 ? '\0' : parserInfo.text[parserInfo.cursor + advance]; }
		static inline bool isDigit(char c) { return (c >= '0' && c <= '9'); }
		static inline bool isNumberPart(char c) { return isDigit(c) || c == '-' || c == '.'; }
		static inline bool isAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
		static inline bool isWhitespace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\0'; }

		void init()
		{
			errorBuffer[0] = '\0';
		}

		SvgGroup* parseSvgDoc(const char* filepath)
		{
			XMLDocument doc;
			doc.LoadFile(filepath);
			XMLElement* svgElement = doc.FirstChildElement("svg");
			if (!svgElement)
			{
				PANIC("No <svg> element found in document '%s'.", filepath);
				return nullptr;
			}

			const XMLAttribute* versionAttribute = svgElement->FindAttribute("version");
			if (!versionAttribute)
			{
				g_logger_warning("Unknown svg version. No version attribute provided for '%s'. Will attempt to parse, but no guarantees it will succeed.", filepath);
			}
			else
			{
				const char* version = versionAttribute->Value();
				if (std::strcmp(version, "1.1") != 0)
				{
					g_logger_warning("Only support for SVG version 1.1 right now. Doc '%s' had version '%s'. Will attempt to parse, but no guarantees it will succeed.", filepath, version);
				}
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

			std::unordered_map<std::string, SvgObject> objIds;

			if (defsElement)
			{
				XMLElement* childEl = defsElement->FirstChildElement();
				// Loop through all the definitions and save the svg objects
				while (childEl != nullptr)
				{
					const XMLAttribute* id = childEl->FindAttribute("id");
					if (id == nullptr) g_logger_warning("Child element '%s' had no id attribute.", childEl->Name());

					const XMLAttribute* pathAttribute = childEl->FindAttribute("d");
					SvgObject obj;
					if (parseSvgPathTag(childEl, &obj))
					{
						std::string idStr = std::string(id->Value());
						auto iter = objIds.find(idStr);
						g_logger_assert(iter == objIds.end(), "Tried to insert duplicate ID '%s' in SVG object map", idStr.c_str());
						obj.calculateBBox();
						objIds[idStr] = obj;
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
					if (std::strcmp(childEl->Name(), "use") == 0)
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
					else if (std::strcmp(childEl->Name(), "rect") == 0)
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
					else if (std::strcmp(childEl->Name(), "path") == 0)
					{
						SvgObject obj;
						if (parseSvgPathTag(childEl, &obj))
						{
							static uint64 uniqueName = 0;
							uniqueName++;
							std::string name = std::to_string(uniqueName);
							Svg::pushSvgToGroup(group, obj, name, Vec2{ FLT_MAX, FLT_MAX });
						}
						else
						{
							g_logger_warning("Failed to parse path tag in SVG '%s'", filepath);
							goto error_cleanup;
						}
					}
					else
					{
						g_logger_warning("Unknown group element '%s'.", childEl->Name());
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
				PANIC("Cannot parse an SVG path that has no text.");
				return false;
			}

			ParserInfo parserInfo;
			parserInfo.cursor = 0;
			parserInfo.text = pathText;
			parserInfo.textLength = pathTextLength;

			Token token = parseNextToken(parserInfo);
			bool lastTokenWasClosePath = false;
			while (token.type != TokenType::EndOfFile)
			{
				// panic if we fail to interpret a command
				bool panic = !interpretCommand(token, parserInfo, &res);
				if (!panic)
				{
					Token nextToken = parseNextToken(parserInfo);
					if (nextToken.type == TokenType::EndOfFile)
					{
						lastTokenWasClosePath = token.type == TokenType::ClosePath;
					}
					else if (nextToken.type == TokenType::Panic)
					{
						panic = true;
					}
					token = nextToken;
				}

				if (panic)
				{
					res.free();
					g_logger_error("Had an error while parsing svg path and panicked");
					return false;
				}
			}

			// We should only do this if the path didn't end with a close_path command
			if (!lastTokenWasClosePath)
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
		static bool parseSvgPathTag(XMLElement* element, SvgObject* output)
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
			return parseSvgPath(pathAttribute->Value(), textLength, output);
		}

		static bool interpretCommand(const Token& token, ParserInfo& parserInfo, SvgObject* res)
		{
			TokenType commandType = token.type;
			bool isAbsolute = token.isAbsolute;
			if (commandType == TokenType::Panic)
			{
				return false;
			}

			// Parse as many {x, y} pairs as possible
			switch (commandType)
			{
			case TokenType::MoveTo:
			{
				std::vector<Vec2> vec2List;
				if (!parseVec2List(vec2List, parserInfo))
				{
					PANIC("Error interpreting move to command. Invalid coordinate encountered.");
					return false;
				}
				if (vec2List.size() <= 0)
				{
					PANIC("Error interpreting move to command. No coordinates provided.");
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
			case TokenType::ClosePath:
			{
				bool isHole = res->numPaths > 1;
				Svg::closePath(res, true, isHole);
			}
			break;
			case TokenType::LineTo:
			{
				std::vector<Vec2> vec2List;
				if (!parseVec2List(vec2List, parserInfo))
				{
					PANIC("Error interpreting line to command. Invalid coordinate encountered.");
					return false;
				}
				if (vec2List.size() <= 0)
				{
					PANIC("Error interpreting line to command. No coordinates provided.");
					return false;
				}

				for (int i = 0; i < vec2List.size(); i++)
				{
					Svg::lineTo(res, Vec2{ vec2List[i].x, vec2List[i].y }, isAbsolute);
				}
			}
			break;
			case TokenType::HzLineTo:
			{
				std::vector<float> numberList;
				if (!parseHzNumberList(numberList, parserInfo))
				{
					PANIC("Error interpreting line to command. Invalid coordinate encountered.");
					return false;
				}
				if (numberList.size() <= 0)
				{
					PANIC("Error interpreting line to command. No coordinates provided.");
					return false;
				}

				for (int i = 0; i < numberList.size(); i++)
				{
					Svg::hzLineTo(res, numberList[i], isAbsolute);
				}
			}
			break;
			case TokenType::VtLineTo:
			{
				std::vector<float> numberList;
				if (!parseVtNumberList(numberList, parserInfo))
				{
					PANIC("Error interpreting line to command. Invalid coordinate encountered.");
					return false;
				}
				if (numberList.size() <= 0)
				{
					PANIC("Error interpreting line to command. No coordinates provided.");
					return false;
				}

				for (int i = 0; i < numberList.size(); i++)
				{
					Svg::vtLineTo(res, numberList[i], isAbsolute);
				}
			}
			break;
			case TokenType::CurveTo:
			{
				std::vector<Vec2> vec2List;
				if (!parseVec2List(vec2List, parserInfo))
				{
					PANIC("Error interpreting move to command. Invalid coordinate encountered.");
					return false;
				}
				if (vec2List.size() <= 0)
				{
					PANIC("Error interpreting move to command. No coordinates provided.");
					return false;
				}

				if (vec2List.size() % 3 != 0)
				{
					PANIC("Cubic polybezier curve must have a multiple of 3 coordinates, otherwise it's not a valid polybezier curve.");
					return false;
				}

				for (size_t i = 0; i < vec2List.size(); i += 3)
				{
					g_logger_assert(i + 2 < vec2List.size(), "Somehow ended up with a non-multiple of 3.");
					Svg::bezier3To(res, vec2List[i + 0], vec2List[i + 1], vec2List[i + 2], isAbsolute);
				}
			}
			break;
			case TokenType::SmoothCurveTo:
			{
				std::vector<Vec2> vec2List;
				if (!parseVec2List(vec2List, parserInfo))
				{
					PANIC("Error interpreting move to command. Invalid coordinate encountered.");
					return false;
				}
				if (vec2List.size() <= 0)
				{
					PANIC("Error interpreting move to command. No coordinates provided.");
					return false;
				}

				if (vec2List.size() % 2 != 0)
				{
					PANIC("Smooth cubic polybezier curve must have a multiple of 2 coordinates, otherwise it's not a valid polybezier curve.");
					return false;
				}

				for (size_t i = 0; i < vec2List.size(); i += 2)
				{
					g_logger_assert(i + 1 < vec2List.size(), "Somehow ended up with a non-multiple of 2.");
					Svg::smoothBezier3To(res, vec2List[i + 0], vec2List[i + 1], isAbsolute);
				}
			}
			break;
			case TokenType::QuadCurveTo:
			{
				std::vector<Vec2> vec2List;
				if (!parseVec2List(vec2List, parserInfo))
				{
					PANIC("Error interpreting move to command. Invalid coordinate encountered.");
					return false;
				}
				if (vec2List.size() <= 0)
				{
					PANIC("Error interpreting move to command. No coordinates provided.");
					return false;
				}

				if (vec2List.size() % 2 != 0)
				{
					PANIC("Quadratic polybezier curve must have a multiple of 2 coordinates, otherwise it's not a valid polybezier curve.");
					return false;
				}

				for (size_t i = 0; i < vec2List.size(); i += 2)
				{
					g_logger_assert(i + 1 < vec2List.size(), "Somehow ended up with a non-multiple of 2.");
					Svg::bezier2To(res, vec2List[i + 0], vec2List[i + 1], isAbsolute);
				}
			}
			break;
			case TokenType::SmoothQuadCurveTo:
			{
				std::vector<Vec2> vec2List;
				if (!parseVec2List(vec2List, parserInfo))
				{
					PANIC("Error interpreting move to command. Invalid coordinate encountered.");
					return false;
				}
				if (vec2List.size() <= 0)
				{
					PANIC("Error interpreting move to command. No coordinates provided.");
					return false;
				}

				for (size_t i = 0; i < vec2List.size(); i++)
				{
					Svg::smoothBezier2To(res, vec2List[i], isAbsolute);
				}
			}
			break;
			case TokenType::ArcTo:
			{
				std::vector<ArcParams> arcParamsList;
				if (!parseArcParamsList(arcParamsList, parserInfo))
				{
					PANIC("Error interpreting move to command. Invalid coordinate encountered.");
					return false;
				}
				if (arcParamsList.size() <= 0)
				{
					PANIC("Error interpreting arc to command. No coordinates provided.");
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
			default:
				PANIC("Unknown SVG command type: %d", commandType);
				return false;
			}

			return true;
		}

		static bool parseVec2List(std::vector<Vec2>& list, ParserInfo& parserInfo)
		{
			do
			{
				// TODO: Revisit this with a match(tokenType) statement
				Token x = consume(TokenType::Number, parserInfo);
				Token y = consume(TokenType::Number, parserInfo);

				if (x.type == TokenType::Number && y.type == TokenType::Number)
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
				Token x = consume(TokenType::Number, parserInfo);

				if (x.type == TokenType::Number)
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
				Token y = consume(TokenType::Number, parserInfo);

				if (y.type == TokenType::Number)
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
				Token rx = consume(TokenType::Number, parserInfo);
				Token ry = consume(TokenType::Number, parserInfo);
				Token xAxisRotation = consume(TokenType::Number, parserInfo);
				Token largeArcFlag = consume(TokenType::Number, parserInfo);
				Token sweepFlag = consume(TokenType::Number, parserInfo);
				Token dstX = consume(TokenType::Number, parserInfo);
				Token dstY = consume(TokenType::Number, parserInfo);

				if (rx.type == TokenType::Number && ry.type == TokenType::Number &&
					xAxisRotation.type == TokenType::Number &&
					largeArcFlag.type == TokenType::Number && sweepFlag.type == TokenType::Number &&
					dstX.type == TokenType::Number && dstY.type == TokenType::Number)
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

			Token x = parseNextToken(pi);
			Token y = parseNextToken(pi);
			Token w = parseNextToken(pi);
			Token h = parseNextToken(pi);

			if (x.type != TokenType::Number || y.type != TokenType::Number || w.type != TokenType::Number || h.type != TokenType::Number)
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

		static Token parseNextToken(ParserInfo& parserInfo)
		{
			Token result;
			result.type = TokenType::Panic;

			if (isAlpha(peek(parserInfo)))
			{
				char commandLetter = advance(parserInfo);
				switch (commandLetter)
				{
				case 'M':
				case 'm':
					result.type = TokenType::MoveTo;
					result.isAbsolute = commandLetter == 'M';
					break;
				case 'Z':
				case 'z':
					result.type = TokenType::ClosePath;
					break;
				case 'L':
				case 'l':
					result.type = TokenType::LineTo;
					result.isAbsolute = commandLetter == 'L';
					break;
				case 'H':
				case 'h':
					result.type = TokenType::HzLineTo;
					result.isAbsolute = commandLetter == 'H';
					break;
				case 'V':
				case 'v':
					result.type = TokenType::VtLineTo;
					result.isAbsolute = commandLetter == 'V';
					break;
				case 'C':
				case 'c':
					result.type = TokenType::CurveTo;
					result.isAbsolute = commandLetter == 'C';
					break;
				case 'S':
				case 's':
					result.type = TokenType::SmoothCurveTo;
					result.isAbsolute = commandLetter == 'S';
					break;
				case 'Q':
				case 'q':
					result.type = TokenType::QuadCurveTo;
					result.isAbsolute = commandLetter == 'Q';
					break;
				case 'T':
				case 't':
					result.type = TokenType::SmoothQuadCurveTo;
					result.isAbsolute = commandLetter == 'T';
					break;
				case 'A':
				case 'a':
					result.type = TokenType::ArcTo;
					result.isAbsolute = commandLetter == 'A';
					break;
				default:
					PANIC("Unknown SVG command: %c", commandLetter);
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
					result.type = TokenType::Number;
					result.as.number = number;
				}
			}
			else if (parserInfo.cursor >= parserInfo.textLength)
			{
				result.type = TokenType::EndOfFile;
			}
			else
			{
				PANIC("Unknown symbol encountered while parsing SVG path. ParserInfo[%d/%d]:'%c'", parserInfo.cursor, parserInfo.textLength, peek(parserInfo));
			}

			skipWhitespaceAndCommas(parserInfo);
			return result;
		}

		static Token consume(TokenType expected, ParserInfo& parserInfo)
		{
			Token token = parseNextToken(parserInfo);
			if (token.type != expected)
			{
				token.type = TokenType::Panic;
				parserInfo.cursor = parserInfo.textLength;
			}

			return token;
		}

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
				*out = (float)atof(smallBuffer);
				return true;
			}

			*out = 0.0f;
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
	}
}

#undef PANIC;