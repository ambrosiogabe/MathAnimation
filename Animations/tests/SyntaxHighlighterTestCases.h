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
    ' '
    <string.quoted.other.ltgt.cpp>
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
    ' '
    <string.quoted.other.ltgt.cpp>
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
    ' '
    <string.quoted.other.ltgt.cpp>
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
  ' '
  <string.quoted.single.js>
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
    ' '
    <meta.parameters.js>
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
R"_(<source.js>
  <keyword.control.js>
    'for'
  </keyword.control.js>
  ' '
  <meta.brace.round.js>
    '('
  </meta.brace.round.js>
  <storage.type.const.js>
    'const'
  </storage.type.const.js>
  ' '
  <constant.other.js>
    'n'
  </constant.other.js>
  ' '
  <keyword.operator.in[$1].js>
    'in'
  </keyword.operator.in[$1].js>
  ' '
  <variable.language.js>
    'this'
  </variable.language.js>
  <meta.delimiter.property.period.js>
    '.'
  </meta.delimiter.property.period.js>
  <variable.other.property.js>
    'numbers'
  </variable.other.property.js>
  <meta.brace.round.js>
    ')'
  </meta.brace.round.js>
  ' '
  <meta.brace.curly.js>
    '{'
  </meta.brace.curly.js>
  '\n  '
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
      <string.quoted.template.js>
        <punctuation.definition.string.begin.js>
          '`'
        </punctuation.definition.string.begin.js>
        'N: '
        <source.js.embedded.source>
          <punctuation.section.embedded.js>
            '${'
          </punctuation.section.embedded.js>
          'n'
          <punctuation.section.embedded.js>
            '}'
          </punctuation.section.embedded.js>
        </source.js.embedded.source>
        <punctuation.definition.string.end.js>
          '`'
        </punctuation.definition.string.end.js>
      </string.quoted.template.js>
      <punctuation.definition.arguments.end.bracket.round.js>
        ')'
      </punctuation.definition.arguments.end.bracket.round.js>
    </meta.arguments.js>
  </meta.method-call.js>
  <punctuation.terminator.statement.js>
    ';'
  </punctuation.terminator.statement.js>
  '\n'
  <meta.brace.curly.js>
    '}'
  </meta.brace.curly.js>
</source.js>
)_";

constexpr const char* JS_CAPTURE_IN_CAPTURE_SRC = "for (const foo in blah) {}";

constexpr const char* JS_CAPTURE_IN_CAPTURE_EXPECTED = \
R"_(<source.js>
  <keyword.control.js>
    'for'
  </keyword.control.js>
  ' '
  <meta.brace.round.js>
    '('
  </meta.brace.round.js>
  <storage.type.const.js>
    'const'
  </storage.type.const.js>
  ' '
  <constant.other.js>
    'foo'
  </constant.other.js>
  ' '
  <keyword.operator.in[$1].js>
    'in'
  </keyword.operator.in[$1].js>
  ' blah'
  <meta.brace.round.js>
    ')'
  </meta.brace.round.js>
  ' '
  <punctuation.section.scope.begin.js>
    '{'
  </punctuation.section.scope.begin.js>
  <punctuation.section.scope.end.js>
    '}'
  </punctuation.section.scope.end.js>
</source.js>
)_";

constexpr const char* LUA_NEWLINE_END_BLOCK_THING = \
R"_(--
function foo()
end)_";

constexpr const char* LUA_NEWLINE_END_BLOCK_THING_EXPECTED = \
R"_(<source.lua>
  <comment.line.double-dash.lua>
    <punctuation.definition.comment.lua>
      '--'
    </punctuation.definition.comment.lua>
    '\n'
  </comment.line.double-dash.lua>
  <meta.function.lua>
    <keyword.control.lua>
      'function'
    </keyword.control.lua>
    ' '
    <entity.name.function.lua>
      'foo'
    </entity.name.function.lua>
    <meta.parameter.lua>
      <punctuation.definition.parameters.begin.lua>
        '('
      </punctuation.definition.parameters.begin.lua>
      <punctuation.definition.parameters.finish.lua>
        ')'
      </punctuation.definition.parameters.finish.lua>
    </meta.parameter.lua>
  </meta.function.lua>
  '\n'
  <keyword.control.lua>
    'end'
  </keyword.control.lua>
