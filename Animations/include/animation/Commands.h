#ifndef MATH_ANIM_COMMANDS_H
#define MATH_ANIM_COMMANDS_H
#include "core.h"

namespace MathAnim
{
	enum class CommandType : uint16
	{
		None = 0,
		DisplayGrid,
		Wait
	};

	namespace Commands
	{
		CommandType toCommandType(const std::string& command);
	}
}

#endif
