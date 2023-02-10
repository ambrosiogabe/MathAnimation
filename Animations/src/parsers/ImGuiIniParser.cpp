#include "parsers/ImGuiIniParser.h"
#include "parsers/Common.h"

#include <nlohmann/json.hpp>

#pragma warning(push, 0)

namespace MathAnim
{
	struct StringView
	{
		const char* source;
		size_t begin;
		size_t length;

		std::string toString() const
		{
			return std::string(source + begin, length);
		}
	};

	struct DockSpaceData
	{
		std::string id;
		std::string window;
		std::string pos;
		std::string size;
		std::string split;
		std::string centralNode;
		std::string selected;
	};

	struct DockNodeData
	{
		std::string id;
		std::string parent;
		std::string sizeRef;
		std::optional<std::string> split;
		std::optional<std::string> centralNode;
		std::string selected;
	};

	struct WindowBlock
	{
		std::string windowName;
		std::optional<std::string> pos;
		std::string size;
		std::string collapsed;
		std::optional<std::string> dockId;
		std::optional<std::string> viewportPos;
		std::optional<std::string> viewportId;
	};

	struct TableBlock
	{
		std::string tableName;
		std::vector<std::string> lines;
	};

	struct DockingLine
	{
		uint8 indentationLevel;
		std::optional<DockSpaceData> dockSpaceData;
		std::optional<DockNodeData> dockNodeData;
	};

	struct DockingBlock
	{
		std::vector<DockingLine> dockingLines;
	};

	namespace ImGuiIniParser
	{
		// ----------------------- Internal Functions -----------------------
		static void addWindowBlockToJson(const WindowBlock& windowBlock, nlohmann::json& j);
		static void addTableBlockToJson(const TableBlock& tableBlock, nlohmann::json& j);
		static void addDockingBlockToJson(const DockingBlock& dockingBlock, nlohmann::json& j);

		static bool parseWindowBlock(WindowBlock& output, ParserInfo& parser);
		static bool parseTableBlock(TableBlock& output, ParserInfo& parser);
		static bool parseDockingBlock(DockingBlock& output, ParserInfo& parser);
		static bool parseDockSpaceData(std::optional<DockSpaceData>& output, ParserInfo& parser);
		static bool parseDockNodeData(std::optional<DockNodeData>& output, ParserInfo& parser);
		static bool parseMultipleKeyValueEqualSignMultiline(std::unordered_map<std::string, std::string>& output, ParserInfo& parser);
		static bool parseMultipleKeyValueEqualSign(std::unordered_map<std::string, std::string>& output, ParserInfo& parser);
		static bool parseKeyValueEqualSign(StringView& key, StringView& value, ParserInfo& parser);
		static bool parseBracketPair(StringView& key, StringView& value, ParserInfo& parser);
		static bool parseBracketIdentifier(StringView& output, ParserInfo& parser);
		static bool makeStringView(StringView& output, size_t start, size_t end, ParserInfo& parser);
		static bool parseLine(StringView& output, ParserInfo& parser);

		static nlohmann::json parseVec2(const std::string& str);
		static Vec2 parseVec2(const nlohmann::json& j);

