#ifndef MATH_ANIM_SYNTAX_HIGHLIGHTER_TEST_CASES_H
#define MATH_ANIM_SYNTAX_HIGHLIGHTER_TEST_CASES_H

constexpr const char* CPP_MAIN_TEST_SRC = \
R"_(#include <stdio.h>

int main()
{
  printf("Hello world!\n");
}
)_";

constexpr const char* CPP_MAIN_TEST_EXPECTED = \
R"_('source.cpp': '<0, 63>'
  'meta.preprocessor.include.cpp': '<0, 19>'
    'keyword.control.directive.include.cpp': '<0, 8>'
      'ATOM': '#include'
    'ATOM': ' '
    'string.quoted.other.ltgt.cpp': '<9, 9>'
      'punctuation.definition.string.begin.cpp': '<0, 1>'
        'ATOM': '<'
      'ATOM': 'stdio.h'
      'punctuation.definition.string.end.cpp': '<8, 1>'
        'ATOM': '>'
    'ATOM': '\n'
  'ATOM': '\n'
  'keyword.other.type.cpp': '<20, 3>'
    'ATOM': 'int'
  'ATOM': ' '
  'entity.name.other.callable.cpp': '<24, 4>'
    'ATOM': 'main'
  'punctuation.section.begin.round.cpp': '<28, 1>'
    'ATOM': '('
  'punctuation.section.end.round.cpp': '<29, 1>'
    'ATOM': ')'
  'ATOM': '\n'
  'punctuation.section.begin.curly.cpp': '<31, 1>'
    'ATOM': '{'
  'ATOM': '\n  '
  'entity.name.other.callable.cpp': '<35, 6>'
    'ATOM': 'printf'
  'punctuation.section.begin.round.cpp': '<41, 1>'
    'ATOM': '('
  'string.quoted.double.cpp': '<42, 16>'
    'punctuation.definition.string.begin.cpp': '<0, 1>'
      'ATOM': '"'
    'ATOM': 'Hello world!'
    'constant.character.escape': '<13, 2>'
      'ATOM': '\n'
    'punctuation.definition.string.end.cpp': '<15, 1>'
      'ATOM': '"'
  'punctuation.section.end.round.cpp': '<58, 1>'
    'ATOM': ')'
  'punctuation.terminator.statement.cpp': '<59, 1>'
    'ATOM': ';'
  'ATOM': '\n'
  'punctuation.section.end.curly.cpp': '<61, 1>'
    'ATOM': '}'
)_";

constexpr const char* CPP_MAIN_TEST_WITH_GLSL_EXPECTED =
R"_('source.glsl': '<0, 63>'
  'keyword.control.import.glsl': '<0, 8>'
    'ATOM': '#include'
  'ATOM': ' '
  'string.quoted.include.glsl': '<9, 9>'
    'ATOM': '<stdio.h>'
  'ATOM': '\n\n'
  'NULL_SCOPE': '<20, 10>'
    'storage.type.glsl': '<0, 3>'
      'ATOM': 'int'
    'ATOM': ' '
    'entity.name.function.glsl': '<4, 4>'
      'ATOM': 'main'
    'ATOM': '()'
  'ATOM': '\n'
  'NULL_SCOPE': '<31, 31>'
    'ATOM': '{\n  '
    'NULL_SCOPE': '<4, 24>'
      'support.function.glsl': '<0, 6>'
        'ATOM': 'printf'
      'ATOM': '("Hello world'
      'keyword.operator.arithmetic.glsl': '<19, 1>'
        'ATOM': '!'
      'ATOM': '\n")'
    'ATOM': ';\n}'
)_";

// NOTE: This covers a test case where I wasn't setting capture groups that
//       were defined as siblings as children when they end up becoming a child.
//       In this case, the decimal '.' should become a child of it's sibling capture
//       group 'constant.numeric.decimal.js' even though the rule that defines them
//       defines them as siblings.
constexpr const char* JS_NUMBER_LITERAL_TEST_SRC = "3.14";

constexpr const char* JS_NUMBER_LITERAL_TEST_EXPECTED = \
R"_('source.js': '<0, 4>'
  'constant.numeric.decimal.js': '<0, 4>'
    'ATOM': '3'
    'meta.delimiter.decimal.period.js': '<1, 1>'
      'ATOM': '.'
    'ATOM': '14'
)_";

// NOTE: This covers a test case where I was incorrectly continuing to parse a document after 
//       a submatch extended to the end of the document. This led to an incorrect parse tree.
constexpr const char* CPP_STRAY_BRACKET_TEST_SRC = \
R"_(#include <stdio.h>
#include <

int main()
{
  printf("Hello world!\n");
}
)_";

