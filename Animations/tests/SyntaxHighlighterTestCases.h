#ifdef _MATH_ANIM_TESTS
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
R"_(<source.cpp>
  <meta.preprocessor.include.cpp>
    <keyword.control.directive.include.cpp>
      '#include'
    </keyword.control.directive.include.cpp>
    <string.quoted.other.ltgt.cpp>
      ' '
      <punctuation.definition.string.begin.cpp>
        '<'
      </punctuation.definition.string.begin.cpp>
      'stdio.h'
      <punctuation.definition.string.end.cpp>
        '>'
      </punctuation.definition.string.end.cpp>
    </string.quoted.other.ltgt.cpp>
    '\n'
  </meta.preprocessor.include.cpp>
  '\n'
  <keyword.other.type.cpp>
    'int'
  </keyword.other.type.cpp>
  ' '
  <entity.name.other.callable.cpp>
    'main'
  </entity.name.other.callable.cpp>
  <punctuation.section.begin.round.cpp>
    '('
  </punctuation.section.begin.round.cpp>
  <punctuation.section.end.round.cpp>
    ')'
  </punctuation.section.end.round.cpp>
  '\n'
  <punctuation.section.begin.curly.cpp>
    '{'
  </punctuation.section.begin.curly.cpp>
  '\n  '
  <entity.name.other.callable.cpp>
    'printf'
  </entity.name.other.callable.cpp>
  <punctuation.section.begin.round.cpp>
    '('
  </punctuation.section.begin.round.cpp>
  <string.quoted.double.cpp>
    <punctuation.definition.string.begin.cpp>
      '"'
    </punctuation.definition.string.begin.cpp>
    'Hello world!'
    <constant.character.escape>
      '\n'
    </constant.character.escape>
    <punctuation.definition.string.end.cpp>
      '"'
    </punctuation.definition.string.end.cpp>
  </string.quoted.double.cpp>
  <punctuation.section.end.round.cpp>
    ')'
  </punctuation.section.end.round.cpp>
  <punctuation.terminator.statement.cpp>
    ';'
  </punctuation.terminator.statement.cpp>
  '\n'
  <punctuation.section.end.curly.cpp>
    '}'
  </punctuation.section.end.curly.cpp>
  '\n'
</source.cpp>
)_";

constexpr const char* CPP_MAIN_TEST_WITH_GLSL_EXPECTED =
R"_(<source.glsl>
  <keyword.control.import.glsl>
    '#include'
  </keyword.control.import.glsl>
  ' '
  <string.quoted.include.glsl>
    '<stdio.h>'
  </string.quoted.include.glsl>
  '\n\n'
  <storage.type.glsl>
    'int'
  </storage.type.glsl>
  ' '
  <entity.name.function.glsl>
    'main'
  </entity.name.function.glsl>
  '()\n{\n  '
  <support.function.glsl>
    'printf'
  </support.function.glsl>
  '("Hello world'
  <keyword.operator.arithmetic.glsl>
    '!'
  </keyword.operator.arithmetic.glsl>
  '\n");\n}\n'
</source.glsl>
)_";

// NOTE: This covers a test case where I wasn't setting capture groups that
//       were defined as siblings as children when they end up becoming a child.
//       In this case, the decimal '.' should become a child of it's sibling capture
//       group 'constant.numeric.decimal.js' even though the rule that defines them
//       defines them as siblings.
constexpr const char* JS_NUMBER_LITERAL_TEST_SRC = "3.14";

constexpr const char* JS_NUMBER_LITERAL_TEST_EXPECTED = \
R"_(<source.js>
  <constant.numeric.decimal.js>
    '3'
    <meta.delimiter.decimal.period.js>
      '.'
    </meta.delimiter.decimal.period.js>
    '14'
  </constant.numeric.decimal.js>
