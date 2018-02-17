/**


\page doc_script_bnf Script language grammar

This is language grammar in Backus-Naur Form (BNF).

The following convention is used:

<pre>
 ( )  - used for grouping
 { }  - 0 or more repetitions
 [ ]  - optional
  |   - or
 ' '  - token
</pre>

<pre>
SCRIPT        ::= {IMPORT | ENUM | TYPEDEF | CLASS | MIXIN | INTERFACE | FUNCDEF | VIRTPROP | VAR | FUNC | NAMESPACE | ';'}
VAR           ::= ['private'|'protected'] TYPE IDENTIFIER [( '=' (INITLIST | EXPR)) | ARGLIST] {',' IDENTIFIER [( '=' (INITLIST | EXPR)) | ARGLIST]} ';'
TYPEDEF       ::= 'typedef' PRIMTYPE IDENTIFIER ';'
FUNCDEF       ::= {'external' | 'shared'} 'funcdef' TYPE ['&'] IDENTIFIER PARAMLIST ';'
INTERFACE     ::= {'external' | 'shared'} 'interface' IDENTIFIER (';' | ([':' IDENTIFIER {',' IDENTIFIER}] '{' {VIRTPROP | INTFMTHD} '}'))
VIRTPROP      ::= ['private' | 'protected'] TYPE ['&'] IDENTIFIER '{' {('get' | 'set') ['const'] [('override' | 'final')] (STATBLOCK | ';')} '}'
FUNC          ::= {'shared' | 'external'} ['private' | 'protected'] [((TYPE ['&']) | '~')] IDENTIFIER PARAMLIST ['const'] {'override' | 'final'} (';' | STATBLOCK)
IMPORT        ::= 'import' TYPE ['&'] IDENTIFIER PARAMLIST 'from' STRING ';'
ENUM          ::= {'shared' | 'external'} 'enum' IDENTIFIER (';' | ('{' IDENTIFIER ['=' EXPR] {',' IDENTIFIER ['=' EXPR]} '}'))
NAMESPACE     ::= 'namespace' IDENTIFIER '{' SCRIPT '}'
MIXIN         ::= 'mixin' CLASS
CLASS         ::= {'shared' | 'abstract' | 'final' | 'external'} 'class' IDENTIFIER (';' | ([':' IDENTIFIER {',' IDENTIFIER}] '{' {VIRTPROP | FUNC | VAR | FUNCDEF} '}'))
INTFMTHD      ::= TYPE ['&'] IDENTIFIER PARAMLIST ['const'] ';'
STATBLOCK     ::= '{' {VAR | STATEMENT} '}'
PARAMLIST     ::= '(' ['void' | (TYPE TYPEMOD [IDENTIFIER] ['=' EXPR] {',' TYPE TYPEMOD [IDENTIFIER] ['=' EXPR]})] ')'
INITLIST      ::= '{' [ASSIGN | INITLIST] {',' [ASSIGN | INITLIST]} '}'
TYPEMOD       ::= ['&' ['in' | 'out' | 'inout']]
TYPE          ::= ['const'] SCOPE DATATYPE ['<' TYPE {',' TYPE} '>'] { ('[' ']') | '@' }
SCOPE         ::= ['::'] {IDENTIFIER '::'} [IDENTIFIER ['<' TYPE {',' TYPE} '>'] '::']
DATATYPE      ::= (IDENTIFIER | PRIMTYPE | '?' | 'auto')
PRIMTYPE      ::= 'void' | 'int' | 'int8' | 'int16' | 'int32' | 'int64' | 'uint' | 'uint8' | 'uint16' | 'uint32' | 'uint64' | 'float' | 'double' | 'bool'
STATEMENT     ::= (IF | FOR | WHILE | RETURN | STATBLOCK | BREAK | CONTINUE | DOWHILE | SWITCH | EXPRSTAT)
WHILE         ::= 'while' '(' ASSIGN ')' STATEMENT
BREAK         ::= 'break' ';'
FOR           ::= 'for' '(' (VAR | EXPRSTAT) EXPRSTAT [ASSIGN {',' ASSIGN}] ')' STATEMENT
CONTINUE      ::= 'continue' ';'
EXPRSTAT      ::= [ASSIGN] ';'
SWITCH        ::= 'switch' '(' ASSIGN ')' '{' {CASE} '}'
IF            ::= 'if' '(' ASSIGN ')' STATEMENT ['else' STATEMENT]
DOWHILE       ::= 'do' STATEMENT 'while' '(' ASSIGN ')' ';'
RETURN        ::= 'return' [ASSIGN] ';'
EXPR          ::= EXPRTERM {EXPROP EXPRTERM}
CASE          ::= (('case' EXPR) | 'default') ':' {STATEMENT}
EXPRTERM      ::= ([TYPE '='] INITLIST) | ({EXPRPREOP} EXPRVALUE {EXPRPOSTOP})
EXPRVALUE     ::= 'void' | CONSTRUCTCALL | FUNCCALL | VARACCESS | CAST | LITERAL | '(' ASSIGN ')' | LAMBDA
CONSTRUCTCALL ::= TYPE ARGLIST
EXPRPREOP     ::= '-' | '+' | '!' | '++' | '--' | '~' | '@'
EXPRPOSTOP    ::= ('.' (FUNCCALL | IDENTIFIER)) | ('[' [IDENTIFIER ':'] ASSIGN {',' [IDENTIFIER ':' ASSIGN} ']') | ARGLIST | '++' | '--'
CAST          ::= 'cast' '<' TYPE '>' '(' ASSIGN ')'
LITERAL       ::= NUMBER | STRING | BITS | 'true' | 'false' | 'null'
VARACCESS     ::= SCOPE IDENTIFIER
LAMBDA        ::= 'function' '(' [[TYPE TYPEMOD] IDENTIFIER {',' [TYPE TYPEMOD] IDENTIFIER}] ')' STATBLOCK
FUNCCALL      ::= SCOPE IDENTIFIER ARGLIST
ARGLIST       ::= '(' [IDENTIFIER ':'] ASSIGN {',' [IDENTIFIER ':'] ASSIGN} ')'
ASSIGN        ::= CONDITION [ ASSIGNOP ASSIGN ]
CONDITION     ::= EXPR ['?' ASSIGN ':' ASSIGN]
EXPROP        ::= MATHOP | COMPOP | LOGICOP | BITOP
ASSIGNOP      ::= '=' | '+=' | '-=' | '*=' | '/=' | '|=' | '&=' | '^=' | '%=' | '**=' | '<<=' | '>>=' | '>>>='
LOGICOP       ::= '&&' | '||' | '^^' | 'and' | 'or' | 'xor'
MATHOP        ::= '+' | '-' | '*' | '/' | '%' | '**'
COMPOP        ::= '==' | '!=' | '<' | '<=' | '>' | '>=' | 'is' | '!is'
BITOP         ::= '&' | '|' | '^' | '<<' | '>>' | '>>>'
IDENTIFIER    ::= single token:  starts with letter or _, can include any letter and digit, same as in C++
NUMBER        ::= single token:  includes integers and real numbers, same as C++
STRING        ::= single token:  single quoted ', double quoted ", or heredoc multi-line string """
BITS          ::= single token:  binary 0b or 0B, octal 0o or 0O, decimal 0d or 0D, hexadecimal 0x or 0X
</pre>




*/