constexpr const char* CPP_STRAY_BRACKET_TEST_EXPECTED = \
R"_('source.cpp': '<0, 74>'
  'meta.preprocessor.include.cpp': '<0, 19>'
    'keyword.control.directive.include.cpp': '<0, 8>'
      'ATOM': '#include'
    'ATOM': ' '
    'string.quoted.other.ltgt.cpp': '<9, 9>'
      'punctuation.definition.string.begin.cpp': '<0, 1>'
        'ATOM': '<'
      'ATOM': 'stdio.h'
      'punctuation.definition.string.end.cpp': '<8, 1>'
        'ATOM': '>'
    'ATOM': '\n'
  'meta.preprocessor.include.cpp': '<19, 55>'
    'keyword.control.directive.include.cpp': '<0, 8>'
      'ATOM': '#include'
    'ATOM': ' '
    'string.quoted.other.ltgt.cpp': '<9, 46>'
      'punctuation.definition.string.begin.cpp': '<0, 1>'
        'ATOM': '<'
      'ATOM': '\n\nint main()\n{\n  printf("Hello world!\n");\n}\n'
)_";

// NOTE: This tests a rule I had where the comment continued parsing until the end of the document
//       because I didn't capture the end of the line correctly
constexpr const char* CPP_SINGLE_LINE_COMMENT_TEST_SRC = \
R"_(// Comments
int foo;
)_";

constexpr const char* CPP_SINGLE_LINE_COMMENT_TEST_EXPECTED = \
R"_('source.cpp': '<0, 21>'
  'comment.line.cpp': '<0, 11>'
    'punctuation.definition.comment.cpp': '<0, 2>'
      'ATOM': '//'
    'ATOM': ' Comments'
  'ATOM': '\n'
  'keyword.other.type.cpp': '<12, 3>'
    'ATOM': 'int'
  'ATOM': ' '
  'entity.name.other.unknown.cpp': '<16, 3>'
    'ATOM': 'foo'
  'punctuation.terminator.statement.cpp': '<19, 1>'
    'ATOM': ';'
)_";

constexpr const char* JS_BASIC_ARROW_FN_TEST_SRC = \
R"_(const foo = () => {
  return { 'PI': 3.14 };
}
)_";

constexpr const char* JS_BASIC_ARROW_FN_TEST_EXPECTED = \
R"_('source.js': '<0, 47>'
  'NULL_SCOPE': '<0, 11>'
    'storage.type.const.js': '<0, 5>'
      'ATOM': 'const'
    'ATOM': ' '
    'constant.other.js': '<6, 3>'
      'ATOM': 'foo'
    'ATOM': ' '
    'keyword.operator.assignment.js': '<10, 1>'
      'ATOM': '='
  'ATOM': ' '
  'NULL_SCOPE': '<12, 34>'
    'meta.function.arrow.js': '<0, 5>'
      'meta.parameters.js': '<0, 2>'
        'punctuation.definition.parameters.begin.bracket.round.js': '<0, 1>'
          'ATOM': '('
        'punctuation.definition.parameters.end.bracket.round.js': '<1, 1>'
          'ATOM': ')'
      'ATOM': ' '
      'storage.type.function.arrow.js': '<3, 2>'
        'ATOM': '=>'
    'ATOM': ' '
    'NULL_SCOPE': '<6, 28>'
      'punctuation.definition.function.body.begin.bracket.curly.js': '<0, 1>'
        'ATOM': '{'
      'ATOM': '\n  '
      'keyword.control.js': '<4, 6>'
        'ATOM': 'return'
      'ATOM': ' '
      'NULL_SCOPE': '<11, 14>'
        'meta.brace.curly.js': '<0, 1>'
          'ATOM': '{'
        'ATOM': ' '
        'string.quoted.single.js': '<2, 4>'
          'punctuation.definition.string.begin.js': '<0, 1>'
            'ATOM': '''
          'ATOM': 'PI'
          'punctuation.definition.string.end.js': '<3, 1>'
            'ATOM': '''
        'keyword.operator.assignment.js': '<6, 1>'
          'ATOM': ':'
        'ATOM': ' '
        'constant.numeric.decimal.js': '<8, 4>'
          'ATOM': '3'
          'meta.delimiter.decimal.period.js': '<1, 1>'
            'ATOM': '.'
          'ATOM': '14'
        'ATOM': ' '
        'meta.brace.curly.js': '<13, 1>'
          'ATOM': '}'
      'punctuation.terminator.statement.js': '<25, 1>'
        'ATOM': ';'
      'ATOM': '\n'
      'punctuation.definition.function.body.end.bracket.curly.js': '<27, 1>'
        'ATOM': '}'
)_";

#endif