</source.js>
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
R"_(<source.cpp>
  <meta.preprocessor.include.cpp>
    <keyword.control.directive.include.cpp>
      '#include'
    </keyword.control.directive.include.cpp>
    <string.quoted.other.ltgt.cpp>
      ' '
      <punctuation.definition.string.begin.cpp>
        '<'
      </punctuation.definition.string.begin.cpp>
      'stdio.h'
      <punctuation.definition.string.end.cpp>
        '>'
      </punctuation.definition.string.end.cpp>
    </string.quoted.other.ltgt.cpp>
    '\n'
    <keyword.control.directive.include.cpp>
      '#include'
    </keyword.control.directive.include.cpp>
    <string.quoted.other.ltgt.cpp>
      ' '
      <punctuation.definition.string.begin.cpp>
        '<'
      </punctuation.definition.string.begin.cpp>
      '\n\nint main()\n{\n  printf("Hello world!\n");\n}\n'
    </string.quoted.other.ltgt.cpp>
  </meta.preprocessor.include.cpp>
</source.cpp>
)_";

// NOTE: This tests a rule I had where the comment continued parsing until the end of the document
//       because I didn't capture the end of the line correctly
constexpr const char* CPP_SINGLE_LINE_COMMENT_TEST_SRC = \
R"_(// Comments
int foo;
)_";

constexpr const char* CPP_SINGLE_LINE_COMMENT_TEST_EXPECTED = \
R"_(<source.cpp>
  <comment.line.cpp>
    <punctuation.definition.comment.cpp>
      '//'
    </punctuation.definition.comment.cpp>
    ' Comments'
  </comment.line.cpp>
  '\n'
  <keyword.other.type.cpp>
    'int'
  </keyword.other.type.cpp>
  ' '
  <entity.name.other.unknown.cpp>
    'foo'
  </entity.name.other.unknown.cpp>
  <punctuation.terminator.statement.cpp>
    ';'
  </punctuation.terminator.statement.cpp>
  '\n'
</source.cpp>
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

// NOTE: This test case found an area where patterns that use anchors (stuff like \G or \A)
//       were not anchoring at the correct points.
constexpr const char* JS_ANCHORED_MATCHES_SRC = \
R"_(var sum = async function () {
  await this.a + this.b;
}
)_";

constexpr const char* JS_ANCHORED_MATCHES_EXPECTED = \
R"_('source.js': '<0, 57>'
  'storage.type.var.js': '<0, 3>'
    'ATOM': 'var'
  'ATOM': ' '
  'NULL_SCOPE': '<4, 52>'
    'meta.function.js': '<0, 23>'
      'entity.name.function.js': '<0, 3>'
        'ATOM': 'sum'
      'ATOM': ' '
      'keyword.operator.assignment.js': '<4, 1>'
        'ATOM': '='
      'ATOM': ' '
      'storage.modifier.async.js': '<6, 5>'
        'ATOM': 'async'
      'ATOM': ' '
      'storage.type.function.js': '<12, 8>'
        'ATOM': 'function'
      'ATOM': ' '
      'meta.parameters.js': '<21, 2>'
        'punctuation.definition.parameters.begin.bracket.round.js': '<0, 1>'
          'ATOM': '('
        'punctuation.definition.parameters.end.bracket.round.js': '<1, 1>'
          'ATOM': ')'
    'ATOM': ' '
    'NULL_SCOPE': '<24, 28>'
      'punctuation.definition.function.body.begin.bracket.curly.js': '<0, 1>'
        'ATOM': '{'
      'ATOM': '\n  '
      'keyword.control.js': '<4, 5>'
        'ATOM': 'await'
      'ATOM': ' '
      'variable.language.js': '<10, 4>'
        'ATOM': 'this'
      'meta.delimiter.property.period.js': '<14, 1>'
        'ATOM': '.'
      'variable.other.property.js': '<15, 1>'
        'ATOM': 'a'
      'ATOM': ' '
      'keyword.operator.js': '<17, 1>'
        'ATOM': '+'
      'ATOM': ' '
      'variable.language.js': '<19, 4>'
        'ATOM': 'this'
      'meta.delimiter.property.period.js': '<23, 1>'
        'ATOM': '.'
      'variable.other.property.js': '<24, 1>'
        'ATOM': 'b'
      'punctuation.terminator.statement.js': '<25, 1>'
        'ATOM': ';'
      'ATOM': '\n'
      'punctuation.definition.function.body.end.bracket.curly.js': '<27, 1>'
        'ATOM': '}'
)_";

