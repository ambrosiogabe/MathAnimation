#include "editor/panels/RegexTester.h"

namespace MathAnim
{
	struct Match
	{
		std::string text;
		int start;
		int end;
	};

	namespace RegexTester
	{
		static constexpr size_t bufferSize = 1024;
		static char regexToTestBuffer[bufferSize] = {};
		static char stringToTestAgainst[bufferSize] = {};
		static bool multiline = false;
		static bool shouldShowWindow = false;

		static std::vector<Match> matches = {};

		void update()
		{
			if (!shouldShowWindow)
			{
				return;
			}

			ImGui::Begin("Regex Tester", &shouldShowWindow);

			if (ImGui::Checkbox(": Is Multiline", &multiline))
			{
				matches = {};
			}

			if (ImGui::InputTextMultiline(": Regex", regexToTestBuffer, bufferSize))
			{
				matches = {};
			}

			if (ImGui::InputTextMultiline(": String to Test", stringToTestAgainst, bufferSize))
			{
				matches = {};
			}

			if (ImGui::Button("Test"))
			{
				const char* pattern = regexToTestBuffer;
				const char* patternEnd = regexToTestBuffer + strlen(regexToTestBuffer);

				int options = ONIG_OPTION_NONE;
				options |= ONIG_OPTION_CAPTURE_GROUP;
				if (multiline)
				{
					options |= ONIG_OPTION_MULTILINE;
				}

				// Enable capture history for Oniguruma
				OnigSyntaxType syn;
				onig_copy_syntax(&syn, ONIG_SYNTAX_DEFAULT);
				onig_set_syntax_op2(&syn,
					onig_get_syntax_op2(&syn) | ONIG_SYN_OP2_ATMARK_CAPTURE_HISTORY);

				OnigRegex reg;
				OnigErrorInfo error;
				int parseRes = onig_new(
					&reg,
					(uint8*)pattern,
					(uint8*)patternEnd,
					options,
					ONIG_ENCODING_ASCII,
					&syn,
					&error
				);

				if (parseRes != ONIG_NORMAL)
				{
					char s[ONIG_MAX_ERROR_MESSAGE_LEN];
					onig_error_code_to_str((UChar*)s, parseRes, &error);
					g_logger_error("Oniguruma Error: '{}'", &s[0]);

					Match match = {};
					match.text = std::string("Oniguruma Error: ") + s;
					matches.emplace_back(match);
				}
				else
				{
					// Find and add all matches to list
					const char* targetStr = stringToTestAgainst;
					const char* targetStrEnd = targetStr + strlen(targetStr);

					const char* searchEnd = targetStr + strlen(targetStr);

					int searchRes = ONIG_MISMATCH;
					auto region = onig_region_new();

					while (targetStr < targetStrEnd)
					{
						const char* searchStart = targetStr;

						onig_region_clear(region);
						searchRes = onig_search(
							reg,
							(uint8*)targetStr,
							(uint8*)targetStrEnd,
							(uint8*)searchStart,
							(uint8*)searchEnd,
							region,
							ONIG_OPTION_NONE
						);

						if (searchRes >= 0)
						{
							int maxEnd = 0;
							for (int i = 0; i < region->num_regs; i++)
							{
								if (region->beg[i] >= 0 && region->end[i] >= region->beg[i])
								{
									int matchStart = region->beg[i];
									int matchEnd = region->end[i];

									Match match = {};
									match.text = std::string(targetStr + matchStart, matchEnd - matchStart);
									match.start = matchStart + (int)(targetStr - stringToTestAgainst);
									match.end = matchEnd + (int)(targetStr - stringToTestAgainst);
									matches.emplace_back(match);

									maxEnd = glm::max(matchEnd, maxEnd);
								}
							}

							targetStr += maxEnd;
							if (maxEnd == 0)
							{
								targetStr++;
							}

							if (multiline)
							{
								break;
							}
						}
						else if (searchRes != ONIG_MISMATCH)
						{
							// Error
							char s[ONIG_MAX_ERROR_MESSAGE_LEN];
							onig_error_code_to_str((UChar*)s, searchRes);
							g_logger_error("Oniguruma Error: '{}'", s);

							Match match = {};
							match.text = std::string("Oniguruma Error: ") + s;
							matches.push_back(match);
							break;
						}
						else
						{
							if (!multiline)
							{
								// No matches found on this line
								// Move to the next line to find the next match
								while (targetStr < targetStrEnd)
								{
									if (*targetStr == '\n')
									{
										targetStr++;
										break;
									}
									targetStr++;
								}
							}
							else
							{
								// If no multiline match was found, there are no matches left
								break;
							}
						}
					}

					onig_region_free(region, 1 /* 1:free self, 0:free contents only */);
				}

				onig_free(reg);
			}

			for (auto& match : matches)
			{
				ImGui::Text("Match<%d:%d>: '%s'", match.start, match.end, match.text.c_str());
			}

			ImGui::End();
		}

		void showWindow()
		{
			shouldShowWindow = true;
		}
	}
}