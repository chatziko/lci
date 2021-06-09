// vim:noet:ts=3

/* Definition of the finite state machine and the grammar rules
	that are used by the parser

	Copyright (C) 2004-8 Kostas Chatzikokolakis
	This file is part of LCI

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details. */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "grammar.h"
#include "termproc.h"
#include "decllist.h"
#include "run.h"


// Valid language characters
// Characters inside each string belong to the same class
// The finite automaton fsm defines transitions for each character class

char *validChars[VALIDCHNO] =	{
	"abcdefghijklmnopqrstuvwxyz_",
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ",
	"01234567890",
	" \t\n\r",
	"+-=!@$%^&*/:<>.,|~;?",
	"\\",
	"(", ")", "'"
};

// Finite automaton used for lexical analysis

STATE fsm[VALIDCHNO][STATENO] = {
//	 S_VAR  S_ID   S_NUM  S_OP   S_LPA  S_RPA  S_LAM, S_INI  S_QUO
	{S_VAR, S_ID , S_VAR, S_VAR, S_VAR, S_VAR, S_VAR, S_VAR, S_QUO },	// lowercase
	{S_VAR, S_ID , S_ID , S_ID,  S_ID , S_ID , S_ID,  S_ID , S_QUO },	// uppercase
	{S_VAR, S_ID , S_NUM, S_NUM, S_NUM, S_NUM, S_NUM, S_NUM, S_QUO },	// num
	{S_INI, S_INI, S_INI, S_INI, S_INI, S_INI, S_INI, S_INI, S_QUO },	// space
	{S_OP , S_OP , S_OP , S_OP,  S_OP , S_OP , S_OP,  S_OP , S_QUO },	// operator
	{S_LAM, S_LAM, S_LAM, S_LAM, S_LAM, S_LAM, S_NEW, S_LAM, S_QUO }, // \ or greek lambda
	{S_LPA, S_LPA, S_LPA, S_LPA, S_NEW, S_LPA, S_LPA, S_LPA, S_QUO },	// (
	{S_RPA, S_RPA, S_RPA, S_RPA, S_RPA, S_NEW, S_RPA, S_RPA, S_QUO },	// )
	{S_QUO, S_QUO, S_QUO, S_QUO, S_QUO, S_QUO, S_QUO, S_QUO, S_ID  }	// '
};

// functions for rule processing
void procRule0(SYMB_INFO *symb);
void procRule1(SYMB_INFO *symb);
void procRule2(SYMB_INFO *symb);
void procRule3(SYMB_INFO *symb);
void procRule4(SYMB_INFO *symb);
void procRule5(SYMB_INFO *symb);
void procRule6(SYMB_INFO *symb);
void procRule7(SYMB_INFO *symb);
void procRule8(SYMB_INFO *symb);
void procRule9(SYMB_INFO *symb);
void procRule11(SYMB_INFO *symb);
void procRule13(SYMB_INFO *symb);
void doNothing(SYMB_INFO *symb);

// grammar rules
// For each rule we store the number of RHS symbols, the RHS symbols themselves, and the
// function that will be called to process the rule each time it appears in the input.

GRAM_RULE grammar[RULENO] = {
	{3, {TK_ID,		TK_EQ,	TK_TERM							}, procRule0},	// CMD -> id = T
	{2, {TK_QM,		TK_TERM										}, procRule1},	// CMD -> ? T
	{2, {TK_VAR,	TK_TERM2										},	procRule2},	// T -> v T'
	{2, {TK_NUM,	TK_TERM2										}, procRule3},	// T -> num T'
	{2, {TK_ID,		TK_TERM2										}, procRule4},	// T -> id T'
	{4, {TK_LPAR,	TK_TERM,	TK_RPAR,	TK_TERM2				},	procRule5},	// T -> ( T ) T'
	{5, {TK_LAMBDA,TK_VAR,	TK_DOT,	TK_TERM,	TK_TERM2	},	procRule6},	// T -> \ v . T T'
	{3, {TK_OPER,	TK_TERM,	TK_TERM2							},	procRule7},	// T' -> OPER T T'
	{0, {																},	doNothing},	// T' -> e
	{2, {TK_CMD,	TK_LIST2										}, procRule9},	// L -> CMD L'
	{0, {																},	doNothing},	// L -> e
	{2, {TK_SEM,	TK_LIST										}, procRule11},// L' -> ; L
	{0, {																},	doNothing},	// L' -> e
	{1, {TK_OP														}, procRule13},// OPER -> op
	{0, {																},	doNothing}	// OPER -> e
};

// LL(1) parsing table
// For each non-terminal symbol and each input symbol we store the rule that should
// be applied. -1 means syntax error.

int LL1[NONTRMNO][TRMNO] = {
//	 VAR	ID		NUM	OP		LPAR	RPAR	LAMDA	DOT	EQ		SEM	QM		EOF
	{-1,	0,		-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	1,		-1	},		//CMD
	{2,	4,		3,		-1,	5,		-1,	6,		-1,	-1,	-1,	-1,	-1	},		//TERM
	{7,	7,		7,		7,		7,		8,		7,		-1,	-1,	8,		-1,	8	},		//TERM2
	{-1,	9,		-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	9,		10	},		//LIST
	{-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	11,	-1,	12	},		//LIST2
	{14,	14,	14,	13,	14,	-1,	14,	-1,	-1,	-1,	-1,	-1 }		//OPER
};


