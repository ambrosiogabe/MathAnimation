#include "parsers/Sandbox.h"
#include "parsers/Grammar.h"
#include "parsers/Common.h"
#include "parsers/SyntaxTheme.h"
#include "parsers/SyntaxHighlighter.h"

namespace MathAnim
{
    static const char* testGrammarFile = "C:/dev/C++/MathAnimations/assets/defaultScripts/exampleGrammar.json";
    static const char* testThemeFile = "C:/dev/C++/MathAnimations/assets/defaultScripts/exampleTheme.json";
    static const char* exampleCode = R"EXAMPLE(
 #include <stdio.h>

 void main() 
 {
     printf("Hello World!");
     return 0;
 }
)EXAMPLE";

	namespace Sandbox
	{
        void testStuff()
        {
            SyntaxTheme* testTheme = SyntaxTheme::importTheme(testThemeFile);
			SyntaxHighlighter h = SyntaxHighlighter(testGrammarFile);
            h.parse(exampleCode, *testTheme);
            h.free();
            SyntaxTheme::free(testTheme);
            return;
        }
	}
}