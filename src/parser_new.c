#include <string.h>
#include <stdio.h>

#include "dparse.h"
#include "termproc.h"
#include "decllist.h"
#include "run.h"

extern D_ParserTables parser_tables_gram;

void parse_new(char *source) {
	D_Parser *parser = new_D_Parser(&parser_tables_gram, sizeof(D_ParseNode_User));
	// parser->save_parse_tree = 1;

	// not sure how these are used
    // parser->loc.pathname = "gaga";
    // parser->loc.line = 1;
    // parser->loc.col = 0;

	// If error_recovery == 1, dparser tries to recover from errors and might
	// call the grammar's actions on incorrect derivations!! (so some terms
	// might be missing) For the moment we execute the terms during parsing
	// (see parse_cmd_term) so we cannot accept this.
	//
	// However error_recovery == 1 produces better error messages (shows the location
	// of the error) so it's good to have. We should simply store the tree while
	// parsing and execute afterwards, if the parsing was successful.
	//
	// parser->error_recovery = 0;

	D_ParseNode *node = dparse(parser, source, strlen(source));
	if(node == NULL || parser->syntax_errors > 0)
		printf("Syntax errors found.\n");
	free_D_Parser(parser);
}

TERM *parse_variable(char *name) {
	TERM *t = termNew();
	t->type = TM_VAR;
	t->name = name;
	return t;
}

TERM *parse_alias(char *name) {
	TERM *t = termNew();
	t->type = TM_ALIAS;
	t->name = name;
	return t;
}

TERM *parse_abstraction(TERM *var, TERM *right) {
	TERM *t = termNew();
	t->type = TM_ABSTR;
	t->lterm = var;
	t->rterm = right;
	return t;
}

// static TERM *fix_application_grouping(TERM* op) {
// 	TERM *s2 = op->rterm;
// 	char breakOp = 0;

// 	// If s2 is an application, then op might need to "go inside" the application.
// 	// Eg. if op = * and s2 = (a + b) then instead of s1 * (a + b) we need to
// 	// get (s1 * a) + b, that is * should go inside +.
// 	// This happens in the following 2 cases:
// 	// 	- op has lower precedence than s2
// 	// 	- they have the same precedence and op is _not_ right-associative, hence it
// 	// 	  _cannot_ have s2 on its right. In this case:
// 	// 		* if s2 is left-associative (so it can have op on its left) then op goes inside
// 	// 		* otherwise there is an ambiguity and a warning is printed

// 	if(s2->type == TM_APPL && op->preced <= s2->preced) {
// 		if(op->preced < s2->preced || (op->assoc != ASS_RIGHT && s2->assoc == ASS_LEFT))
// 			breakOp = 1;
// 		else if(op->assoc != ASS_RIGHT)
// 			fprintf(stderr, "Warning: Precedence ambiguity between operators '%s' and '%s'. Use brackets.\n",
// 				(op->name ? op->name : "APPL"), (s2->name ? s2->name : "APPL"));
// 	}

// 	if(!breakOp)
// 		return op;

// 	op->rterm = s2->lterm;
// 	s2->lterm = fix_application_grouping(op);
// 	return s2;
// }

TERM *parse_application(TERM *left, char *oper_name, TERM *right) {
	TERM *t = termNew();
	t->type = TM_APPL;
	t->name = oper_name;
	t->lterm = left;
	t->rterm = right;

	// if(oper_name != NULL) {
	// 	OPER *oper = getOper(oper_name);
	// 	t->preced = (oper ? oper->preced : APPL_PRECED);
	// 	t->assoc = (oper ? oper->assoc : APPL_ASSOC);

	// 	t = fix_application_grouping(t);
	// }

	return t;
}

TERM *parse_number(char *s) {
	int num = 0;

	if(strlen(s) > 4)
		fprintf(stderr, "Error: integers must be in the range 0-9999. Changing to 0.\n");
	else
		num = atoi(s);

	return termChurchNum(num);
}

void parse_cmd_declaration(char *id, TERM *t) {
	termRemoveOper(t);
	termSetClosedFlag(t);

	if(t->closed)
		termAddDecl(id, t);
	else
		fprintf(stderr, "Error: alias %s is not a closed term and won't be registered\n", id);
}

void parse_cmd_term(TERM *t) {
	execTerm(t);
}