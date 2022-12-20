#include "scripting/MathAnimGlobals.h"

namespace MathAnim
{
	namespace MathAnimGlobals
	{
        static const std::string builtinDefinitionLuaSrc = R"BUILTIN_SRC(
declare logger: {
    write: <T...>(T...) -> (),
    info: <T...>(T...) -> (),
    warn: <T...>(T...) -> (),
    error: <T...>(T...) -> ()
}

)BUILTIN_SRC";

        std::string getBuiltinDefinitionSource()
        {
            return builtinDefinitionLuaSrc;
        }
	}
}