constexpr const char* JS_INTERPOLATED_STRING_SRC = "`${this.a + this.b}`;";

constexpr const char* JS_INTERPOLATED_STRING_EXPECTED = \
R"_('source.js': '<0, 21>'
  'string.quoted.template.js': '<0, 20>'
    'punctuation.definition.string.begin.js': '<0, 1>'
      'ATOM': '`'
    'source.js.embedded.source': '<1, 18>'
      'punctuation.section.embedded.js': '<0, 2>'
        'ATOM': '${'
      'variable.language.js': '<2, 4>'
        'ATOM': 'this'
      'meta.delimiter.property.period.js': '<6, 1>'
        'ATOM': '.'
      'variable.other.property.js': '<7, 1>'
        'ATOM': 'a'
      'ATOM': ' '
      'keyword.operator.js': '<9, 1>'
        'ATOM': '+'
      'ATOM': ' '
      'variable.language.js': '<11, 4>'
        'ATOM': 'this'
      'meta.delimiter.property.period.js': '<15, 1>'
        'ATOM': '.'
      'variable.other.property.js': '<16, 1>'
        'ATOM': 'b'
      'punctuation.section.embedded.js': '<17, 1>'
        'ATOM': '}'
    'punctuation.definition.string.end.js': '<19, 1>'
      'ATOM': '`'
  'punctuation.terminator.statement.js': '<20, 1>'
    'ATOM': ';'
)_";

constexpr const char* JS_TEST_BEGIN_CAPTURE_EXTENDING_BEYOND_MATCH_SRC = "console.log(\"Hello World!\");";

constexpr const char* JS_TEST_BEGIN_CAPTURE_EXTENDING_BEYOND_MATCH_EXPECTED = \
R"_('source.js': '<0, 28>'
  'NULL_SCOPE': '<0, 27>'
    'entity.name.type.object.console.js': '<0, 7>'
      'ATOM': 'console'
    'meta.method-call.js': '<7, 20>'
      'meta.delimiter.method.period.js': '<0, 1>'
        'ATOM': '.'
      'support.function.console.js': '<1, 3>'
        'ATOM': 'log'
      'meta.arguments.js': '<4, 16>'
        'punctuation.definition.arguments.begin.bracket.round.js': '<0, 1>'
          'ATOM': '('
        'string.quoted.double.js': '<1, 14>'
          'punctuation.definition.string.begin.js': '<0, 1>'
            'ATOM': '"'
          'ATOM': 'Hello World!'
          'punctuation.definition.string.end.js': '<13, 1>'
            'ATOM': '"'
        'punctuation.definition.arguments.end.bracket.round.js': '<15, 1>'
          'ATOM': ')'
  'punctuation.terminator.statement.js': '<27, 1>'
    'ATOM': ';'
)_";

constexpr const char* JS_FOR_LOOP_SRC = \
R"_(for (const n in this.numbers) {
  console.log(`N: ${n}`);
})_";

