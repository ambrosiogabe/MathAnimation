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
  'storage.type.glsl': '<20, 3>'
    'ATOM': 'int'
  'ATOM': ' '
  'entity.name.function.glsl': '<24, 4>'
    'ATOM': 'main'
  'ATOM': '('
  'invalid.illegal.stray-bracket-end.glsl': '<29, 1>'
    'ATOM': ')'
  'ATOM': '\n{\n  '
  'support.function.glsl': '<35, 6>'
    'ATOM': 'printf'
  'ATOM': '("Hello world'
  'keyword.operator.arithmetic.glsl': '<54, 1>'
    'ATOM': '!'
  'ATOM': '\n"'
  'invalid.illegal.stray-comment-end.glsl': '<58, 1>'
    'ATOM': ')'
  'ATOM': ';\n'
  'invalid.illegal.stray-bracket-end.glsl': '<61, 1>'
    'ATOM': '}'
)_";

constexpr const char* JS_NUMBER_LITERAL_TEST_SRC = "3.14";

constexpr const char* JS_NUMBER_LITERAL_TEST_EXPECTED = \
R"_('source.js': '<0, 4>'
  'constant.numeric.decimal.js': '<0, 4>'
    'ATOM': '3'
    'meta.delimiter.decimal.period.js': '<1, 1>'
      'ATOM': '.'
    'ATOM': '14'
)_";

#endif