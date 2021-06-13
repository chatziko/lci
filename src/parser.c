#include <string.h>
#include <stdio.h>

#include "dparse.h"
#include "termproc.h"
#include "decllist.h"
#include "run.h"

extern D_ParserTables parser_tables_gram;

int parse_string(char *source) {
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

	int success = node != NULL && parser->syntax_errors == 0;
	if(!success)
		printf("Syntax errors found.\n");

	free_D_Parser(parser);

	return success;
}

TERM *create_variable(char *name) {
	TERM *t = termNew();
	t->type = TM_VAR;
	t->name = name;
	return t;
}

TERM *create_alias(char *name) {
	TERM *t = termNew();
	t->type = TM_ALIAS;
	t->name = name;
	return t;
}

TERM *create_abstraction(TERM *var, TERM *right) {
	TERM *t = termNew();
	t->type = TM_ABSTR;
	t->lterm = var;
	t->rterm = right;
	return t;
}

static void get_assoc_preced(TERM *t, int *preced, ASS_TYPE *assoc) {
	OPER *oper = t->name != NULL ? getOper(t->name) : NULL;

	if(oper == NULL) {
		*preced = APPL_PRECED;
		*assoc = APPL_ASSOC;
	} else {
		*preced = oper->preced;
		*assoc = oper->assoc;
	}
}

// applications in lambda-calculus are left-associative "a b c = ((a b) c)"
// so grammar.g parses applications/operators in a left-associative way,
// and without any knowledge of precedece. Here we check whether
// (a OP' b) OP c should be changed to a OP' (b OP c) due to
// the precedence and/or associativity of OP/OP'.
//
// TODO: dparser supports user-defined scanners which allow to provide the precedence/associativity
//       of operators at runtime, to directly produce a correct parse. I gave it a quick try
//       but couldn't make it to work.

static TERM *fix_precedence(TERM* op) {
	TERM *left = op->lterm;
	if(left->type != TM_APPL || left->closed)	// "closed" means it is protected by parenthesis
		return op;

	// get associativity and precedence
	int op_preced, left_preced;
	ASS_TYPE op_assoc, left_assoc;
	get_assoc_preced(op, &op_preced, &op_assoc);
	get_assoc_preced(left, &left_preced, &left_assoc);

	// If left is an application, then op might need to "go inside" the application.
	// Eg. if op = * and left = (a + b) then instead of (a + b) * right we need to
	// get a + (b * right), that is * should go inside +.
	// This happens in the following 2 cases:
	// 	- op has lower precedence than left
	// 	- they have the same precedence and op is _not_ left-associative, hence it
	// 	  _cannot_ have 'left' on its left. In this case:
	// 		* if left is right-associative (so it can have op on its right) then op goes inside
	// 		* otherwise there is an ambiguity and a warning is printed

	char breakOp = 0;
	if(op_preced <= left_preced) {
		if(op_preced < left_preced || (op_assoc != ASS_LEFT && left_assoc == ASS_RIGHT))
			breakOp = 1;
		else if(op_assoc != ASS_LEFT)
			fprintf(stderr, "Warning: Precedence ambiguity between operators '%s' and '%s'. Use brackets.\n",
				(op->name ? op->name : "APPL"), (left->name ? left->name : "APPL"));
	}

	if(!breakOp)
		return op;

	op->lterm = left->rterm;
	left->rterm = fix_precedence(op);
	return left;
}

TERM *create_application(TERM *left, char *oper_name, TERM *right) {
	TERM *t = termNew();
	t->type = TM_APPL;
	t->name = oper_name;
	t->lterm = left;
	t->rterm = right;
	t->closed = 0;

	t = fix_precedence(t);

	return t;
}

TERM *create_number(char *s) {
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