</source.lua>
)_";

constexpr const char* LUA_BACKREFERENCE_TEST = \
R"_(--[===[
blah
]===])_";

constexpr const char* LUA_BACKREFERENCE_TEST_EXPECTED = \
R"_(<source.lua>
  <comment.block.lua>
    <punctuation.definition.comment.begin.lua>
      '--[===['
    </punctuation.definition.comment.begin.lua>
    '\nblah\n'
    <punctuation.definition.comment.end.lua>
      ']===]'
    </punctuation.definition.comment.end.lua>
  </comment.block.lua>
</source.lua>
)_";

constexpr const char* LUA_BACKREFERENCE_0_SIZE = \
R"_(--[[
blah
]])_";

constexpr const char* LUA_BACKREFERENCE_0_SIZE_EXPECTED = \
R"_(<source.lua>
  <comment.block.lua>
    <punctuation.definition.comment.begin.lua>
      '--[['
    </punctuation.definition.comment.begin.lua>
    '\nblah\n'
    <punctuation.definition.comment.end.lua>
      ']]'
    </punctuation.definition.comment.end.lua>
  </comment.block.lua>
</source.lua>
)_";

constexpr const char* LUA_BACKREFERENCE_MISMATCH = \
R"_(--[==[
blah
]])_";

constexpr const char* LUA_BACKREFERENCE_MISMATCH_EXPECTED = \
R"_(<source.lua>
  <comment.block.lua>
    <punctuation.definition.comment.begin.lua>
      '--[==['
    </punctuation.definition.comment.begin.lua>
    '\nblah\n]]'
  </comment.block.lua>
</source.lua>
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
R"_(<source.js>
  <meta.class.js>
    <storage.type.class.js>
      'class'
    </storage.type.class.js>
    ' '
    <entity.name.type.class.js>
      'Foo'
    </entity.name.type.class.js>
  </meta.class.js>
  ' '
  <meta.brace.curly.js>
    '{'
  </meta.brace.curly.js>
  '\n  '
  <meta.function.method.definition.js>
    <keyword.operator.getter[$1].js>
      'get'
    </keyword.operator.getter[$1].js>
    ' '
    <entity.name.function.js>
      'bar'
    </entity.name.function.js>
    <meta.parameters.js>
      <punctuation.definition.parameters.begin.bracket.round.js>
        '('
      </punctuation.definition.parameters.begin.bracket.round.js>
      <punctuation.definition.parameters.end.bracket.round.js>
        ')'
      </punctuation.definition.parameters.end.bracket.round.js>
    </meta.parameters.js>
  </meta.function.method.definition.js>
  ' '
  <punctuation.definition.function.body.begin.bracket.curly.js>
    '{'
  </punctuation.definition.function.body.begin.bracket.curly.js>
  '\n    '
  <keyword.control.js>
    'return'
  </keyword.control.js>
  ' '
  <constant.numeric.decimal.js>
    '0'
    <meta.delimiter.decimal.period.js>
      '.'
    </meta.delimiter.decimal.period.js>
    '1'
  </constant.numeric.decimal.js>
  <punctuation.terminator.statement.js>
    ';'
  </punctuation.terminator.statement.js>
  '\n  '
  <punctuation.definition.function.body.end.bracket.curly.js>
    '}'
  </punctuation.definition.function.body.end.bracket.curly.js>
  '\n'
  <meta.brace.curly.js>
    '}'
  </meta.brace.curly.js>
  '\n'
</source.js>
)_";

#endif
#endif // _MATH_ANIM_TESTS