		void convertImGuiIniToJson(const char* iniFilepath, const char* outputFilepath, const Vec2& outputResolution)
		{
			ParserInfo parser = Parser::openParserForFile(iniFilepath);

			nlohmann::json j = {};
			j["Windows"] = nlohmann::json::array();
			j["Tables"] = nlohmann::json::array();
			j["ConfigResolution"]["X"] = outputResolution.x;
			j["ConfigResolution"]["Y"] = outputResolution.y;

			while (Parser::peek(parser) != '\0')
			{
				Parser::skipWhitespace(parser);

				if (parser.cursor >= parser.textLength)
				{
					break;
				}

				StringView key = {};
				StringView value = {};
				if (!parseBracketPair(key, value, parser))
				{
					g_logger_error("Expected bracket pair like [Key][Value] and instead got something else in imgui.ini file.");
					Parser::freeParser(parser);
					return;
				}

				std::string keyStr = key.toString();
				std::string valueStr = value.toString();
				if (keyStr == "Window")
				{
					WindowBlock windowBlock = {};
					if (!parseWindowBlock(windowBlock, parser))
					{
						g_logger_error("Failed to parse window block in imgui.ini file.");
						Parser::freeParser(parser);
						return;
					}
					windowBlock.windowName = valueStr;
					addWindowBlockToJson(windowBlock, j);
				}
				else if (keyStr == "Table")
				{
					if (!Parser::consumeNewline(parser))
					{
						g_logger_error("Expected newline after [Table][<SomeTableName>] while parsing imgui.ini file.");
						Parser::freeParser(parser);
						return;
					}

					TableBlock tableBlock = {};
					if (!parseTableBlock(tableBlock, parser))
					{
						g_logger_error("Failed to parse table block in imgui.ini file.");
						Parser::freeParser(parser);
						return;
					}
					tableBlock.tableName = valueStr;
					addTableBlockToJson(tableBlock, j);
				}
				else if (keyStr == "Docking" && valueStr == "Data")
				{
					if (!Parser::consumeNewline(parser))
					{
						g_logger_error("Expected newline after [Docking][Data] while parsing imgui.ini file.");
						Parser::freeParser(parser);
						return;
					}

					DockingBlock dockingBlock = {};
					if (!parseDockingBlock(dockingBlock, parser))
					{
						g_logger_error("Failed to parse docking block in imgui.ini file.");
						Parser::freeParser(parser);
						return;
					}
					addDockingBlockToJson(dockingBlock, j);
				}
			}

			Parser::freeParser(parser);

			std::string jsonOutput = j.dump(4);
			FILE* f = fopen(outputFilepath, "wb");
			if (f != nullptr)
			{
				fwrite(jsonOutput.c_str(), jsonOutput.size(), 1, f);
			}

			if (f)
			{
				fclose(f);
			}
		}

		void convertJsonToImGuiIni(const char* jsonFilepath, const char* outputFilepath, const Vec2& targetResolution)
		{
			float scaleX = 1.0f;
			float scaleY = 1.0f;
			try
			{
				std::ifstream f(jsonFilepath);
				nlohmann::json j = nlohmann::json::parse(f);

				// Set up scale factors
				if (j.contains("ConfigResolution"))
				{
					nlohmann::json& res = j["ConfigResolution"];
					scaleX = res.contains("X") ? targetResolution.x / res["X"] : 1.0f;
					scaleY = res.contains("Y") ? targetResolution.y / res["Y"] : 1.0f;
				}

				FILE* outputFile = fopen(outputFilepath, "wb");
				if (!outputFile)
				{
					g_logger_error("Failed to open file '%s' for writing when converting imgui json file.", outputFilepath);
					return;
				}

				// Output Windows first
				char formatBuffer[4096];
				for (const nlohmann::json& window : j["Windows"])
				{
					if (!window["name"].is_null())
					{
						std::string name = window["name"];
						snprintf(formatBuffer, sizeof(formatBuffer), "[Window][%s]\n", name.c_str());
						fwrite(formatBuffer, std::strlen(formatBuffer), 1, outputFile);
					}
					if (!window["pos"].is_null())
					{
						Vec2 pos = parseVec2(window["pos"]);
						snprintf(formatBuffer, sizeof(formatBuffer), "Pos=%d,%d\n", (int)(pos.x * scaleX), (int)(pos.y * scaleY));
						fwrite(formatBuffer, std::strlen(formatBuffer), 1, outputFile);
					}
					if (!window["viewportPos"].is_null())
					{
						Vec2 pos = parseVec2(window["viewportPos"]);
						snprintf(formatBuffer, sizeof(formatBuffer), "ViewportPos=%d,%d\n", (int)(pos.x * scaleX), (int)(pos.y * scaleY));
						fwrite(formatBuffer, std::strlen(formatBuffer), 1, outputFile);
					}
					if (!window["viewportId"].is_null())
					{
						std::string viewportId = window["viewportId"];
						snprintf(formatBuffer, sizeof(formatBuffer), "ViewportId=%s\n", viewportId.c_str());
						fwrite(formatBuffer, std::strlen(formatBuffer), 1, outputFile);
					}
					if (!window["size"].is_null())
					{
						Vec2 size = parseVec2(window["size"]);
						snprintf(formatBuffer, sizeof(formatBuffer), "Size=%d,%d\n", (int)(size.x * scaleX), (int)(size.y * scaleY));
						fwrite(formatBuffer, std::strlen(formatBuffer), 1, outputFile);
					}
					if (!window["collapsed"].is_null())
					{
						std::string collapsed = window["collapsed"];
						snprintf(formatBuffer, sizeof(formatBuffer), "Collapsed=%s\n", collapsed.c_str());
						fwrite(formatBuffer, std::strlen(formatBuffer), 1, outputFile);
					}
					if (!window["dockId"].is_null())
					{
						std::string dockId = window["dockId"];
						snprintf(formatBuffer, sizeof(formatBuffer), "DockId=%s\n", dockId.c_str());
						fwrite(formatBuffer, std::strlen(formatBuffer), 1, outputFile);
					}

					fwrite("\n", sizeof(char), 1, outputFile);
				}

				// Then tables
				for (const nlohmann::json& table : j["Tables"])
				{
					std::string tableName = table["name"];
					snprintf(formatBuffer, sizeof(formatBuffer), "[Table][%s]\n", tableName.c_str());
					fwrite(formatBuffer, std::strlen(formatBuffer), 1, outputFile);

					if (table.contains("lines"))
					{
						for (const nlohmann::json& line : table["lines"])
						{
							std::string lineStr = line;
							snprintf(formatBuffer, sizeof(formatBuffer), "%s", lineStr.c_str());
							fwrite(formatBuffer, std::strlen(formatBuffer), 1, outputFile);
						}
					}
				}
				
				fclose(outputFile);
			}
			catch (std::exception ex)
			{
				g_logger_error("Failed to parse imgui ini as json file: '%s'", ex.what());
			}
		}