constexpr const char* JS_FOR_LOOP_EXPECTED = \
R"_('source.js': '<0, 59>'
  'keyword.control.js': '<0, 3>'
    'ATOM': 'for'
  'ATOM': ' '
  'NULL_SCOPE': '<4, 25>'
    'meta.brace.round.js': '<0, 1>'
      'ATOM': '('
    'NULL_SCOPE': '<1, 10>'
      'storage.type.const.js': '<0, 5>'
        'ATOM': 'const'
      'ATOM': ' '
      'constant.other.js': '<6, 1>'
        'ATOM': 'n'
      'ATOM': ' '
      'keyword.operator.in[$1].js': '<8, 2>'
        'ATOM': 'in'
    'ATOM': ' '
    'variable.language.js': '<12, 4>'
      'ATOM': 'this'
    'meta.delimiter.property.period.js': '<16, 1>'
      'ATOM': '.'
    'variable.other.property.js': '<17, 7>'
      'ATOM': 'numbers'
    'meta.brace.round.js': '<24, 1>'
      'ATOM': ')'
  'ATOM': ' '
  'NULL_SCOPE': '<30, 29>'
    'meta.brace.curly.js': '<0, 1>'
      'ATOM': '{'
    'ATOM': '\n  '
    'NULL_SCOPE': '<4, 22>'
      'entity.name.type.object.console.js': '<0, 7>'
        'ATOM': 'console'
      'meta.method-call.js': '<7, 15>'
        'meta.delimiter.method.period.js': '<0, 1>'
          'ATOM': '.'
        'support.function.console.js': '<1, 3>'
          'ATOM': 'log'
        'meta.arguments.js': '<4, 11>'
          'punctuation.definition.arguments.begin.bracket.round.js': '<0, 1>'
            'ATOM': '('
          'string.quoted.template.js': '<1, 9>'
            'punctuation.definition.string.begin.js': '<0, 1>'
              'ATOM': '`'
            'ATOM': 'N: '
            'source.js.embedded.source': '<4, 4>'
              'punctuation.section.embedded.js': '<0, 2>'
                'ATOM': '${'
              'ATOM': 'n'
              'punctuation.section.embedded.js': '<3, 1>'
                'ATOM': '}'
            'punctuation.definition.string.end.js': '<8, 1>'
              'ATOM': '`'
          'punctuation.definition.arguments.end.bracket.round.js': '<10, 1>'
            'ATOM': ')'
    'punctuation.terminator.statement.js': '<26, 1>'
      'ATOM': ';'
    'ATOM': '\n'
    'meta.brace.curly.js': '<28, 1>'
      'ATOM': '}'
)_";

constexpr const char* JS_CAPTURE_IN_CAPTURE_SRC = "for (const foo in blah) {}";

constexpr const char* JS_CAPTURE_IN_CAPTURE_EXPECTED = \
R"_('source.js': '<0, 26>'
  'keyword.control.js': '<0, 3>'
    'ATOM': 'for'
  'ATOM': ' '
  'NULL_SCOPE': '<4, 19>'
    'meta.brace.round.js': '<0, 1>'
      'ATOM': '('
    'NULL_SCOPE': '<1, 12>'
      'storage.type.const.js': '<0, 5>'
        'ATOM': 'const'
      'ATOM': ' '
      'constant.other.js': '<6, 3>'
        'ATOM': 'foo'
      'ATOM': ' '
      'keyword.operator.in[$1].js': '<10, 2>'
        'ATOM': 'in'
    'ATOM': ' blah'
    'meta.brace.round.js': '<18, 1>'
      'ATOM': ')'
  'ATOM': ' '
  'punctuation.section.scope.begin.js': '<24, 1>'
    'ATOM': '{'
  'punctuation.section.scope.end.js': '<25, 1>'
    'ATOM': '}'
)_";

constexpr const char* LUA_NEWLINE_END_BLOCK_THING = \
R"_(--
function foo()
end)_";

constexpr const char* LUA_NEWLINE_END_BLOCK_THING_EXPECTED = \
R"_('source.lua': '<0, 21>'
  'NULL_SCOPE': '<0, 3>'
    'comment.line.double-dash.lua': '<0, 3>'
      'punctuation.definition.comment.lua': '<0, 2>'
        'ATOM': '--'
      'ATOM': '\n'
  'meta.function.lua': '<3, 14>'
    'keyword.control.lua': '<0, 8>'
      'ATOM': 'function'
    'ATOM': ' '
    'entity.name.function.lua': '<9, 3>'
      'ATOM': 'foo'
    'meta.parameter.lua': '<12, 2>'
      'punctuation.definition.parameters.begin.lua': '<0, 1>'
        'ATOM': '('
      'punctuation.definition.parameters.finish.lua': '<1, 1>'
        'ATOM': ')'
  'ATOM': '\n'
  'keyword.control.lua': '<18, 3>'
    'ATOM': 'end'
)_";

constexpr const char* LUA_BACKREFERENCE_TEST = \
R"_(--[===[
blah
]===])_";

