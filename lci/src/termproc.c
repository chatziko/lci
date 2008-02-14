/* Term manipulation functions

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
#include <assert.h>

#include "kazlib/list.h"

#include "termproc.h"
#include "decllist.h"
#include "parser.h"
#include "run.h"


#define DEFAULT_POOL_SIZE 500
TERM **termPool = NULL;
int termPoolSize = 0;
int termPoolIndex = -1;


// termPrint
//
// Prints a lambda term
// If showPar = 0 then only the required parentheses are printed
// If greeklambda = 1 then a greek lambda character is used instead of "."
// (must have ISO-8859-7 character set to display it correctly)

void termPrint(TERM *t, int isMostRight) {
	char showPar = getOption(OPT_SHOWPAR),
		greekLambda = getOption(OPT_GREEKLAMBDA),
		readable = getOption(OPT_READABLE);
	int num;

	switch(t->type) {
	 case TM_VAR:
	 case TM_ALIAS:
		printf("%s", t->name);
		break;

	 case TM_ABSTR:
		//an einai church numeral typwnoyme ton antistoixo ari8mo
		if(readable && (num = termNumber(t)) != -1)
			printf("%d", num);
		else if(readable && termIsList(t))
			termPrintList(t);
		else {
			if(showPar || !isMostRight) printf("(");

			printf(greekLambda ? "\xEB" : "\\");
			termPrint(t->lterm, 0);
			printf(".");
			termPrint(t->rterm, 1);

			if(showPar || !isMostRight) printf(")");
		}
		break;

	 case TM_APPL:
		if(showPar) printf("(");

		termPrint(t->lterm, 0);

		//if(t->name)
			//printf(" %s ", t->name);
		//else
			printf(" ");

		if(!showPar && t->rterm->type == TM_APPL) printf("(");
		termPrint(t->rterm, isMostRight);
		if(!showPar && t->rterm->type == TM_APPL) printf(")");

		if(showPar) printf(")");
		break;
	}
}

// termFree
//
// Apeley8erwsh ths mnhmhs enos TERM

void termFree(TERM *t) {
	TERM **newPool;
	int i;

	// if NULL do nothing
	if(!t) return;

	termPoolIndex++;

	// if the pool is full then create a new one of twice the size
	if(termPoolIndex >= termPoolSize) {
		termPoolSize = termPoolSize == 0 ? DEFAULT_POOL_SIZE : 2*termPoolSize;
		newPool = malloc(termPoolSize * sizeof(TERM*));

		// copy terms to new pool
		for(i = 0; i < termPoolIndex; i++)
			newPool[i] = termPool[i];

		free(termPool);
		termPool = newPool;
	}

	// put the term in the pool
	termPool[termPoolIndex] = t;
}

void termGC() {
	while(termPoolIndex >= 0)
		free(termNew());

	if(termPoolSize > DEFAULT_POOL_SIZE) {
		free(termPool);
		termPool = NULL;
		termPoolSize = 0;
	}
	termPoolIndex = -1;
}

TERM *termNew() {
	TERM *t;

	if(termPoolIndex >= 0) {
		t = termPool[termPoolIndex];
		termPoolIndex--;

		switch(t->type) {
		 case TM_VAR:
		 case TM_ALIAS:
			free(t->name);
			break;
	
		 case TM_APPL:
			free(t->name);
		 case TM_ABSTR:
			termFree(t->lterm);
			termFree(t->rterm);
			break;
		}
	} else
		t = malloc(sizeof(TERM));

	return t;
}

// termClone
//
// Dhmioyrgei kai epistrefei ena entigrafo enos oroy
// (kai olwn twn orwn katw apo ayton)

TERM *termClone(TERM *t) {
	TERM *newTerm;

	newTerm = termNew();
	newTerm->type = t->type;
	newTerm->preced = t->preced;
	newTerm->closed = t->closed;
	//newTerm->assoc = t->assoc;			// assoc used only in parsing, no need to copy it

	if(t->type == TM_VAR || t->type == TM_ALIAS)
		newTerm->name = strdup(t->name);
	else {														//TM_ABRST or TM_APPL
		newTerm->name = NULL;
		newTerm->lterm = termClone(t->lterm);
		newTerm->rterm = termClone(t->rterm);
	}

	return newTerm;
}

// termSubst
//
// Antikatastash ths metablhths x ston oro M me ton oro N
// H antikatastash ginetai symfwna me ton orismo thw selidas 149 twn shmeiwsewn
//
// Note:
// closed flags are kept updated during conversions with minimum complexity cost
// (constant time). It is not always possible to detect that a term became closed
// without cost, so a term marked as non-closed could in fact be closed. But the
// opposite should hold, terms marked as closed MUST be closed.

int termSubst(TERM *x, TERM *M, TERM *N, int mustClone) {
	TERM z, *y, *P, *clone;
	int found = 0;

	// nothing can be substituted in closed terms
	if(M->closed)
		return 0;

	switch(M->type) {
	 case TM_VAR:
	 	if(strcmp(M->name, x->name) == 0) {
			free(M->name);
			if(mustClone) {
				clone = termClone(N);
				*M = *clone;
				free(clone);
			} else
				*M = *N;

			found = 1;
		}
		break;

	 case TM_APPL:
		found = termSubst(x, M->lterm, N, mustClone);
		found = termSubst(x, M->rterm, N, mustClone || found) || found;

		// if both branches become closed then M also becomes closed
		M->closed = M->lterm->closed &&
			 			M->rterm->closed;
		break;

	 case TM_ABSTR:		
		y = M->lterm;
		P = M->rterm;

		//periptwsh 1: x = y
		if(strcmp(y->name, x->name) == 0)
			break;

		// If y is free in N then we should alpha-convert it to a different name to avoid capture
		// except if x is not free in P in which case no substitution will happen anyway
		//
		// NOTE
		// termIsFreeVar is a very constly check to make, and it is performed billions of times.
		// However, in 'practical' cases we never need to make such substitutions so these costly tests
		// always fail (in Queens N example we never enter in the following if).
		// Some profiling showed that 77% percent of the execution time was spent in termIsFree and
		// putting the hole 'if' in comments leads to an incredible speed boost (Queens 5 solved in 2 seconds
		// instead of 50) however it breaks the cases where substitution IS needed, for example where we
		// have free variables:
		//   (\x.\y.y x) y  ->  \a.a y  (bound y renamed to a)
		//   but with the 'if' in comments we incorrectly get \y.y y
		//
		// Starting with version 0.5 a good optimization is made using the term's 'closed' flag. In practical
		// cases we are using only closed terms. To exploit this fact each term has a closed flag which is calculated
		// once in the beggining of the execution and is updated during the conversions whenever possible without
		// computational overhead. A closed flag means that termSubst and termIsFreeVar can return immediately without
		// inspecting the term. This gives almost the same performance boost as removing the 'if' (but without breaking
		// cases with free variables), still some termIsFrees are called but very few.
		// Note: termIsFreeVar checks the closed flag itself but in the following 'if' we first check N->closed and
		//       P->closed to avoid extra function calls and avoid calling termIsFreeVar(N, y->name) if P->closed is set
		// 
		if(!N->closed && !P->closed &&
			termIsFreeVar(N, y->name) &&	termIsFreeVar(P, x->name)
			) {
			//printf("ISFREE\n");

			// x in FV(P) kai y in FV(N)
			// bound variable must be renamed before performing P[x:=N]
			z.type = TM_VAR;
			z.name = getVariable(N, P);
			z.closed = 0;

			termSubst(y, P, &z, 1);
			y->name = z.name;
		}
		found = termSubst(x, P, N, mustClone);

		// if P becomes closed then M also becomes closed
		M->closed = P->closed;
		break;

	 case TM_ALIAS:
		// aliases are closed terms so no substitution is possible
		// We should never reach here because of the closed flag
		assert(0);
		break;
	}

	return found;
}

// Epistrefei 1 an h metablhth name anhkei stis eley8eres metablhtes
// toy oroy t, diaforetika 0.

#ifndef NDEBUG
int freeNo;				// count the number of calls of termIsFree
#endif
int termIsFreeVar(TERM *t, char *name) {
	// closed terms have no free variables
	if(t->closed)
		 return 0;
#ifndef NDEBUG
	freeNo++;
#endif

	switch(t->type) {
	 case TM_VAR:
		return (strcmp(t->name, name) == 0);

	 case TM_APPL:
		return termIsFreeVar(t->lterm, name) ||
				 termIsFreeVar(t->rterm, name);

	 case TM_ABSTR:
		return strcmp(t->lterm->name, name) != 0 &&
				 termIsFreeVar(t->rterm, name);
	
	 case TM_ALIAS:
		//ta aliases prepei na einai kleistoi oroi (xwris eley8eres metablhtes)!
		return 0;

	 default:		//we never reach here!
		assert(0);
		return 0;
	}
}

// termConv
//
// Ektelei thn aristeroterh β h η metatroph poy yparxei gia to oro t
//
// Epistrofh
// 	1	An bre8hke metatrph
//		0	An den yparxei metatroph
//		-1	An synebh kapoio la8os
//
// Note:
// closed flags are kept updated during conversions with minimum complexity cost
// (constant time). It is not always possible to detect that a term became closed
// without cost, so a term marked as non-closed could in fact be closed. But the
// opposite should hold, terms marked as closed MUST be closed.

int termConv(TERM *t) {
	TERM *L, *M, *N, *x;
	int res, found;
	char closed;

	// verify that the closed bit has been set and is not garbage
	assert(t->closed == 0 || t->closed == 1);

	switch(t->type) {
	 case TM_VAR:
		return 0;

	 case TM_ABSTR:
		// Check for eta-conversion
		// \x.M x -> M  if x not free in M
		L = t->rterm;			// M x
		M = L->lterm;
		N = L->rterm;
		x = t->lterm;

		if(L->type == TM_APPL &&
			N->type == TM_VAR &&
			strcmp(N->name, x->name) == 0
			&& !termIsFreeVar(M, x->name)) {

			//η-conversion
			*t = *M;

			//free memory
			free(M);
			free(L);
			termFree(N);
			termFree(x);

			return 1;
		}

		return termConv(t->rterm);

	 case TM_APPL:
		// An o aristeros oros einai alias prepei na ton antikatasthsoume dioti mporei
		// na periexei afairesh. To while mpaine epeidh kai meta mporei na yparxei alias
		while(t->lterm->type == TM_ALIAS)
			if(termAliasSubst(t->lterm) != 0)
				return -1;

		//An den yparxei afairesh sta aristera tote den mporei
		//na ginei β-metatroph opote synexizoyme thn anazhthsh sto dentro
		if(t->lterm->type != TM_ABSTR) {
			res = termConv(t->lterm);
			return res != 0
				? res
				: termConv(t->rterm);
		}

		// Αν preced == 255 σημαίνει ότι η εφαρμογή είχε οριστεί με ~. Τότε κάνουμε
	   // πρώτα τις μετατροπές στο δεξιά υποδέντρο (call-by-value)
		if(t->preced == 255 && (res = termConv(t->rterm)) != 0)
			return res;

		L = t->lterm;
		x = L->lterm;
		M = L->rterm;
		N = t->rterm;

		//β-conversion
		closed = t->closed;
		found = termSubst(x, M, N, 0);
		*t = *M;

		// if t was closed, it remains closed (bete-conversion does not introduce free variables)
		// however the inverse can happen, a non-closed t can become closed if M becomes
		// closed itself (for example if x does not appear in M then any possible free variables of N
		// will disapear after the beta-conversion)
		t->closed = M->closed || closed;

		//free memory
		free(L);
		free(M);
		termFree(x);
		if(found)
			free(N);
		else
			termFree(N);

		return 1;

	 case TM_ALIAS:
		//Gia na doume an yparxei kapoia metatroph prepei na antikatasta8ei to
		//alias me ton antistoixo oro
		if(termAliasSubst(t) != 0)
			return -1;

		//Eyresh kapoias metatrophs ston neo oro
		return termConv(t);

	 default:										//we never reach here!
		assert(0);
		return -1;
	}
}

// termPower
//
// Epistrefei ton oro f^pow(a) o opoios orizetai sthn selida 162 twn shmeiwsewn

TERM *termPower(TERM *f, TERM *a, int pow) {
	TERM *newTerm;

	if(pow == 0)
		newTerm = termClone(a);
	else {
		newTerm = termNew();
		newTerm->type = TM_APPL;
		newTerm->name = NULL;
		newTerm->lterm = termClone(f);
		newTerm->rterm = termPower(f, a, pow-1);
	}

	return newTerm;
}

// termChurchNum
//
// Dhmiourgei kai epistrefei to church numeral
// poy antistoixei ston ari8mo n:	\f.\x.f^n(x)

TERM *termChurchNum(int n) {
	TERM *l1 = termNew(),
		  *l2 = termNew(),
		  *f  = termNew(),
		  *x  = termNew();

	f->type = TM_VAR;
	f->name = strdup("f");

	x->type = TM_VAR;
	x->name = strdup("x");

	l1->type = TM_ABSTR;
	l1->name = NULL;
	l1->lterm = f;
	l1->rterm = l2;

	l2->type = TM_ABSTR;
	l2->name = NULL;
	l2->lterm = x;
	l2->rterm = termPower(f, x, n);

	return l1;
}

// termNumber
//
// An o oros t einai church numeral tote epistrefei ton ari8mo
// ston opoio antistoixei, diaforetika epistrefei -1

int termNumber(TERM *t) {
	TERM *f, *x, *cur;
	int n = 0;

	//o prwtos oros prepei na einai \f.
	if(t->type != TM_ABSTR) return -1;
	f = t->lterm;

	//o deyteros oros prepei na einai \x. με x <> f
	if(t->rterm->type != TM_ABSTR) return -1;
	x = (t->rterm->lterm);
	if(strcmp(f->name, x->name) == 0) return -1;

	//anagwnrish toy oroy f^n(x), ypologismos toy n
	for(cur = t->rterm->rterm; ; cur = cur->rterm, n++) {
		if(cur->type == TM_VAR && strcmp(cur->name, x->name) == 0)
			return n;

		if(cur->type != TM_APPL ||
			cur->lterm->type != TM_VAR ||
			strcmp(cur->lterm->name, f->name) != 0)
			return -1;
	}
}	

// termIsList
//
// Epostrefei 1 an o oros t einai kwdikopoihsh kapoias listas
// dhladh sth morfh \s.s Head Tail h Nil: \x.\x.\y.x

int termIsList(TERM *t) {
	TERM *r;

	if(t->type != TM_ABSTR) return 0;

	r = t->rterm;
	switch(r->type) {
	 case TM_APPL:
		//elegxos gia th morfh \s.s Head Tail
		if(r->lterm->type == TM_APPL &&
			r->lterm->lterm->type == TM_VAR &&
			strcmp(r->lterm->lterm->name, t->lterm->name) == 0)
			return termIsList(r->rterm);
		break;

	 case TM_ABSTR:
		//elegxos gia th morfh Nil: \x.\x.\y.x
		if(r->rterm->type == TM_ABSTR &&
			r->rterm->rterm->type == TM_VAR &&
			strcmp(r->rterm->rterm->name, r->lterm->name) == 0)
			return 1;
		break;

	 default:
		;
	}

	return 0;
}

// termPrintList
//
// Ektypwnei enan ton oro t me thn morfh [A, B, C, ...]. O oros prepei
// na einai kwdikopoihsh listas (h termIsList(t) prepei na epistrefei 1)

void termPrintList(TERM *t) {
	TERM *r;
	int i = 0;

	printf("[");

	for(r = t->rterm; r->type == TM_APPL; r = r->rterm->rterm) {
		if(i++ > 0) printf(", ");
		termPrint(r->lterm->rterm, 1);
	}

	printf("]");
}

// termAliasSubst
//
// Antika8ista to alias t me ton antistoixo oro.
// Epistrefei 0 an o oros bre8hke h 1 an to alias den exei oristei.

int termAliasSubst(TERM *t) {
	TERM *newTerm;

	if(!(newTerm = termFromDecl(t->name))) {
		printf("Error: Alias %s is not declared.\n", t->name);
		return 1;
	}

	assert(newTerm->closed == 1);

	//antikatastash toy oroy
	free(t->name);
	*t = *newTerm;
	free(newTerm);

	return 0;
}

// termRemoveAliases
//
// Antika8ista ta ALIASES ston oro t me toy antistoixous orous.
// Epistreifei 0 an oles oi antikatastaseis eginan kanonika kai
// 1 an kapoio ALIAS den exei oristei
// An id != NULL tote antika8istatai mono to sygekrimeno alias

int termRemoveAliases(TERM *t, char *id) {

	switch(t->type) {
	 case(TM_VAR):
		 return 0;

	 case(TM_ABSTR):
		 return termRemoveAliases(t->rterm, id);

	 case(TM_APPL):
		 return termRemoveAliases(t->lterm, id) ||
				  termRemoveAliases(t->rterm, id);

	 case(TM_ALIAS):
		return !id || strcmp(id, t->name) == 0
			? termAliasSubst(t)
			: 0;

		//Antikatastash mono enos epipedoy
		//return termRemoveAliases(t);

	 default:				//we never reach here
		return 0;
	}
}

// termAlias2Var
//
// Αντικαθιστά όλες τις εμφανίσεις ενός alias με μια μεταβλητή.
// Χρησιμοποιείται κατά την αφαίρεση της αναδρμομής με την χρήση fixed point combinator

void termAlias2Var(TERM *t, char *alias, char *var) {

	switch(t->type) {
	 case(TM_VAR):
		return;

	 case(TM_ABSTR):
		termAlias2Var(t->rterm, alias, var);
		return;

	 case(TM_APPL):
		termAlias2Var(t->lterm, alias, var);
		termAlias2Var(t->rterm, alias, var);
		return;

	 case(TM_ALIAS):
		if(strcmp(alias, t->name) == 0) {
			t->type = TM_VAR;
			free(t->name);
			t->name = strdup(var);
		}
		return;
	}
}

void termRemoveOper(TERM *t) {
	TERM *alias, *appl;
	
	switch(t->type) {
	 case(TM_VAR):
	 case(TM_ALIAS):
		break;

	 case(TM_ABSTR):
		termRemoveOper(t->rterm);
		break;

	 case(TM_APPL):
		termRemoveOper(t->lterm);
		termRemoveOper(t->rterm);

		//Αν ο t έχει όνομα σημαίνει ότι είναι operator. Στην
		//περίπτωση αυτή κάνουμε τη μετατροπή
		//		a op b -> 'op' a b
		//
		// Ο operator ~ έχει ειδική μεταχείριση και χρησιμοποιείται κατά την εκτέλεση
	 	//	για να καθεοριστεί η σειρά αποτίμησης. Θέτωντας preced = 255 απλά "μαρκάρουμε" τον όρο

		if(t->name && strcmp(t->name, "~") == 0) {
			t->preced = 255;
			free(t->name);
			t->name = NULL;

		} else if(t->name) {
			// alias = op
			alias = termNew();
			alias->type = TM_ALIAS;
			alias->name = t->name;
			alias->closed = 1;							// aliases are closed terms
			t->name = NULL;

			// apple = op a
			appl = termNew();
			appl->type = TM_APPL;
			appl->name = NULL;

			appl->lterm = alias;
			appl->rterm = t->lterm;
			appl->closed = appl->rterm->closed;		// the application "op a" is closed if a is closed

			// t = (op a) b
			t->lterm = appl;
		}

		break;
	}
}

void termSetClosedFlag(TERM *t) {
	list_t *fvars = termFreeVars(t);
	list_destroy_nodes(fvars);
	list_destroy(fvars);
}

list_t* termFreeVars(TERM *t) {
	list_t *vars, *rvars;
	lnode_t *node;

	switch(t->type) {
	 case(TM_VAR):
		vars = list_create(LISTCOUNT_T_MAX);
		list_append(vars, lnode_create(t->name));
		break;

	 case(TM_ALIAS):
		vars = list_create(LISTCOUNT_T_MAX);
		break;

	 case(TM_ABSTR):
		vars = termFreeVars(t->rterm);
		while(node = list_find(vars, t->lterm->name, (int(*)(const void*, const void*))strcmp))
			lnode_destroy(list_delete(vars, node));
		break;

	 case(TM_APPL):
		vars = termFreeVars(t->lterm);
		rvars = termFreeVars(t->rterm);
		list_transfer(vars, rvars, list_first(rvars));
		list_destroy(rvars);
		break;
	}

	t->closed = list_isempty(vars);
	return vars;
}

// Vriskei mia metablhth poy na mhn yparxei stis listes l1 kai l2 dokimazontas
// diadoxika oles tis symboloseires me thn akolou8h seira
//		a, b, ..., z, aa, ab, .., ba, bb, ..., zz, aaa, aab, ...

char *getVariable(TERM *t1, TERM *t2) {
	char s[10] = {'a', '\0'};
	int curLen = 1, i;

	//kataskeyazoume symboloseires mexri na broume kapoia
	//poy na mhn anhkei se kamia apo tis listes
	while(termIsFreeVar(t1, s) || termIsFreeVar(t2, s)) {
		//ay3anoume ton teleytaio xarakthra. An aytos ginei 'z' ton
		//kanoyme 'a' kai ay3anoume ton prohgoumeno (meta to "az" einai to "ba") klp
		for(i = curLen-1; i >= 0 && ++s[i] == 'z'; i--)
			s[i] = 'a';

		//an oloi oi xarakthres eginan 'z' prepei na megalwsoume to
		//mhkos ths symboloseiras kai na 3anarxizoume apo to "aa..."
		if(i < 0) {
			s[curLen] = 'a';
			s[++curLen] = '\0';
		}
	}

	//h symboloseira bre8hke. Epistrefoume ena antigrafo.
	return strdup(s);
}


