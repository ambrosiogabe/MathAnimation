#include "editor/ConsoleLog.h"
#include "core/Colors.h"

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

		void info(const char* inFilename, int inLine, const char* format, ...)
		{
			LogEntry entry = {};
			entry.severity = Severity::Info;
			entry.filename = inFilename;
			entry.line = inLine;

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
				// Log to the actual console as well
				_g_logger_info(inFilename, inLine, "%s", entry.message.c_str());
				logHistory.push_back(entry);
			}
		}

		void warning(const char* inFilename, int inLine, const char* format, ...)
		{
			LogEntry entry = {};
			entry.severity = Severity::Warning;
			entry.filename = inFilename;
			entry.line = inLine;

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
				// Log to the actual console as well
				_g_logger_warning(inFilename, inLine, "%s", entry.message.c_str());
				logHistory.push_back(entry);
			}
		}

		void error(const char* inFilename, int inLine, const char* format, ...)
		{
			LogEntry entry = {};
			entry.severity = Severity::Error;
			entry.filename = inFilename;
			entry.line = inLine;

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
				// Log to the actual console as well
				_g_logger_error(inFilename, inLine, "%s", entry.message.c_str());
				logHistory.push_back(entry);
			}
		}

		void update()
		{
			while (logHistory.size() > maxLogHistorySize)
			{
				logHistory.erase(logHistory.begin());
			}

			ImGui::Begin("Console Output");

			// Iterate backwards since logs are usually read from most recent to least 
			// recent
			for (auto entry = logHistory.rbegin(); entry != logHistory.rend(); entry++)
			{
				if (entry->count > 0)
				{
					ImGui::Text("(%d)", entry->count);
					ImGui::SameLine();
				}

				if (entry->severity == Severity::Log)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, Colors::Primary[0]);
					ImGui::Text("Log");
				}
				else if (entry->severity == Severity::Info)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, Colors::AccentGreen[0]);
					ImGui::Text("Info");
				}
				else if (entry->severity == Severity::Warning)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, Colors::AccentYellow[0]);
					ImGui::Text("Warning");
				}
				else if (entry->severity == Severity::Error)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, Colors::AccentRed[0]);
					ImGui::Text("Error");
				}

				ImGui::SameLine();
				ImGui::Text("<%s:%d>", entry->filename.c_str(), entry->line);
				ImGui::PopStyleColor();

				ImGui::Text("%s", entry->message.c_str());

				ImGui::Separator();
			}

			ImGui::End();
		}
	}
}