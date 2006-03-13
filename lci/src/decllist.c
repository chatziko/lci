/* Functions to manipulate the list of declarations

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
#include <assert.h>

#include "decllist.h"
#include "termproc.h"
#include "parser.h"


DECL *declList = NULL;


// termAddDecl
//
// Adds the term to the declaration list with the given id.
// If a term with this id already exists it is replaced.

void termAddDecl(char *id, TERM *term) {
	DECL *decl;

	// declared term must be closed
	assert(term->closed);

	// if a declaration with this id exists, replace it
	if((decl = getDecl(id))) {
		//free declaration memory
		free(decl->id);
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
// the given id, or NULL if there is no such declaration.

DECL *getDecl(char *id) {
	DECL *decl;

	for(decl = declList; decl; decl = decl->next)
		if(strcmp(decl->id, id) == 0)
			return decl;

	return NULL;
}

// termFromDecl
//
// Επιστρέφει ένα νέο όρο ο οποίος είναι αντίγραφο του αποθηκευμένου με αυτό το id

TERM *termFromDecl(char *id) {
	DECL *decl = getDecl(id);

	return decl
		? termClone(decl->term)
		: NULL;
}

// buildAliasesList
//
// Diagrafei thn palia lista me ta aliases kai dhmiourgei mia nea

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
// Επιστρέφει 1 αν το id υπάρχει στη λίστα, διαφορετικά 0

int searchAliasList(IDLIST *list, char *id) {
	for(list = list->next; list; list = list->next)
		if(strcmp(list->id, id) == 0)
			return 1;

	return 0;
}

// findAliases
//
// Vriskei ola ta aliases poy xrhsimopoioyntai apo ton
// oro t kai ta pros8etei sthn lista list

void findAliases(TERM *t, IDLIST *list) {
	IDLIST *tmp;

	switch(t->type) {
	 case TM_VAR:
		return;

	 case TM_ALIAS:
		if(!searchAliasList(list, t->name)) {
			tmp = malloc(sizeof(IDLIST));
			strcpy(tmp->id, t->name);
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

	//arxikopoihsh ths DFS
	for(curNode = declList; curNode; curNode = curNode->next)
		curNode->flag = 0;

	//h dfs mporei na xreiastei na ektelestei polles fores an o grafos einai mh synektikos
	for(curNode = declList; curNode; curNode = curNode->next)
		if(curNode->flag == 0) {
			newCycle = dfs(curNode);
			if(newCycle.size > bestCycle.size)
				bestCycle = newCycle;
		}

	//an bre8hke kapoios kyklos ton afairoume
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
// Depth first search για αναζήτηση κύκλων. Εύρεση και επιστροφή του μέγιστου
// δυνατού κύκλου, δηλαδή αυτού που δεν περιέχεται σε κάποιον άλλο

CYCLE dfs(DECL *curNode) {
	CYCLE bestCycle, newCycle;
	DECL *newNode;
	IDLIST *idl;
	int curSize;

	bestCycle.size = 0;
	bestCycle.start = bestCycle.end = NULL;
	curNode->flag = 1;

	//epe3ergasia twn geitonwn
	for(idl = curNode->aliases.next; idl; idl = idl->next) {
		//Αν το alias δεν είναι καταχωρημένο το αγνοούμε
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
// Επιστρέφει το μέγεθος ενός κύκλου

int getCycleSize(DECL *start, DECL *end) {
	DECL *cur;
	int size = 1;

	for(cur = end; cur != start; cur = cur->prev)
		size++;

	return size;
}

// removeCycle
//
// Αφαιρεί την αναδρομή στα aliases του κύκλου C χρησιμοποιώντας έναν
// fixed point combinator Y (πρέπει να έχει οριστεί στο .lcirc)
//
// Αν υπάρχουν περισσότερα από ένα aliases ενώνονται πρώτα σε ένα tuple
// και οι εμφανίσεις τους αντικαθιστώνται με κλησεις Index
//
// TODO
// Αν ένα alias δεν καλεί τον εαυτό του τότε η αντικατάστασή του στο σώμα
// των άλλων aliases εξαλείφει την ανάγκη για tuple.

void removeCycle(CYCLE c) {
	DECL *d, *decl;
	TERM *t, *newTerm, *tmpTerm;
	char buffer[500],
		  newId[50],
		  *tmpId;
	int i;

	//Αν υπάρχουν περισσότερα από ένα aliases στον κύκλο τότε ενώνοντα σε ένα tuple
	if(c.size > 1) {
		//construct new id
		newId[0] = '\0';
		for(i = 0, d = c.end; i < c.size; i++, d = d->prev) {
			if(i > 0) strcat(newId, "_");
			strcat(newId, d->id);
		}

		//construct tupled function
		strcpy(buffer, "\\y.y ");
		for(i = 0, d = c.end; i < c.size; i++, d = d->prev) {
			strcat(buffer, d->id);
			strcat(buffer, " ");
		}

		//create term using parser and insert it into the list
		scInputType = SC_BUFFER;
		scInput = buffer;
		getToken(NULL);
		parse((void**)&t, TK_TERM);

		termSetClosedFlag(t);				// mark sub-terms as closed
		termAddDecl(strdup(newId), t);

		//Ta aliases antikatastash twn aliases me tous orismous toys
		termRemoveAliases(t, NULL);

		// Οσα aliases περιέχονται στον κύκλο έχουν ενσωματωθεί στο tuple.
		// Ετσι οι ορισμοί τους αντικαθιστώνται από όρους Index
		for(i = 0, d = c.end; i < c.size; i++) {
			tmpId = strdup(d->id);
			d = d->prev;

			tmpTerm = getIndexTerm(c.size, i, newId);
			termSetClosedFlag(tmpTerm);
			termAddDecl(tmpId, tmpTerm);

			// Αντικατάσταση του συγκεκριμένου ALIAS με το νέο ορισμό σε όλο το πρόγραμμα
			for(decl = declList; decl; decl = decl->next)
				termRemoveAliases(decl->term, tmpId);
		}
	} else {
		t = c.start->term;
		strcpy(newId, c.start->id);
	}

	// Για να αφαιρεθεί η αναδρομή οι εμφανίσεις του alias μέσα στον εαυτό του αντικαθιστώνται
	// με τη μεταβλητή _me και προστίθεται ένας fixed point combinator
	// Ετσι ο όρος A=N θα γίνει A=Υ \_me.N[A:=_me]
	termAlias2Var(t, newId, "_me");							//Allagh toy alias se _me

	newTerm = termNew();											//Efarmogh toy Y ston oro
	newTerm->type = TM_APPL;
	newTerm->name = NULL;

	newTerm->lterm = termNew();								//Y
	newTerm->lterm->type = TM_ALIAS;
	newTerm->lterm->name = strdup("Y");

	newTerm->rterm = termNew();								//Afairesh \_me.
	tmpTerm = newTerm->rterm;
	tmpTerm->type = TM_ABSTR;
	tmpTerm->name = NULL;
	tmpTerm->rterm = t;

	tmpTerm->lterm = termNew();								//Metablhth _me
	tmpTerm->lterm->type = TM_VAR;
	tmpTerm->lterm->name = strdup("_me");

	// Change declaration
	// Note: can't use termAddDecl because it frees the old term (used in the new one)
	termSetClosedFlag(newTerm);
	decl = getDecl(newId);
	decl->term = newTerm;

	//Επανακατασκευή όλων των λιστών με τα aliases
	for(decl = declList; decl; decl = decl->next)
		buildAliasList(decl);	
}

// getIndexTerm
//
// Επιστρέφει έναν λ-όρο ο οποίος διαλέξει το n-ατο από το tuple των varno
// στοιχείων. Ο όρος αυτός είναι ο ακόλουθος
// TUPLE \x1.\x2. ... \xvarno.xn

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
// Τυπώνει όλη τη λίστα με τους ορισμούς

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



// ------- Diaxeirish operators --------

OPER *operList = NULL;


OPER *getOper(char *id) {
	OPER *op;

	for(op = operList; op; op = op->next)
		if(strcmp(op->id, id) == 0)
			return op;

	return NULL;
}

// addOper
//
// Καταχωρείτην δήλωση ενός operator. Αν υπάρχει ήδη αντικαθίσταται

void addOper(char *id, int preced, ASS_TYPE assoc) {
	OPER *op;

	//an to id exei hdh dhlw8ei kanoume antikatastash
	if((op = getOper(id)))
		free(op->id);
	else {
		//an den bre8hke dhmiourgoume kainourgio
		op = malloc(sizeof(OPER));
		op->next = operList;
		operList = op;
	}

	op->id = id;
	op->preced = preced;
	op->assoc = assoc;
}


