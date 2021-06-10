// vim:noet:ts=3

/* Declarations for parser.c

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

#ifndef PARSER_H
#define PARSER_H

#include "grammar.h"
#include "str_intern.h"

#define PAR_OK			0
#define PAR_ERROR		-1


// lexer's input type
typedef enum { SC_BUFFER, SC_FILE } INPUT_TYPE;


// token info
typedef struct {
	TOKEN_TYPE type;
	char *value;
} TOKEN;

// grammar symbol info
typedef struct tag_symb_info {
	char isTerminal;
	TOKEN_TYPE type;
	int ruleNo;
	struct tag_symb_info *chl[MAXRULELEN];
	void *value;
} SYMB_INFO;

// grammar rule info
typedef struct {
	int rsNo;								// number of symbols in the right-hand side
	TOKEN_TYPE rs[MAXRULELEN];			// array of right-hand side symbols
	void(*func)(SYMB_INFO*);			// rule processing function
} GRAM_RULE;


// functions provided by parser

int getToken(TOKEN *curToken);
int parse(void **progTree, int uGrammar);


// Variables available outside parser.c
extern void *scInput;
extern INPUT_TYPE scInputType;
extern int scLineNo;

// arrays modelling the finite state automaton (fsm) and the grammar
// must be defined in some .c file
extern char *validChars[VALIDCHNO];
extern STATE fsm[VALIDCHNO][STATENO];

extern GRAM_RULE grammar[RULENO];
extern int LL1[NONTRMNO][TRMNO];


#endif


