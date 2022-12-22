#include "editor/ConsoleLog.h"
#include "editor/ImGuiExtended.h"
#include "scripting/LuauLayer.h"
#include "core/Colors.h"
#include "core/ImGuiLayer.h"
#include "utils/FontAwesome.h"
#include "platform/Platform.h"

#include <imgui_internal.h>
#include <lua.h>
#include <lualib.h>

namespace MathAnim
{
	enum class Severity : uint8
	{
		Log,
		Info,
		Warning,
		Error
	};

	struct LogEntry
	{
		std::string filename;
		int line;
		Severity severity;
		std::string message;
		int count;
		ImVec2 size;

		bool operator==(const LogEntry& other)
		{
			return other.filename == filename &&
				other.line == line &&
				other.severity == severity &&
				other.message == message;
		}

		bool operator!=(const LogEntry& other)
		{
			return !(*this == other);
		}
	};

	namespace ConsoleLog
	{
		// --------- Internal Variables ---------
		static std::vector<LogEntry> logHistory;
		static constexpr size_t maxLogHistorySize = 500;

		static constexpr size_t formatBufferSize = 1024 * 10; // 10KB buffer for a log message seems reasonable /shrug 
		static char formatBuffer[formatBufferSize];
		static constexpr ImVec2 logPadding = ImVec2(7.0f, 20.0f);
		static constexpr float logPaddingBottomY = 12.0f;

		void log(const char* inFilename, int inLine, const char* format, ...)
		{
			LogEntry entry = {};
			entry.severity = Severity::Log;
			entry.filename = inFilename;
			entry.line = inLine;
			entry.size = ImVec2(1.0f, 1.0f);

			va_list args;
			va_start(args, format);
			vsnprintf(formatBuffer, formatBufferSize, format, args);
			va_end(args);

			entry.message = formatBuffer;

			if (logHistory.size() > 0 && entry == logHistory[logHistory.size() - 1])
			{
				logHistory[logHistory.size() - 1].count++;
			}
			else
			{
				logHistory.push_back(entry);
			}
		}

		void info(const char* inFilename, int inLine, const char* format, ...)
		{
			LogEntry entry = {};
			entry.severity = Severity::Info;
			entry.filename = inFilename;
			entry.line = inLine;
			entry.size = ImVec2(1.0f, 1.0f);

			va_list args;
			va_start(args, format);
			vsnprintf(formatBuffer, formatBufferSize, format, args);
			va_end(args);

			entry.message = formatBuffer;

			if (logHistory.size() > 0 && entry == logHistory[logHistory.size() - 1])
			{
				logHistory[logHistory.size() - 1].count++;
			}
			else
			{
				logHistory.push_back(entry);
			}
		}

		void warning(const char* inFilename, int inLine, const char* format, ...)
		{
			LogEntry entry = {};
			entry.severity = Severity::Warning;
			entry.filename = inFilename;
			entry.line = inLine;
			entry.size = ImVec2(1.0f, 1.0f);

			va_list args;
			va_start(args, format);
			vsnprintf(formatBuffer, formatBufferSize, format, args);
			va_end(args);

			entry.message = formatBuffer;

			if (logHistory.size() > 0 && entry == logHistory[logHistory.size() - 1])
			{
				logHistory[logHistory.size() - 1].count++;
			}
			else
			{
				logHistory.push_back(entry);
			}
		}

		void error(const char* inFilename, int inLine, const char* format, ...)
		{
			LogEntry entry = {};
			entry.severity = Severity::Error;
			entry.filename = inFilename;
			entry.line = inLine;
			entry.size = ImVec2(1.0f, 1.0f);

			va_list args;
			va_start(args, format);
			vsnprintf(formatBuffer, formatBufferSize, format, args);
			va_end(args);

			entry.message = formatBuffer;

			if (logHistory.size() > 0 && entry == logHistory[logHistory.size() - 1])
			{
				logHistory[logHistory.size() - 1].count++;
			}
			else
			{
				logHistory.push_back(entry);
			}
		}

