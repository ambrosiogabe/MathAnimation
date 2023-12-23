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
      '\\n'
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
  '\\n");\n}\n'
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
      '\n\nint main()\n{\n  printf("Hello world!\\n");\n}\n'
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
R"_(<source.js>
  <storage.type.const.js>
    'const'
  </storage.type.const.js>
  ' '
  <constant.other.js>
    'foo'
  </constant.other.js>
  ' '
  <keyword.operator.assignment.js>
    '='
  </keyword.operator.assignment.js>
  ' '
  <meta.function.arrow.js>
    <meta.parameters.js>
      <punctuation.definition.parameters.begin.bracket.round.js>
        '('
      </punctuation.definition.parameters.begin.bracket.round.js>
      <punctuation.definition.parameters.end.bracket.round.js>
        ')'
      </punctuation.definition.parameters.end.bracket.round.js>
    </meta.parameters.js>
    ' '
    <storage.type.function.arrow.js>
      '=>'
    </storage.type.function.arrow.js>
  </meta.function.arrow.js>
  ' '
  <punctuation.definition.function.body.begin.bracket.curly.js>
    '{'
  </punctuation.definition.function.body.begin.bracket.curly.js>
  '\n  '
  <keyword.control.js>
    'return'
  </keyword.control.js>
  ' '
  <meta.brace.curly.js>
    '{'
  </meta.brace.curly.js>
  <string.quoted.single.js>
    ' '
    <punctuation.definition.string.begin.js>
      '\''
    </punctuation.definition.string.begin.js>
    'PI'
    <punctuation.definition.string.end.js>
      '\''
    </punctuation.definition.string.end.js>
  </string.quoted.single.js>
  <keyword.operator.assignment.js>
    ':'
  </keyword.operator.assignment.js>
  ' '
  <constant.numeric.decimal.js>
    '3'
    <meta.delimiter.decimal.period.js>
      '.'
    </meta.delimiter.decimal.period.js>
    '14'
  </constant.numeric.decimal.js>
  ' '
  <meta.brace.curly.js>
    '}'
  </meta.brace.curly.js>
  <punctuation.terminator.statement.js>
    ';'
  </punctuation.terminator.statement.js>
  '\n'
  <punctuation.definition.function.body.end.bracket.curly.js>
    '}'
  </punctuation.definition.function.body.end.bracket.curly.js>
  '\n'
</source.js>
)_";

// NOTE: This test case found an area where patterns that use anchors (stuff like \G or \A)
//       were not anchoring at the correct points.
constexpr const char* JS_ANCHORED_MATCHES_SRC = \
R"_(var sum = async function () {
  await this.a + this.b;
}
)_";

constexpr const char* JS_ANCHORED_MATCHES_EXPECTED = \
R"_(<source.js>
  <storage.type.var.js>
    'var'
  </storage.type.var.js>
  ' '
  <meta.function.js>
    <entity.name.function.js>
      'sum'
    </entity.name.function.js>
    ' '
    <keyword.operator.assignment.js>
      '='
    </keyword.operator.assignment.js>
    ' '
    <storage.modifier.async.js>
      'async'
    </storage.modifier.async.js>
    ' '
    <storage.type.function.js>
      'function'
    </storage.type.function.js>
    <meta.parameters.js>
      ' '
      <punctuation.definition.parameters.begin.bracket.round.js>
        '('
      </punctuation.definition.parameters.begin.bracket.round.js>
      <punctuation.definition.parameters.end.bracket.round.js>
        ')'
      </punctuation.definition.parameters.end.bracket.round.js>
    </meta.parameters.js>
  </meta.function.js>
  ' '
  <punctuation.definition.function.body.begin.bracket.curly.js>
    '{'
  </punctuation.definition.function.body.begin.bracket.curly.js>
  '\n  '
  <keyword.control.js>
    'await'
  </keyword.control.js>
  ' '
  <variable.language.js>
    'this'
  </variable.language.js>
  <meta.delimiter.property.period.js>
    '.'
  </meta.delimiter.property.period.js>
  <variable.other.property.js>
    'a'
  </variable.other.property.js>
  ' '
  <keyword.operator.js>
    '+'
  </keyword.operator.js>
  ' '
  <variable.language.js>
    'this'
  </variable.language.js>
  <meta.delimiter.property.period.js>
    '.'
  </meta.delimiter.property.period.js>
  <variable.other.property.js>
    'b'
  </variable.other.property.js>
  <punctuation.terminator.statement.js>
    ';'
  </punctuation.terminator.statement.js>
  '\n'
  <punctuation.definition.function.body.end.bracket.curly.js>
    '}'
  </punctuation.definition.function.body.end.bracket.curly.js>
  '\n'
</source.js>
)_";

constexpr const char* JS_INTERPOLATED_STRING_SRC = "`${this.a + this.b}`;";

constexpr const char* JS_INTERPOLATED_STRING_EXPECTED = \
R"_(<source.js>
  <string.quoted.template.js>
    <punctuation.definition.string.begin.js>
      '`'
    </punctuation.definition.string.begin.js>
    <source.js.embedded.source>
      <punctuation.section.embedded.js>
        '${'
      </punctuation.section.embedded.js>
      <variable.language.js>
        'this'
      </variable.language.js>
      <meta.delimiter.property.period.js>
        '.'
      </meta.delimiter.property.period.js>
      <variable.other.property.js>
        'a'
      </variable.other.property.js>
      ' '
      <keyword.operator.js>
        '+'
      </keyword.operator.js>
      ' '
      <variable.language.js>
        'this'
      </variable.language.js>
      <meta.delimiter.property.period.js>
        '.'
      </meta.delimiter.property.period.js>
      <variable.other.property.js>
        'b'
      </variable.other.property.js>
      <punctuation.section.embedded.js>
        '}'
      </punctuation.section.embedded.js>
    </source.js.embedded.source>
    <punctuation.definition.string.end.js>
      '`'
    </punctuation.definition.string.end.js>
  </string.quoted.template.js>
  <punctuation.terminator.statement.js>
    ';'
  </punctuation.terminator.statement.js>
</source.js>
)_";

constexpr const char* JS_TEST_BEGIN_CAPTURE_EXTENDING_BEYOND_MATCH_SRC = "console.log(\"Hello World!\");";

constexpr const char* JS_TEST_BEGIN_CAPTURE_EXTENDING_BEYOND_MATCH_EXPECTED = \
R"_(<source.js>
  <entity.name.type.object.console.js>
    'console'
  </entity.name.type.object.console.js>
  <meta.method-call.js>
    <meta.delimiter.method.period.js>
      '.'
    </meta.delimiter.method.period.js>
    <support.function.console.js>
      'log'
    </support.function.console.js>
    <meta.arguments.js>
      <punctuation.definition.arguments.begin.bracket.round.js>
        '('
      </punctuation.definition.arguments.begin.bracket.round.js>
      <string.quoted.double.js>
        <punctuation.definition.string.begin.js>
          '"'
        </punctuation.definition.string.begin.js>
        'Hello World!'
        <punctuation.definition.string.end.js>
          '"'
        </punctuation.definition.string.end.js>
      </string.quoted.double.js>
      <punctuation.definition.arguments.end.bracket.round.js>
        ')'
      </punctuation.definition.arguments.end.bracket.round.js>
    </meta.arguments.js>
  </meta.method-call.js>
  <punctuation.terminator.statement.js>
    ';'
  </punctuation.terminator.statement.js>
</source.js>
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