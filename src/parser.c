// vim:noet:ts=3

/* Lexical analyzer and parser

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
#include <string.h>
#include <stdlib.h>

#include "parser.h"



void buildParseTree(SYMB_INFO *symb);


void *scInput;
INPUT_TYPE scInputType;
int scLineNo;



// getToken
//
// Reads one token from stdin and stores it in ptok.
// If called with ptok=NULL then initializes the scanner.
//
// Return Value:
//		PAR_OK		Successfully read token
//		PAR_ERROR	Syntax error
// 

int getToken(TOKEN *ptok) {
	static char tokenBuffer[100], *tbuf;
	static STATE prevState;
	static int charNo[255];

	//if getToken is called without ptok then initialize scanner
	if(!ptok) {
		int i;
		char *cpt;

		tbuf = tokenBuffer;
		prevState = S_INI;
		scLineNo = 1;

		//initialize charNums table
		for(i = 0; i < 255; i++)
			charNo[i] = -1;

		for(i = 0; i < VALIDCHNO; i++)
			for(cpt = validChars[i]; *cpt != '\0'; cpt++)
				charNo[(unsigned char)*cpt] = i;

		return PAR_OK;
	}

	ptok->type = -1;

	while(prevState != S_EOF) {
		STATE newState;
		char c;

		//get a character
		if(scInputType == SC_FILE) {
			c = fgetc(scInput);

			if ((c & 0xFF) == 0xCE) {
				char oldc = c;
				c = fgetc(scInput);

				if ((c & 0xFF) == 0xBB) {
					c = '\\';
				} else {
					c = oldc;
					fseek(scInput, -1, SEEK_CUR);
				}
			}
		} else {
			if(!(c = *(char*)scInput++))
				c = EOF;

			if ((c & 0xFF) == 0xCE) {
				c = *(char*)scInput++;
				if ((c & 0xFF) == 0xBB) {
					c = '\\';
				} else {
					c--;
				}
			}
		}
		//printf("%c", c);

		//handle comments
		if(scInputType == SC_FILE && c == '#')
			do {
				c = fgetc(scInput);
			} while(c != '\n' && c != '\r' && c != EOF);

		//count lines
		if(c == '\n') scLineNo++;

		//get new state
		if(c != EOF) {
			if(charNo[(unsigned char)c] == -1)						//invalid character
				return PAR_ERROR;

			if((newState = fsm[charNo[(unsigned char)c]][prevState]) == S_IGN)
				continue;
		} else
			newState = S_EOF;

		//if completed a terminal token then return it
		if(newState != prevState && (prevState < FINALSTATES || prevState == S_INI)) {
			*tbuf = '\0';								//reset buffer
			tbuf = tokenBuffer;

			if(prevState != S_INI) {
				ptok->type = prevState == S_OP
					? selectOper(tbuf)
					: prevState;
				ptok->value = strdup(tbuf);
			}
		}

		//store character
		*tbuf++ = c;
		if(newState != S_NEW)
			prevState = newState;

		//if token completed then return
		if(ptok->type != -1)
			return PAR_OK;
	}

	// Exiting while means that we reached EOF. If the buffer is not empty
	// it means that the last token was not completed.
	if(tokenBuffer[0] != EOF)
		return PAR_ERROR;

	ptok->type = TK_EOF;
	return PAR_OK;
}


// parse
//
// LL(1) parser
// Implements the classic algorithm for LL(1) top-down parsing. Uses the 'grammar' array
// which contains the grammar rules and the 'LL1' parsing table for selecting rules.

int parse(void **progTree, int uGrammar) {
	SYMB_INFO *stack[200],
				 **spt = stack,
				 *curSymb,
				 mainSymb = {0, INIT_SYMB, 0, {}, NULL};
	TOKEN curToken;
	int ruleNo, i;

	if(uGrammar >= 0)
		mainSymb.type = uGrammar;

	// insert main symbol in stack and read first token
	*spt++ = &mainSymb;
	if(getToken(&curToken) != PAR_OK)
		return PAR_ERROR;

	while(spt != stack) {
		curSymb = *--spt;

		if(curSymb->isTerminal) {
			// terminal symbol
			if(curSymb->type == curToken.type) {
				// matches the input symbol
				curSymb->value = curToken.value;					//Apo8hkeysh timhs
				if(getToken(&curToken) != PAR_OK)				//pairnoyme neo symbolo
					return PAR_ERROR;

			} else
				return PAR_ERROR;
		} else {
			// non-terminal symbol
			ruleNo = LL1[curSymb->type - TRMNO][curToken.type];		//eyresh kanona
			if(ruleNo == -1) return PAR_ERROR;								//syntaktiko la8os

			// add all symbols in the rule's RHS to the stack in reverse order. We keep
			// these symbols in the chl array.
			curSymb->ruleNo = ruleNo;
			for(i = grammar[ruleNo].rsNo - 1; i >= 0; i--) {
				SYMB_INFO *tmpSymb = malloc(sizeof(SYMB_INFO));
				curSymb->chl[i] = tmpSymb;
	
				tmpSymb->type = grammar[ruleNo].rs[i];
				tmpSymb->isTerminal = (tmpSymb->type < TRMNO);
				tmpSymb->value = NULL;
				*spt++ = tmpSymb;
			}
		}
	}

	// create parse tree
	buildParseTree(&mainSymb);

	// success
	if(progTree) *progTree = mainSymb.value;
	return PAR_OK;
}


// buildParseTree
//
// Called after scanning the whole input. Starting from the leaves, it calls
// each rule's processing function to build the parse tree.

void buildParseTree(SYMB_INFO *symb) {
	int i;

	// terminal symbol, no processing is needed
	if(symb->isTerminal)
		return;

	// first process all children
	for(i = 0; i < grammar[symb->ruleNo].rsNo; i++)
		buildParseTree(symb->chl[i]);

	// process the rule
	grammar[symb->ruleNo].func(symb);

	// free memory
	for(i = 0; i < grammar[symb->ruleNo].rsNo; i++) {
		if(symb->chl[i]->isTerminal)
			free(symb->chl[i]->value);
		free(symb->chl[i]);
	}
}