constexpr const char* LUA_BACKREFERENCE_TEST_EXPECTED = \
R"_('source.lua': '<0, 18>'
  'NULL_SCOPE': '<0, 18>'
    'comment.block.lua': '<0, 18>'
      'punctuation.definition.comment.begin.lua': '<0, 7>'
        'ATOM': '--[===['
      'ATOM': '\nblah\n'
      'punctuation.definition.comment.end.lua': '<13, 5>'
        'ATOM': ']===]'
)_";

constexpr const char* LUA_BACKREFERENCE_0_SIZE = \
R"_(--[[
blah
]])_";

constexpr const char* LUA_BACKREFERENCE_0_SIZE_EXPECTED = \
R"_('source.lua': '<0, 12>'
  'NULL_SCOPE': '<0, 12>'
    'comment.block.lua': '<0, 12>'
      'punctuation.definition.comment.begin.lua': '<0, 4>'
        'ATOM': '--[['
      'ATOM': '\nblah\n'
      'punctuation.definition.comment.end.lua': '<10, 2>'
        'ATOM': ']]'
)_";

constexpr const char* LUA_BACKREFERENCE_MISMATCH = \
R"_(--[==[
blah
]])_";

constexpr const char* LUA_BACKREFERENCE_MISMATCH_EXPECTED = \
R"_('source.lua': '<0, 14>'
  'NULL_SCOPE': '<0, 14>'
    'comment.block.lua': '<0, 14>'
      'punctuation.definition.comment.begin.lua': '<0, 6>'
        'ATOM': '--[==['
      'ATOM': '\nblah\n]]'
)_";

// NOTE: This is for a scope capture that looks like foo.$1ter
//       where the scope should become the match followed by ter.
//       In this case foo.getter
constexpr const char* JS_COMPLEX_SCOPE_CAPTURE = \
R"_(class Foo {
  get bar() {
    return 0.1;
  }
}
)_";

constexpr const char* JS_COMPLEX_SCOPE_CAPTURE_EXPECTED = \
R"_('source.js': '<0, 48>'
  'meta.class.js': '<0, 9>'
    'storage.type.class.js': '<0, 5>'
      'ATOM': 'class'
    'ATOM': ' '
    'entity.name.type.class.js': '<6, 3>'
      'ATOM': 'Foo'
  'ATOM': ' '
  'NULL_SCOPE': '<10, 37>'
    'meta.brace.curly.js': '<0, 1>'
      'ATOM': '{'
    'ATOM': '\n  '
    'NULL_SCOPE': '<4, 31>'
      'meta.function.method.definition.js': '<0, 9>'
        'keyword.operator.getter[$1].js': '<0, 3>'
          'ATOM': 'get'
        'ATOM': ' '
        'entity.name.function.js': '<4, 3>'
          'ATOM': 'bar'
        'meta.parameters.js': '<7, 2>'
          'punctuation.definition.parameters.begin.bracket.round.js': '<0, 1>'
            'ATOM': '('
          'punctuation.definition.parameters.end.bracket.round.js': '<1, 1>'
            'ATOM': ')'
      'ATOM': ' '
      'NULL_SCOPE': '<10, 21>'
        'punctuation.definition.function.body.begin.bracket.curly.js': '<0, 1>'
          'ATOM': '{'
        'ATOM': '\n    '
        'keyword.control.js': '<6, 6>'
          'ATOM': 'return'
        'ATOM': ' '
        'constant.numeric.decimal.js': '<13, 3>'
          'ATOM': '0'
          'meta.delimiter.decimal.period.js': '<1, 1>'
            'ATOM': '.'
          'ATOM': '1'
        'punctuation.terminator.statement.js': '<16, 1>'
          'ATOM': ';'
        'ATOM': '\n  '
        'punctuation.definition.function.body.end.bracket.curly.js': '<20, 1>'
          'ATOM': '}'
    'ATOM': '\n'
    'meta.brace.curly.js': '<36, 1>'
      'ATOM': '}'
)_";

#endif
#endif // _MATH_ANIM_TESTS