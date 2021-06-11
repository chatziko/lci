// vim:noet:ts=3

/* Functions to manipulate the list of declarations

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "decllist.h"
#include "termproc.h"
#include "parser.h"


DECL *declList = NULL;


// termAddDecl
//
// Adds the term to the declaration list with the given (interned) id.
// If a term with this id already exists it is replaced.

void termAddDecl(char *id, TERM *term) {
	DECL *decl;

	// declared term must be closed
	assert(term->closed);

	// if a declaration with this id exists, replace it
	if((decl = getDecl(id))) {
		//free declaration memory
		termFree(decl->term);

	} else {
		// if declaration not found, create a new one
		decl = malloc(sizeof(DECL));
		decl->aliases.next = NULL;
		decl->next = declList;
		declList = decl;
	}

	decl->id = id;
	decl->term = term;
	buildAliasList(decl);
}

// getDecl
//
// Returns a record corresponding to the declaration with
// the given (interned) id, or NULL if there is no such declaration.

DECL *getDecl(char *id) {
	DECL *decl;

	for(decl = declList; decl; decl = decl->next)
		if(decl->id == id)
			return decl;

	return NULL;
}

// termFromDecl
//
// Returns a clone of the term stored with the given id

TERM *termFromDecl(char *id) {
	DECL *decl = getDecl(id);

	return decl
		? termClone(decl->term)
		: NULL;
}

// buildAliasesList
//
// Deletes the old alias list and creates a new one

void buildAliasList(DECL *d) {
	IDLIST *idl, *tmp;
 
	//free aliases list
	for(idl = d->aliases.next; idl; idl = tmp) {
		tmp = idl->next;
		free(idl);
	}

	d->aliases.next = NULL;
	findAliases(d->term, &d->aliases);

	//print alias list
	//printf("%s: ", d->id);
	//for(tmp = d->aliases.next; tmp; tmp = tmp->next)
		//printf("%s, ", tmp->id);
	//printf("\n");
}

// searchAliasList
//
// Returns 1 if id exists in list, otherwise 0

int searchAliasList(IDLIST *list, char *id) {
	for(list = list->next; list; list = list->next)
		if(list->id == id)
			return 1;

	return 0;
}

// findAliases
//
// Finds all aliases used by term t and adds them to 'list'

void findAliases(TERM *t, IDLIST *list) {
	IDLIST *tmp;

	switch(t->type) {
	 case TM_VAR:
		return;

	 case TM_ALIAS:
		if(!searchAliasList(list, t->name)) {
			tmp = malloc(sizeof(IDLIST));
			tmp->id = t->name;
			tmp->next = list->next;
			list->next = tmp;
		}
		return;

	 case TM_ABSTR:
		findAliases(t->rterm, list);
		return;

	 case TM_APPL:
		findAliases(t->lterm, list);
		findAliases(t->rterm, list);
		return;
	}
}

void printCycle(DECL *start, DECL *end) {
	DECL *curNode;
	for(curNode = end; curNode != start; curNode = curNode->prev)
		printf("%s, ", curNode->id);
	printf("%s\n", start->id);
}

int findCycle() {
	DECL *curNode;
	CYCLE bestCycle, newCycle;

	bestCycle.size = 0;

	// initialize DFS
	for(curNode = declList; curNode; curNode = curNode->next)
		curNode->flag = 0;

	// DFS might need to be executed multiple times if the graph is not connected
	for(curNode = declList; curNode; curNode = curNode->next)
		if(curNode->flag == 0) {
			newCycle = dfs(curNode);
			if(newCycle.size > bestCycle.size)
				bestCycle = newCycle;
		}

	// if a cycle was found, remove it
	if(bestCycle.size > 0) {
		//printf("best cycle size: %d\n", bestCycle.size);
		//printCycle(bestCycle.start, bestCycle.end);
		removeCycle(bestCycle);
		return 1;
	}

	return 0;
}

// dfs
//
// Depth first search for finding cycles. Finds and returns a maximal
// cycle, i.e. one not contained in some other cycle.

CYCLE dfs(DECL *curNode) {
	CYCLE bestCycle, newCycle;
	DECL *newNode;
	IDLIST *idl;
	int curSize;

	bestCycle.size = 0;
	bestCycle.start = bestCycle.end = NULL;
	curNode->flag = 1;

	// process neighborhoods
	for(idl = curNode->aliases.next; idl; idl = idl->next) {
		// if the alias is not defined, ignore it
		if(!(newNode = getDecl(idl->id)))
			continue;

		switch(newNode->flag) {
		 case 0:
			 newNode->prev = curNode;
			 newCycle = dfs(newNode);

			 if(newCycle.size > bestCycle.size)
				 bestCycle = newCycle;
			 break;

		 case 1:
			if((curSize = getCycleSize(newNode, curNode)) > bestCycle.size) {
				bestCycle.end = curNode;
				bestCycle.start = newNode;
				bestCycle.size = curSize;
			}
			break;
		}
	}
	curNode->flag = 2;

	return bestCycle;
}

// getCycleSize
//
// Returns the size of a cycle

int getCycleSize(DECL *start, DECL *end) {
	DECL *cur;
	int size = 1;

	for(cur = end; cur != start; cur = cur->prev)
		size++;

	return size;
}

// removeCycle
//
// Removes recursion in the aliases of cycle C using a fixed point combinator
// Y (must be defined in .lcirc).
//
// If there is more than one alias, they are first merged in a tuple
// and their occurrences are replaced by Index calls.
//
// TODO
// If an alias does not call itself then its replacement in the body
// of the other aliases could eliminate the need for a tuple.

void removeCycle(CYCLE c) {
	DECL *d, *decl;
	TERM *t, *newTerm, *tmpTerm;
	char buffer[500],
		  *newId,		// interned newId
		  *tmpId;
	int i;

	// if there is more than one alias in the cycle, merge them in a tuple
	if(c.size > 1) {
		//construct new id
		char newId_raw[50];
		newId_raw[0] = '\0';
		for(i = 0, d = c.end; i < c.size; i++, d = d->prev) {
			if(i > 0) strcat(newId_raw, "_");
			strcat(newId_raw, d->id);
		}
		newId = str_intern(newId_raw);

		// construct tupled function
		strcpy(buffer, "\\y.y ");
		for(i = 0, d = c.end; i < c.size; i++, d = d->prev) {
			strcat(buffer, d->id);
			strcat(buffer, " ");
		}

		// create term using parser and insert it into the list
		scInputType = SC_BUFFER;
		scInput = buffer;
		getToken(NULL);
		parse((void**)&t, TK_TERM);

		termSetClosedFlag(t);				// mark sub-terms as closed
		termAddDecl(newId, t);

		// replace aliases with their corresponding terms
		termRemoveAliases(t, NULL);

		// Aliases contained in the cycle have been merged in a tuple.
		// So their appearances are replaced by Index calls
		for(i = 0, d = c.end; i < c.size; i++) {
			tmpId = d->id;
			d = d->prev;

			tmpTerm = getIndexTerm(c.size, i, newId);
			termSetClosedFlag(tmpTerm);
			termAddDecl(tmpId, tmpTerm);

			// replace this specific alias with its definition in the whole program
			for(decl = declList; decl; decl = decl->next)
				termRemoveAliases(decl->term, tmpId);
		}
	} else {
		t = c.start->term;
		newId = c.start->id;
	}

	// To remove recursion, appearances of the alias within its body are replaced with the
	// variable _me and we add a fixed point combinator.
	// Hence the term A=N becomes A=� \_me.N[A:=_me]
	termAlias2Var(t, newId, str_intern("_me"));						// Change alias to _me

	newTerm = termNew();											// Application of Y to the term
	newTerm->type = TM_APPL;
	newTerm->name = NULL;

	newTerm->lterm = termNew();								// Y
	newTerm->lterm->type = TM_ALIAS;
	newTerm->lterm->name = str_intern("Y");

	newTerm->rterm = termNew();								// Remove \_me.
	tmpTerm = newTerm->rterm;
	tmpTerm->type = TM_ABSTR;
	tmpTerm->name = NULL;
	tmpTerm->rterm = t;

	tmpTerm->lterm = termNew();								// _me variable
	tmpTerm->lterm->type = TM_VAR;
	tmpTerm->lterm->name = str_intern("_me");

	// Change declaration
	// Note: can't use termAddDecl because it frees the old term (used in the new one)
	termSetClosedFlag(newTerm);
	decl = getDecl(newId);
	decl->term = newTerm;

	// reconstruct all alias lists
	for(decl = declList; decl; decl = decl->next)
		buildAliasList(decl);	
}

// getIndexTerm
//
// Returns the following lambda-term:
//   TUPLE \x1.\x2. ... \xvarno.x<n>
// which chooses the n-th element of a varno-tuple

TERM *getIndexTerm(int varno, int n, char *tuple) {
	TERM *t;
	char buffer[500];
	int i;

	//build string
	sprintf(buffer, "%s (", tuple);
	for(i = 0; i < varno; i++)
		sprintf(buffer + strlen(buffer), "\\x%d.", i);

	sprintf(buffer + strlen(buffer), "x%d)", n);

	//create term
	scInputType = SC_BUFFER;
	scInput = buffer;
	getToken(NULL);
	parse((void**)&t, TK_TERM);

	return t;
}

// printDeclList
//
// Prints the entire declaration list

void printDeclList(char *id) {
	DECL *d;

	if(id)
		if(!(d = getDecl(id)))
			printf("Error: alias %s not found.\n", id);
		else {
			printf("%s = ", id);
			termPrint(d->term, 1);
			printf("\n");
		}
	else
		for(d = declList; d; d = d->next) {
			printf("%s = ", d->id);
			termPrint(d->term, 1);
			printf("\n");
		}
}



// ------- Manage operators --------

OPER *operList = NULL;


OPER *getOper(char *id) {
	OPER *op;

	for(op = operList; op; op = op->next)
		if(op->id == id)
			return op;

	return NULL;
}

// addOper
//
// Adds an operator declaration. Replace if already exists.

void addOper(char *id, int preced, ASS_TYPE assoc) {
	OPER *op;

	// if id is already registered we replace
	if(!(op = getOper(id))) {
		// not found, create new
		op = malloc(sizeof(OPER));
		op->next = operList;
		operList = op;
	}

	op->id = id;
	op->preced = preced;
	op->assoc = assoc;
}


