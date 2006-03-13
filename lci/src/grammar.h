/* Declarations for grammar.c

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

#ifndef GRAMMAR_H
#define GRAMMAR_H

#if HAVE_CONFIG_H
#include <config.h>
#endif


// Add __attribute__((packed)) if compiler supports it
#ifdef HAVE_ATTR_PACKED
#define ATTR_PACKED __attribute__((packed))
#else
#define ATTR_PACKED
#endif

//Sta8eres poy epitrepoun th xrhsh twn $$ kai $(n) me ton idio tropo
//me ton yacc stis synarthseis epe3ergasias kanonwn

#define $$		symb->value
#define $(n)	symb->chl[n]->value

//sta8eres poy aforoun thn grammatikh

#define TRMNO			12				//ari8mos termatikwn symbolwn
#define NONTRMNO		6				//ari8mos mh termatikwn symbolwn
#define RULENO			15				//ari8mos kanonwn ths grammatikhs
#define MAXRULELEN	5				//megisto mege8os enos de3i kanona
#define VALIDCHNO		9				//ari8mos kathgoriwn ston pinaka epitreptwn xarakthrwn
#define STATENO 		9				//ari8mos katastasewn toy fsm ths lektikhs analyshs
#define FINALSTATES	7				//Tekiles katastaseis toy aytomatoy

// TOKENS ths grammatikhs

typedef enum {
	TK_VAR = 0, TK_ID, TK_NUM, TK_OP, TK_LPAR, TK_RPAR, TK_LAMBDA,	//terminals
	TK_DOT, TK_EQ, TK_SEM, TK_QM, TK_EOF,
	TK_CMD, TK_TERM, TK_TERM2, TK_LIST, TK_LIST2, TK_OPER				//non-terminals
} TOKEN_TYPE;

//arxiko symbolo ths grammatikhs
#define INIT_SYMB TK_LIST

// Katastaseis toy peperasmenoy aytomatoy ths lektiths analyshs
// Oi prwtes FINALSTATES katastaseis einai oi telikes katastaseis kai
// antistoixoyn se termatika TOKENS, oi ypoloipes xrhsimopoioyntai eswterika
//apo ton lektiko analyth

typedef enum {
	S_VAR = TK_VAR, S_ID, S_NUM, S_OP, S_LPA, S_RPA, S_LAM,
	S_INI, S_QUO,
	S_EOF, S_NEW, S_IGN, S_ERR				//eidika states (den metrane sto STATENO)
} STATE;


// structs gia thn anaparastash enos ë-programmatos se morfh dentroy.
// Kata to parsing me th boh8eia toy syntaktikoy dentroy 8a dhmiourgh8ei
// to dentro toy programmatos to opoio 8a xrhsimopoih8ei gia thn ektelesh

// Note: ATTR_PACKED (#defined __atribute__((packed))) instructs the compiler to
// use 1 byte instead of 4 for the enum
enum term_type_tag { TM_ABSTR, TM_APPL, TM_VAR, TM_ALIAS } ATTR_PACKED;
enum ass_type_tag { ASS_LEFT, ASS_RIGHT, ASS_NONE } ATTR_PACKED;

typedef enum term_type_tag TERM_TYPE;
typedef enum ass_type_tag ASS_TYPE;
typedef enum { CM_QUEST, CM_DECL } COMMAND_TYPE;

// Note: put bigger fields first (lterm, rterm, name) to allow
// the compiler to align the struct without wasting space.
// results in a smaller sizeof(TERM)
//
typedef struct term_tag {
	struct term_tag *lterm;					//aristero kai de3i paidi
	struct term_tag *rterm;					//(gia efarmoges kai afaireseis)
	char *name;									//onoma (gia metablhtes, aliases kai efarmoges me operator)
	TERM_TYPE type;							//metablhth, efarmogh h afairesh
	ASS_TYPE assoc;
	unsigned char preced;
	char closed;
} TERM;

typedef struct command_tag {
	COMMAND_TYPE type;
	char *id;
	TERM *term;
	struct command_tag *next;
} COMMAND;


TOKEN_TYPE selectOper(char *name);


#endif


