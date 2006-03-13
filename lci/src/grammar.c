/* Definition of the finite state machine and the grammar rules
	that are used by the parser

	Copyright (C) 2006 Kostas Chatzikokolakis
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


// Epitreptoi xarakthres ths glwssas
// Oi xarakthres poy briskontai se ka8e string anhkoyn sthn idia kathgoria
// Sto peperasmeno aytomato fsm orizontai oi metabaseis gia ka8e kathgoria xarakthrwn

char *validChars[VALIDCHNO] =	{
	"abcdefghijklmnopqrstuvwxyz_",
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ",
	"01234567890",
	" \t\n\r",
	"+-=!@$%^&*/:<>.,|~;?",
	"\\\xEB",
	"(", ")", "'"
};

// Peperasmeno aytomato poy xrhsimopoieitai sthn lektikh analysh

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

//functions gia thn epe3ergasia kanonwn
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

//kanones grammatikhs
//Gia ka8e kanona apo8hkeyetai o ari8mos symbolwn sto de3i meros, ta symbola
//sto de3i meros, kai h synarthsh pou 8a klh8ei gia na epe3ergastei ton kanona
//ka8e fora poy aytos 8a emfanistei sthn eisodo

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

//Pinakas LL(1)
//Gia ka8e mh termatiko symbolo kai gia ka8e symbolo eisodou apo8hkeyoyme ton
//kanona poy prepei na efarmostei. To -1 shmainei syntaktiko la8os.

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

// ----------- Synarthseis epe3ergasias kanonwn ----------------

//Precedence and associativity of applications
#define APPL_PRECED	100
#define APPL_ASSOC	ASS_LEFT

// newAppl
//
// O oros op mporei na einai NULL (opote epistrefetai to s1) h na exei
// mia efarmogh poy exei symplhrwmeno mono to de3i kladi. Sthn periptwsh ayth
// 8a mpei to s1 ws aristero kladi toy op ektos kai an logw proteraiothtas o op
// prepei na "mpei mesa" ston s2

TERM *newAppl(TERM *s1, TERM *op) {
	TERM *s;

	if(!op)
		s = s1;
	else {
		TERM *s2 = op->rterm;
		char breakOp = 0;

		// An o oros s2 einai efarmogh tote yparxei periptwsh o op na "mpei mesa"
		// sthn efarmogh. Px an op = * kai s2 = (a + b) tote anti gia s1 * (a + b)
		// prepei na paroume (s1 * a) + b dhladh o * na mpei mesa sto +
		// Ayto symbainei stis akolou8es 2 periptwseis
		//		- O op exei mikroterh proteraiothta apo ton s2
		//		- Exoun ish proteraiothta kai o op DEN einai de3ia prosetairistikos dhladh
		//		  DEN mporei na exei to s2 sta de3ia toy. An o s2 einai aristera
		//		  prosetairistikos (dhladh mporei na exei ton op sta aristera toy) tote
		//		  o op "mpainei mesa" diaforetika yparxei ambiguity kai typwnetai mhnyma

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
// Διαγράφει όλες τις εμφανίσεις του c από το s και επιστρέφει το s

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
		fprintf(stderr, "Error: alias %s is not a closed term and won't be registered\n", $(0));

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
	s->name = strdup($(0));

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

// T -> id T'
void procRule4(SYMB_INFO *symb) {
	TERM *s = termNew();
	s->type = TM_ALIAS;
	s->name = strdup(removeChar($(0), '\''));

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
	v->name = strdup($(1));

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
	t->name = $(0);
	t->rterm = newAppl($(1), $(2));

	// An yparxei operator pairnoyme thn proter. kai thn prosetair. apo ton telesth
	// Diaforetika h efarmogh exei 100 yfx
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

// Apla 8etei to $$ se NULL
void doNothing(SYMB_INFO *symb) {
	$$ = NULL;
}