		// ----------------------- Internal Functions -----------------------
		static void addWindowBlockToJson(const WindowBlock& windowBlock, nlohmann::json& j)
		{
			nlohmann::json windowAsJson = {};
			windowAsJson["name"] = windowBlock.windowName;
			windowAsJson["size"] = parseVec2(windowBlock.size);
			windowAsJson["collapsed"] = windowBlock.collapsed;
			windowAsJson["pos"] = nullptr;
			if (windowBlock.pos.has_value()) windowAsJson["pos"] = parseVec2(windowBlock.pos.value());
			windowAsJson["dockId"] = nullptr;
			if (windowBlock.dockId.has_value()) windowAsJson["dockId"] = windowBlock.dockId.value();
			windowAsJson["viewportPos"] = nullptr;
			if (windowBlock.viewportPos.has_value()) windowAsJson["viewportPos"] = parseVec2(windowBlock.viewportPos.value());
			windowAsJson["viewportId"] = nullptr;
			if (windowBlock.viewportId.has_value()) windowAsJson["viewportId"] = windowBlock.viewportId.value();

			j["Windows"].emplace_back(windowAsJson);
		}

		static void addTableBlockToJson(const TableBlock& tableBlock, nlohmann::json& j)
		{
			nlohmann::json tableAsJson = {};
			tableAsJson["name"] = tableBlock.tableName;
			tableAsJson["lines"] = nlohmann::json::array();
			for (const auto& line : tableBlock.lines)
			{
				tableAsJson["lines"].push_back(line);
			}

			j["Tables"].emplace_back(tableAsJson);
		}

