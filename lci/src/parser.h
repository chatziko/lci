/* Declarations for parser.c

	Copyright (C) 2003 Kostas Hatzikokolakis
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

#define PAR_OK			0
#define PAR_ERROR		-1


// Typos eisodoy toy lektikoy analyth
typedef enum { SC_BUFFER, SC_FILE } INPUT_TYPE;


//plhrofories gia ena token
typedef struct {
	TOKEN_TYPE type;
	char *value;
} TOKEN;

//plhrofories gia ena symbolo ths grammatikhs
typedef struct tag_symb_info {
	char isTerminal;
	TOKEN_TYPE type;
	int ruleNo;
	struct tag_symb_info *chl[MAXRULELEN];
	void *value;
} SYMB_INFO;

//plhrofories enos kanona ths grammatikhs
typedef struct {
	int rsNo;								//ari8mos symbolwn sto de3i melos
	TOKEN_TYPE rs[MAXRULELEN];			//pinakas me symbola tou de3iou melous
	void(*func)(SYMB_INFO*);			//synarthsh epe3ergasia tou kanona
} GRAM_RULE;


// functions provided by parser

int getToken(TOKEN *curToken);
int parse(void **progTree, int uGrammar);


// Variables available outside parser.c
extern void *scInput;
extern INPUT_TYPE scInputType;
extern int scLineNo;

//Pinakes poy ka8orizoun to fsm kai thn grammatikh.
//Prepei na orizontai se kapoio .c arxeio
extern char *validChars[VALIDCHNO];
extern STATE fsm[VALIDCHNO][STATENO];

extern GRAM_RULE grammar[RULENO];
extern int LL1[NONTRMNO][TRMNO];


#endif


