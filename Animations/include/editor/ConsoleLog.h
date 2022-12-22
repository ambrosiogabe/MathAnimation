#ifndef MATH_ANIM_CONSOLE_LOG_H
#define MATH_ANIM_CONSOLE_LOG_H
#include "core.h"

struct lua_State;

namespace MathAnim
{
	namespace ConsoleLog
	{
		void log(const char* inFilename, int inLine, const char* format, ...);
		void info(const char* filename, int line, const char* format, ...);
		void warning(const char* filename, int line, const char* format, ...);
		void error(const char* filename, int line, const char* format, ...);

		void update();

		void log(lua_State* L, const char* format, ...);
		void info(lua_State* L, const char* format, ...);
		void warning(lua_State* L, const char* format, ...);
		void error(lua_State* L, const char* format, ...);
	}
}

#define ConsoleLogLog(format, ...) MathAnim::ConsoleLog::log(__FILE__, __LINE__, format, __VA_ARGS__)
#define ConsoleLogInfo(format, ...) MathAnim::ConsoleLog::info(__FILE__, __LINE__, format, __VA_ARGS__)
#define ConsoleLogWarning(format, ...) MathAnim::ConsoleLog::warning(__FILE__, __LINE__, format, __VA_ARGS__)
#define ConsoleLogError(format, ...) MathAnim::ConsoleLog::error(__FILE__, __LINE__, format, __VA_ARGS__)

#endif 