#define OPERNO 5
TOKEN_TYPE selectOper(char *name) {
	struct {
		char *name;
		TOKEN_TYPE token;
	} opers[OPERNO] = {
		{"->",	TK_DOT},
		{".",		TK_DOT},
		{"=",		TK_EQ},
		{";",		TK_SEM},
		{"?",		TK_QM}
	};
	int i;

	for(i = 0; i < OPERNO; i++)
		if(strcmp(name, opers[i].name) == 0)
			return opers[i].token;

	return TK_OP;
}

// ----------- Rule processing functions ----------------

// newAppl
//
// The op term can be either NULL (in which case s1 is returned) or it can contain
// an appliation with only its right branch filled. In this case s1 will be inserted
// as the left branch, unless due to precedence op needs to "go inside" s2.

TERM *newAppl(TERM *s1, TERM *op) {
	TERM *s;

	if(!op)
		s = s1;
	else {
		TERM *s2 = op->rterm;
		char breakOp = 0;

		// If s2 is an application, then op might need to "go inside" the application.
		// Eg. if op = * and s2 = (a + b) then instead of s1 * (a + b) we need to
		// get (s1 * a) + b, that is * should go inside +.
		// This happens in the following 2 cases:
		// 	- op has lower precedence than s2
		// 	- they have the same precedence and op is _not_ right-associative, hence it
		// 	  _cannot_ have s2 on its right. In this case:
		// 		* if s2 is left-associative (so it can have op on its left) then op goes inside
		// 		* otherwise there is an ambiguity and a warning is printed

		if(s2->type == TM_APPL && op->preced <= s2->preced) {
			if(op->preced < s2->preced || (op->assoc != ASS_RIGHT && s2->assoc == ASS_LEFT))
				breakOp = 1;
			else if(op->assoc != ASS_RIGHT)
				fprintf(stderr, "Warning: Precedence ambiguity between operators '%s' and '%s'. Use brackets.\n",
					(op->name ? op->name : "APPL"), (s2->name ? s2->name : "APPL"));
		}

		if(breakOp) {
			s = s2;
			op->rterm = s2->lterm;
			s2->lterm = newAppl(s1, op);
		} else {
			op->lterm = s1;
			s = op;
		}
	}

	return s;
}

// removeChar
//
// Deletes all occurrences of c from s and returns s

char *removeChar(char *s, char c) {
	int n = 0;
	char *sp = s;

	do {
		if(*sp == c)
			n++;
		else
			*(sp-n) = *sp;
	} while (*sp++ != '\0');

	return s;
}


// CMD -> id = T 
void procRule0(SYMB_INFO *symb) {
	//COMMAND *c = malloc(sizeof(COMMAND));

	//c->type = CM_DECL;
	//c->term = $(2);
	//c->id = strdup(removeChar($(0), '\''));

	//$$ = c;

	termRemoveOper($(2));
	termSetClosedFlag($(2));

	if( ((TERM*)$(2))->closed )
		termAddDecl(strdup(removeChar($(0), '\'')), $(2));
	else
		fprintf(stderr, "Error: alias %s is not a closed term and won't be registered\n", (char*)$(0));

	$$ = NULL;
}

// CMD -> ? T
void procRule1(SYMB_INFO *symb) {
	//COMMAND *c = malloc(sizeof(COMMAND));

	//c->type = CM_QUEST;
	//c->term = $(1);
	//c->id = NULL;

	//$$ = c;

	execTerm($(1));
	$$ = NULL;
}

// T -> v T'
void procRule2(SYMB_INFO *symb) {
	TERM *s = termNew();
	s->type = TM_VAR;
	s->name = str_intern($(0));

	$$ = newAppl(s, $(1));
}

// T -> num T'
void procRule3(SYMB_INFO *symb) {
	int num = 0;

	if(strlen($(0)) > 4)
		fprintf(stderr, "Error: integers must be in the range 0-9999.\n");
	else
		num = atoi($(0));

	$$ = newAppl(termChurchNum(num), $(1));
}

#include "str_intern.h"

// T -> id T'
void procRule4(SYMB_INFO *symb) {
	TERM *s = termNew();
	s->type = TM_ALIAS;
	s->name = str_intern(removeChar($(0), '\''));

	$$ = newAppl(s, $(1));
}

// T -> ( T ) T'
void procRule5(SYMB_INFO *symb) {
	((TERM*)$(1))->preced = 0;
	$$ = newAppl($(1), $(3));
}

// T -> \ v . T T'
void procRule6(SYMB_INFO *symb) {
	TERM *s = termNew(),
		  *v = termNew();

	v->type = TM_VAR;
	v->name = str_intern($(1));

	s->type = TM_ABSTR;
	s->lterm = v;
	s->rterm = newAppl($(3), $(4));

	$$ = s;
}

// T' -> OPER T T'
void procRule7(SYMB_INFO *symb) {
	TERM *t = termNew();
	OPER *op = NULL;

	t->type = TM_APPL;
	t->name = str_intern($(0));
	t->rterm = newAppl($(1), $(2));

	// If an operator exists that we get the precedence/associativity from the operator,
	// otherwise the application has 100 yfx
	if($(0)) op = getOper($(0));
	t->preced = (op ? op->preced : APPL_PRECED);
	t->assoc = (op ? op->assoc : APPL_ASSOC);

	$$ = t;
}

// L -> CMD L'
void procRule9(SYMB_INFO *symb) {
	//((COMMAND*)$(0))->next = $(1);
	//$$ = $(0);

	$$ = NULL;
}

// L' -> ; L
void procRule11(SYMB_INFO *symb) {
	//$$ = $(1);
	$$ = NULL;
}

// OPER -> op
void procRule13(SYMB_INFO *symb) {
	$$ = strdup($(0));
}

// Simply sets $$ to NULL
void doNothing(SYMB_INFO *symb) {
	$$ = NULL;
}


