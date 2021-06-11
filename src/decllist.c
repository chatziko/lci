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

#include "ADTMap.h"
#include "decllist.h"
#include "termproc.h"
#include "parser.h"

static Map declarations = NULL;
static Map operators = NULL;


static DECL *getDecl(char *id);

static void buildAliasList(DECL *d);
static void findAliases(TERM *t, Vector aliases);

static CYCLE dfs(DECL *curNode);
static int getCycleSize(DECL *start, DECL *end);
static void removeCycle(CYCLE c);
static TERM *getIndexTerm(int varno, int n, char *tuple);

static int compare_pointers(Pointer a, Pointer b) {
	return a - b;
}

// termAddDecl
//
// Adds the term to the declaration list with the given (interned) id.
// If a term with this id already exists it is replaced.

void termAddDecl(char *id, TERM *term) {
	// declared term must be closed
	assert(term->closed);

	if(declarations == NULL) {
		// strings are interned, so we can just compare them as pointers
		declarations = map_create(compare_pointers, NULL, free);
		map_set_hash_function(declarations, hash_pointer);
	}

	// if a declaration with this id exists, replace it
	DECL *decl = getDecl(id);

	if(decl != NULL) {
		// free the term, it's going to be replaced
		termFree(decl->term);

	} else {
		// if declaration not found, create a new one
		decl = malloc(sizeof(DECL));
		decl->aliases = NULL;

		map_insert(declarations, id, decl);
	}

	decl->id = id;
	decl->term = term;
	buildAliasList(decl);
}

// getDecl
//
// Returns a record corresponding to the declaration with
// the given (interned) id, or NULL if there is no such declaration.

static DECL *getDecl(char *id) {
	return declarations  != NULL
		? map_find(declarations, id)
		: NULL;
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

static void buildAliasList(DECL *d) {
	//free aliases list
	if(d->aliases != NULL)
		vector_destroy(d->aliases);

	d->aliases = vector_create(0, NULL);
	findAliases(d->term, d->aliases);

	//print alias list
	//printf("%s: ", d->id);
	//for(tmp = d->aliases.next; tmp; tmp = tmp->next)
		//printf("%s, ", tmp->id);
	//printf("\n");
}

// findAliases
//
// Finds all aliases used by term t and adds them to 'aliases'

static void findAliases(TERM *t, Vector aliases) {
	switch(t->type) {
	 case TM_VAR:
		return;

	 case TM_ALIAS:
		if(!vector_find(aliases, t->name, compare_pointers))
			vector_insert_last(aliases, t->name);
		return;

	 case TM_ABSTR:
		findAliases(t->rterm, aliases);
		return;

	 case TM_APPL:
		findAliases(t->lterm, aliases);
		findAliases(t->rterm, aliases);
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
	CYCLE bestCycle;

	bestCycle.size = 0;

	// initialize DFS
	for(MapNode node = map_first(declarations); node != MAP_EOF; node = map_next(declarations, node)) {
		DECL* decl = map_node_value(declarations, node);
		decl->flag = 0;
	}

	// DFS might need to be executed multiple times if the graph is not connected
	for(MapNode node = map_first(declarations); node != MAP_EOF; node = map_next(declarations, node)) {
		DECL* decl = map_node_value(declarations, node);
		if(decl->flag == 0) {
			CYCLE newCycle = dfs(decl);
			if(newCycle.size > bestCycle.size)
				bestCycle = newCycle;
		}
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

static CYCLE dfs(DECL *curNode) {
	CYCLE bestCycle, newCycle;
	DECL *newNode;
	int curSize;

	bestCycle.size = 0;
	bestCycle.start = bestCycle.end = NULL;
	curNode->flag = 1;

	// process neighborhoods
	for(int i = 0; i < vector_size(curNode->aliases); i++) {
		// if the alias is not defined, ignore it
		if(!(newNode = getDecl(vector_get_at(curNode->aliases, i))))
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

static int getCycleSize(DECL *start, DECL *end) {
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

static void removeCycle(CYCLE c) {
	DECL *d, *decl;
	TERM *t, *newTerm, *tmpTerm;
	char *newId;		// interned newId
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
		char buffer[500];
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
			char* tmpId = d->id;
			d = d->prev;

			tmpTerm = getIndexTerm(c.size, i, newId);
			termSetClosedFlag(tmpTerm);
			termAddDecl(tmpId, tmpTerm);

			// replace this specific alias with its definition in the whole program
			for(MapNode node = map_first(declarations); node != MAP_EOF; node = map_next(declarations, node)) {
				DECL* decl = map_node_value(declarations, node);
				termRemoveAliases(decl->term, tmpId);
			}
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
	for(MapNode node = map_first(declarations); node != MAP_EOF; node = map_next(declarations, node)) {
		DECL* decl = map_node_value(declarations, node);
		buildAliasList(decl);
	}
}

// getIndexTerm
//
// Returns the following lambda-term:
//   TUPLE \x1.\x2. ... \xvarno.x<n>
// which chooses the n-th element of a varno-tuple

static TERM *getIndexTerm(int varno, int n, char *tuple) {
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

	if(id) {
		DECL *d = getDecl(id);
		if(d == NULL) {
			printf("Error: alias %s not found.\n", id);
		} else {
			printf("%s = ", id);
			termPrint(d->term, 1);
			printf("\n");
		}
	} else {
		for(MapNode node = map_first(declarations); node != MAP_EOF; node = map_next(declarations, node)) {
			DECL* d = map_node_value(declarations, node);
			printf("%s = ", d->id);
			termPrint(d->term, 1);
			printf("\n");
		}
	}
}



// ------- Manage operators --------

OPER *getOper(char *id) {
	return operators != NULL
		? map_find(operators, id)
		: NULL;
}

// addOper
//
// Adds an operator declaration. Replace if already exists.

void addOper(char *id, int preced, ASS_TYPE assoc) {

	if(operators == NULL) {
		// strings are interned, so we can just compare them as pointers
		operators = map_create(compare_pointers, NULL, free);
		map_set_hash_function(operators, hash_pointer);
	}

	// if id is already registered we replace
	OPER *op = getOper(id);
	if(op == NULL) {
		// not found, create new
		op = malloc(sizeof(OPER));
		op->id = id;
		map_insert(operators, id, op);
	}

	op->preced = preced;
	op->assoc = assoc;
}