		static void addDockingBlockToJson(const DockingBlock& dockingBlock, nlohmann::json& j)
		{
			nlohmann::json dockingAsJson = {};
			dockingAsJson["nodes"] = nlohmann::json::array();
			for (const auto& line : dockingBlock.dockingLines)
			{
				nlohmann::json node = {};
				if (line.dockNodeData.has_value())
				{
					const DockNodeData& dockNode = line.dockNodeData.value();
					node["type"] = "DockNode";
					node["id"] = dockNode.id;
					node["parent"] = dockNode.parent;
					node["sizeRef"] = parseVec2(dockNode.sizeRef);
					node["split"] = nullptr;
					if (dockNode.split.has_value()) node["split"] = dockNode.split.value();
					node["centralNode"] = nullptr;
					if (dockNode.centralNode.has_value()) node["centralNode"] = dockNode.centralNode.value();
					node["selected"] = dockNode.selected;
				}
				else if (line.dockSpaceData.has_value())
				{
					const DockSpaceData& dockSpace = line.dockSpaceData.value();
					node["type"] = "DockSpace";
					node["id"] = dockSpace.id;
					node["window"] = dockSpace.window;
					node["pos"] = parseVec2(dockSpace.pos);
					node["size"] = parseVec2(dockSpace.size);
					node["split"] = dockSpace.split;
					node["selected"] = dockSpace.selected;
				}
				dockingAsJson["nodes"].emplace_back(node);
			}

			j["DockingData"] = dockingAsJson;
		}

		static bool parseWindowBlock(WindowBlock& output, ParserInfo& parser)
		{
			std::unordered_map<std::string, std::string> keyValuePairs = {};
			parseMultipleKeyValueEqualSignMultiline(keyValuePairs, parser);

			// Required attributes
			if (keyValuePairs.find("Collapsed") == keyValuePairs.end())
			{
				return false;
			}

			if (keyValuePairs.find("Size") == keyValuePairs.end())
			{
				return false;
			}

			// Add any attributes that exist
			output.collapsed = keyValuePairs["Collapsed"];
			output.size = keyValuePairs["Size"];
			output.pos = std::nullopt;
			output.dockId = std::nullopt;
			output.viewportId = std::nullopt;
			output.viewportPos = std::nullopt;

			if (keyValuePairs.find("Pos") != keyValuePairs.end())
			{
				output.pos = keyValuePairs["Pos"];
			}
			if (keyValuePairs.find("DockId") != keyValuePairs.end())
			{
				output.dockId = keyValuePairs["DockId"];
			}
			if (keyValuePairs.find("ViewportId") != keyValuePairs.end())
			{
				output.viewportId = keyValuePairs["ViewportId"];
			}
			if (keyValuePairs.find("ViewportPos") != keyValuePairs.end())
			{
				output.viewportPos = keyValuePairs["ViewportPos"];
			}

			return true;
		}

		static bool parseTableBlock(TableBlock& output, ParserInfo& parser)
		{
			while (Parser::isAlpha(Parser::peek(parser)))
			{
				StringView line = {};
				if (!parseLine(line, parser))
				{
					g_logger_error("Failed to parse line in table while parsing imgui.ini file.");
					return false;
				}
				output.lines.emplace_back(line.toString());
			}

			return true;
		}

		static bool parseDockingBlock(DockingBlock& output, ParserInfo& parser)
		{
			// Parse through all lines in [Docking][Data] section
			size_t indentationLevel = 0;
			while (Parser::isAlpha(Parser::peek(parser)))
			{
				// Parse the first identifier DockSpace or DockNode
				size_t identifierStart = parser.cursor;
				size_t identifierEnd = parser.cursor;
				while (!Parser::isWhitespace(Parser::peek(parser)))
				{
					Parser::advance(parser);
				}
				identifierEnd = parser.cursor;
				Parser::advance(parser);

				StringView identifier = {};
				if (!makeStringView(identifier, identifierStart, identifierEnd, parser))
				{
					g_logger_error("Failed to parse dock node in imgui.ini file.");
					return false;
				}

				std::string identifierStr = identifier.toString();
				DockingLine dockingLine = {};
				dockingLine.indentationLevel = indentationLevel;
				dockingLine.dockNodeData = std::nullopt;
				dockingLine.dockSpaceData = std::nullopt;

				if (identifierStr == "DockSpace")
				{
					if (!parseDockSpaceData(dockingLine.dockSpaceData, parser))
					{
						g_logger_error("Failed to parse DockSpaceData in imgui.ini file.");
						return false;
					}
				}
				else if (identifierStr == "DockNode")
				{
					if (!parseDockNodeData(dockingLine.dockNodeData, parser))
					{
						g_logger_error("Failed to parse DockSpaceData in imgui.ini file.");
						return false;
					}
				}

				output.dockingLines.emplace_back(dockingLine);

				// Is it tabs or spaces in the file?
				indentationLevel = 0;
				while (Parser::peek(parser) == ' ')
				{
					indentationLevel++;
					Parser::consume(parser, ' ');
				}
			}

			return true;
		}

