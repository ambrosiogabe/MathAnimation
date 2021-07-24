#include "animation/Commands.h"

namespace MathAnim
{
	namespace Commands
	{
		static std::unordered_map<std::string, CommandType> mCommandMap = {
			{ "display_grid", CommandType::DisplayGrid },
			{ "wait",         CommandType::Wait }
		};

		static std::string toLower(std::string s)
		{
			std::transform(s.begin(), s.end(), s.begin(),
				[](unsigned char c) { return std::tolower(c); } // correct
			);
			return s;
		}

		CommandType toCommandType(const std::string& command)
		{
			auto iter = mCommandMap.find(toLower(command));
			return iter == mCommandMap.end() ?
				CommandType::None :
				iter->second;
		}
	}
}