		void update()
		{
			while (logHistory.size() > maxLogHistorySize)
			{
				logHistory.erase(logHistory.begin());
			}

			ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
			ImGui::Begin("Console Output", nullptr, ImGuiWindowFlags_MenuBar);

			if (ImGui::BeginMenuBar())
			{
				if (ImGui::Button("Clear"))
				{
					logHistory.clear();
				}

				ImGui::EndMenuBar();
			}

			// Iterate backwards since logs are usually read from most recent to least 
			// recent
			int uid = 0;
			ImGui::PushFont(ImGuiLayer::getMonoFont());
			for (auto entry = logHistory.rbegin(); entry != logHistory.rend(); entry++)
			{
				{
					ImVec2 cursorPos = ImGui::GetCursorScreenPos();
					ImGui::PushID(uid++);
					ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
					if (ImGui::Button("##Entry", ImVec2(ImGui::GetContentRegionAvail().x, entry->size.y)))
					{
						if (Platform::fileExists(entry->filename.c_str()))
						{
							if (!Platform::openFileWithVsCode(entry->filename.c_str(), entry->line))
							{
								// If the vscode command fails for whatever reason try to open the 
								// file using the default file command
								Platform::openFileWithDefaultProgram(entry->filename.c_str());
							}
						}
					}
					ImGui::PopStyleColor(2);
					ImGui::PopID();
					ImGui::SetCursorScreenPos(cursorPos);
				}

				ImGui::BeginGroup();
				ImGui::SetCursorPos(ImGui::GetCursorPos() + logPadding);

				if (entry->severity == Severity::Log)
				{
					ImGuiExtended::LargeIcon(ICON_FA_INFO_CIRCLE, Colors::Primary[1]);
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, Colors::Primary[0]);
					ImGui::BeginGroup();
					ImGui::Text("Log");
				}
				else if (entry->severity == Severity::Info)
				{
					ImGuiExtended::LargeIcon(ICON_FA_INFO_CIRCLE, Colors::AccentGreen[1]);
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, Colors::AccentGreen[0]);
					ImGui::BeginGroup();
					ImGui::Text("Info");
				}
				else if (entry->severity == Severity::Warning)
				{
					ImGuiExtended::LargeIcon(ICON_FA_EXCLAMATION_TRIANGLE, Colors::AccentYellow[2]);
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, Colors::AccentYellow[0]);
					ImGui::BeginGroup();
					ImGui::Text("Warning");
				}
				else if (entry->severity == Severity::Error)
				{
					ImGuiExtended::LargeIcon(ICON_FA_EXCLAMATION_TRIANGLE, Colors::AccentRed[2]);
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, Colors::AccentRed[0]);
					ImGui::BeginGroup();
					ImGui::Text("Error");
				}

				ImGui::SameLine();
				ImGui::Text("<%s:%d>", entry->filename.c_str(), entry->line);
				ImGui::PopStyleColor();

				if (entry->count > 0)
				{
					ImGui::SameLine();
					ImGui::Text("(%d)", entry->count);
				}

				ImGui::Text("%s", entry->message.c_str());
				ImGui::EndGroup();

				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.0f, logPaddingBottomY));
				ImGui::EndGroup();

				entry->size = ImGui::GetItemRectSize();

				ImGui::Separator();
			}

			ImGui::PopFont();
			ImGui::End();
			ImGui::PopStyleColor();
		}

		void log(lua_State* L, const char* format, ...)
		{
			int stackFrame = lua_stackdepth(L) - 1;
			lua_Debug debugInfo = {};
			lua_getinfo(L, stackFrame, "l", &debugInfo);
			const std::string& scriptFilepath = LuauLayer::getCurrentExecutingScriptFilepath();

			va_list args;
			va_start(args, format);
			vsnprintf(formatBuffer, formatBufferSize, format, args);
			va_end(args);

			ConsoleLog::log(scriptFilepath.c_str(), debugInfo.currentline, "%s", formatBuffer);
		}

		void info(lua_State* L, const char* format, ...)
		{
			int stackFrame = lua_stackdepth(L) - 1;
			lua_Debug debugInfo = {};
			lua_getinfo(L, stackFrame, "l", &debugInfo);
			const std::string& scriptFilepath = LuauLayer::getCurrentExecutingScriptFilepath();

			va_list args;
			va_start(args, format);
			vsnprintf(formatBuffer, formatBufferSize, format, args);
			va_end(args);

			ConsoleLog::info(scriptFilepath.c_str(), debugInfo.currentline, "%s", formatBuffer);
		}

		void warning(lua_State* L, const char* format, ...)
		{
			int stackFrame = lua_stackdepth(L) - 1;
			lua_Debug debugInfo = {};
			lua_getinfo(L, stackFrame, "l", &debugInfo);
			const std::string& scriptFilepath = LuauLayer::getCurrentExecutingScriptFilepath();

			va_list args;
			va_start(args, format);
			vsnprintf(formatBuffer, formatBufferSize, format, args);
			va_end(args);

			ConsoleLog::warning(scriptFilepath.c_str(), debugInfo.currentline, "%s", formatBuffer);
		}

		void error(lua_State* L, const char* format, ...)
		{
			int stackFrame = lua_stackdepth(L) - 1;
			lua_Debug debugInfo = {};
			lua_getinfo(L, stackFrame, "l", &debugInfo);
			const std::string& scriptFilepath = LuauLayer::getCurrentExecutingScriptFilepath();

			va_list args;
			va_start(args, format);
			vsnprintf(formatBuffer, formatBufferSize, format, args);
			va_end(args);

			ConsoleLog::error(scriptFilepath.c_str(), debugInfo.currentline, "%s", formatBuffer);
		}
	}
}