		static bool parseDockSpaceData(std::optional<DockSpaceData>& output, ParserInfo& parser)
		{
			Parser::skipWhitespace(parser);

			std::unordered_map<std::string, std::string> keyValuePairs = {};
			if (!parseMultipleKeyValueEqualSign(keyValuePairs, parser))
			{
				return false;
			}

			if (!Parser::consumeNewline(parser))
			{
				if (!Parser::consume(parser, '\0'))
				{
					return false;
				}
			}

			// Make sure everything exists
			const std::string requiredKeys[] = {
				"ID", "Window", "Pos", "Size", "Split", "Selected"
			};
			for (const std::string& key : requiredKeys)
			{
				if (keyValuePairs.find(key) == keyValuePairs.end())
				{
					g_logger_error("Failed to parse imgui.ini file. DockSpace was missing required key '%s'", key.c_str());
					return false;
				}
			}

			DockSpaceData res = {};
			res.id = keyValuePairs["ID"];
			res.window = keyValuePairs["Window"];
			res.pos = keyValuePairs["Pos"];
			res.size = keyValuePairs["Size"];
			res.split = keyValuePairs["Split"];
			res.selected = keyValuePairs["Selected"];
			output = res;
			return true;
		}

		static bool parseDockNodeData(std::optional<DockNodeData>& output, ParserInfo& parser)
		{
			Parser::skipWhitespace(parser);

			std::unordered_map<std::string, std::string> keyValuePairs = {};
			if (!parseMultipleKeyValueEqualSign(keyValuePairs, parser))
			{
				return false;
			}

			if (!Parser::consumeNewline(parser))
			{
				if (!Parser::consume(parser, '\0'))
				{
					return false;
				}
			}

			// Make sure everything exists
			const std::string requiredKeys[] = {
				"ID", "Parent", "SizeRef", "Selected"
			};
			for (const std::string& key : requiredKeys)
			{
				if (keyValuePairs.find(key) == keyValuePairs.end())
				{
					g_logger_error("Failed to parse imgui.ini file. DockSpace was missing required key '%s'", key.c_str());
					return false;
				}
			}

			DockNodeData res = {};
			res.split = std::nullopt;
			res.centralNode = std::nullopt;

			res.id = keyValuePairs["ID"];
			res.parent = keyValuePairs["Parent"];
			res.sizeRef = keyValuePairs["SizeRef"];
			if (keyValuePairs.find("Split") != keyValuePairs.end())
			{
				res.split = keyValuePairs["Split"];
			}
			if (keyValuePairs.find("CentralNode") != keyValuePairs.end())
			{
				res.centralNode = keyValuePairs["CentralNode"];
			}
			res.selected = keyValuePairs["Selected"];
			output = res;
			return true;
		}

		static bool parseMultipleKeyValueEqualSignMultiline(std::unordered_map<std::string, std::string>& output, ParserInfo& parser)
		{
			while ((Parser::peek(parser) == '\n' || Parser::peek(parser) == '\r') && Parser::peek(parser) != '\0')
			{
				StringView key = {};
				StringView value = {};
				if (!parseKeyValueEqualSign(key, value, parser))
				{
					return false;
				}

				output.emplace(key.toString(), value.toString());
			}

			return true;
		}

		static bool parseMultipleKeyValueEqualSign(std::unordered_map<std::string, std::string>& output, ParserInfo& parser)
		{
			while ((Parser::peek(parser) != '\n' && Parser::peek(parser) != '\r') && Parser::peek(parser) != '\0')
			{
				StringView key = {};
				StringView value = {};
				if (!parseKeyValueEqualSign(key, value, parser))
				{
					return false;
				}

				output.emplace(key.toString(), value.toString());

				if (Parser::peek(parser) == ' ')
				{
					Parser::consume(parser, ' ');
				}
			}

			return true;
		}

