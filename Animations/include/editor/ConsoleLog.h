#ifndef MATH_ANIM_CONSOLE_LOG_H
#define MATH_ANIM_CONSOLE_LOG_H
#include "core.h"

namespace MathAnim
{
	namespace ConsoleLog
	{
		void log(const char* inFilename, int inLine, const char* format, ...);
		void info(const char* filename, int line, const char* format, ...);
		void warning(const char* filename, int line, const char* format, ...);
		void error(const char* filename, int line, const char* format, ...);

		void update();
	}
}

#define ConsoleLogLog(format, ...) MathAnim::ConsoleLog::log(__FILE__, __LINE__, format, __VA_ARGS__)
#define ConsoleLogInfo(format, ...) MathAnim::ConsoleLog::info(__FILE__, __LINE__, format, __VA_ARGS__)
#define ConsoleLogWarning(format, ...) MathAnim::ConsoleLog::warning(__FILE__, __LINE__, format, __VA_ARGS__)
#define ConsoleLogError(format, ...) MathAnim::ConsoleLog::error(__FILE__, __LINE__, format, __VA_ARGS__)

#endif 