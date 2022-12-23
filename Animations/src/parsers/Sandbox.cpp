#include "parsers/Sandbox.h"
#include "parsers/Grammar.h"
#include "parsers/Common.h"

namespace MathAnim
{
    static const char* testGrammarFile = "C:/dev/C++/MathAnimations/assets/defaultScripts/exampleGrammar.json";

	namespace Sandbox
	{
        void testStuff()
        {
            Grammar test = Grammar::importGrammar(testGrammarFile);
            g_logger_info("Test done.");
            return;
        }
	}
}