		static bool parseKeyValueEqualSign(StringView& key, StringView& value, ParserInfo& parser)
		{
			Parser::skipWhitespace(parser);

			if (!Parser::isAlpha(Parser::peek(parser)))
			{
				return false;
			}

			// Parse key
			{
				size_t keyStart = parser.cursor;
				size_t keyEnd = keyStart;
				while (Parser::peek(parser) != '=')
				{
					if (Parser::peek(parser) == '\n' || Parser::peek(parser) == '\0' || Parser::peek(parser) == '\r')
					{
						break;
					}

					Parser::advance(parser);
				}

				if (!Parser::consume(parser, '='))
				{
					return false;
				}

				keyEnd = parser.cursor - 1;

				if (!makeStringView(key, keyStart, keyEnd, parser))
				{
					return false;
				}
			}

			// Parse value
			{
				size_t valueStart = parser.cursor;
				size_t valueEnd = valueStart;
				while (!Parser::isWhitespace(Parser::peek(parser)))
				{
					Parser::advance(parser);
				}

				// Could end on a newline or a \0 or a space
				if (!Parser::isWhitespace(Parser::peek(parser)))
				{
					return false;
				}

				valueEnd = parser.cursor;

				if (!makeStringView(value, valueStart, valueEnd, parser))
				{
					return false;
				}
			}

			return true;
		}

		static bool parseBracketPair(StringView& key, StringView& value, ParserInfo& parser)
		{
			Parser::skipWhitespace(parser);
			if (!parseBracketIdentifier(key, parser))
			{
				return false;
			}

			if (!parseBracketIdentifier(value, parser))
			{
				return false;
			}

			return true;
		}

		static bool parseBracketIdentifier(StringView& output, ParserInfo& parser)
		{
			if (!Parser::consume(parser, '['))
			{
				return false;
			}

			size_t identifierStart = parser.cursor;
			size_t identifierEnd = identifierStart;
			while (Parser::peek(parser) != ']')
			{
				if (Parser::peek(parser) == '\n' || Parser::peek(parser) == '\r' || Parser::peek(parser) == '\0')
				{
					break;
				}

				Parser::advance(parser);
			}

			if (!Parser::consume(parser, ']'))
			{
				return false;
			}

			identifierEnd = parser.cursor - 1;

			return makeStringView(output, identifierStart, identifierEnd, parser);
		}

		static bool makeStringView(StringView& output, size_t start, size_t end, ParserInfo& parser)
		{
			if (end - start > parser.textLength)
			{
				return false;
			}

			if (end < start)
			{
				return false;
			}

			if (end > parser.textLength)
			{
				return false;
			}

			output.source = parser.text;
			output.begin = start;
			output.length = end - start;
			return true;
		}

		static bool parseLine(StringView& output, ParserInfo& parser)
		{
			size_t lineStart = parser.cursor;
			size_t lineEnd = lineStart;
			while (Parser::peek(parser) != '\n' && Parser::peek(parser) != '\r' && Parser::peek(parser) != '\0')
			{
				Parser::advance(parser);
			}

			if (!Parser::consumeNewline(parser))
			{
				if (!Parser::consume(parser, '\0'))
				{
					return false;
				}
			}

			lineEnd = parser.cursor;
			return makeStringView(output, lineStart, lineEnd, parser);
		}

		static nlohmann::json parseVec2(const std::string& str)
		{
			ParserInfo parser = {};
			parser.cursor = 0;
			parser.text = str.c_str();
			parser.textLength = str.size();

			float x;
			if (!Parser::parseNumber(parser, &x))
			{
				return nullptr;
			}

			if (!Parser::consume(parser, ','))
			{
				return nullptr;
			}

			float y;
			if (!Parser::parseNumber(parser, &y))
			{
				return nullptr;
			}

			nlohmann::json res = {};
			res["X"] = x;
			res["Y"] = y;
			return res;
		}

		static Vec2 parseVec2(const nlohmann::json& j)
		{
			Vec2 res = {NAN, NAN};
			if (j.contains("X"))
			{
				res.x = j["X"];
			}

			if (j.contains("Y"))
			{
				res.y = j["Y"];
			}

			return res;
		}
	}
}

#pragma warning(pop)