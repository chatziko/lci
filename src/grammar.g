{
	#include <string.h>

	#include "dparse.h"
	#include "parser.h"
	#include "str_intern.h"

	// The type of <dollar> variables. Default is void*
	// typedef struct { ... } ParseNode;
	// #define D_ParseNode_User ParseNode

	// returns the contents of a node as an interned string
	char *str(D_ParseNode node) {
		return str_intern_range(node.start_loc.s, node.end);
	}

	int is_reserved_oper(char *oper) {
		return strcmp(oper, "=") == 0;
	}
}

// NOTES
// $#n            number of children of node n
// ${child 1,0}   child 0 of child 1 (returns the node, use node->user for the data)

program: command ( ';' command )* ';'?;

command
	: alias '=' term 				{ parse_cmd_declaration($0, $2); }
    | '?'? term						{ parse_cmd_term($1); };

term
	: variable						{ $$ = create_variable($0); }
    | number						{ $$ = create_number($0); }
	| alias							{ $$ = create_alias($0); }
	| '(' term ')'					{ $$ = $1; }
	| term operator? term $left 1	{ char *op = $#1 ? ${child 1,0}->user : NULL;
									  $$ = create_application($0, op, $2); }
	| lambda variable '.' term		{ $$ = create_abstraction(create_variable($1), $3); };

lambda: '\\' | 'λ';

alias: "[A-Z][a-zA-Z0-9_]*"			{ $$ = str($n); }
     | "'" "[^']+" "'"				{ $$ = str($n1); };
variable: "[a-z_][a-zA-Z0-9_]*"		{ $$ = str($n); };
number: "[0-9]+"					{ $$ = str($n); };
operator: "[+\-=!@$%^&*/:<>.,|~?]+"	[ $$ = str($n); if(is_reserved_oper($$)) ${reject}; ];

whitespace: "([ \t\n]|#[^\n]*)*";	// '# comment' (until the end the line) is treated as whitespace
