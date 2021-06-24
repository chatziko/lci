// vim:noet:ts=3

// Term manipulation functions
// Copyright Kostas Chatzikokolakis

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "kazlib/list.h"
#include "ADTVector.h"

#include "termproc.h"
#include "decllist.h"
#include "parser.h"
#include "run.h"
#include "str_intern.h"


static int termIsIdentity(TERM *t);
static int termIsList(TERM *t);
static void termPrintList(TERM *t);

static int termIsFreeVar(TERM *t, char *name);
static int termSubst(TERM *x, TERM *M, TERM *N, int mustClone);
static int termAliasSubst(TERM *t);
static list_t* termFreeVars(TERM *t);
static char *getVariable(TERM *t1, TERM *t2);


static Vector termPool = NULL;


// termPrint
//
// Prints a lambda term
// If showPar = 0 then only the required parentheses are printed
// If greeklambda = 1 then a greek lambda character is used instead of "\"
// (must have your terminal set to use UTF-8 as its locale to display)

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
		// if the term is a church numeral we print the corresponding number
		if(readable && (num = termNumber(t)) != -1)
			printf("%d", num);
		else if(readable && termIsList(t))
			termPrintList(t);
		else if(readable && termIsIdentity(t))
			printf("I");
		else {
			if(showPar || !isMostRight) printf("(");

			printf(greekLambda ? "\u03BB" : "\\");
			termPrint(t->lterm, 0);
			printf(".");
			termPrint(t->rterm, 1);

			if(showPar || !isMostRight) printf(")");
		}
		break;

	 case TM_APPL:
		if(showPar) printf("(");

		termPrint(t->lterm, 0);

		if(t->name)
			printf(" %s ", t->name);
		else
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
// Free a TERM's memory

void termFree(TERM *t) {
	// if NULL do nothing
	if(!t) return;

	if(termPool == NULL)
		termPool = vector_create(0, NULL);

	// put the term in the pool, but dont recurse to subterms, they
	// will be freed when the term is used (this seems to be faster).
	vector_insert_last(termPool, t);
}

void termGC() {
	if(termPool == NULL)
		return;

	while(vector_size(termPool) > 0)
		free(termNew());

	vector_destroy(termPool);
	termPool = NULL;
}

TERM *termNew() {
	if(termPool == NULL)
		termPool = vector_create(0, NULL);

	int size = vector_size(termPool);
	if(size > 0) {
		TERM* t = vector_get_at(termPool, size-1);
		vector_remove_last(termPool);

		// free subterm (they are not freed when the term is freed)
		if(t->type == TM_APPL || t->type == TM_ABSTR) {
			termFree(t->lterm);
			termFree(t->rterm);
		}
		return t;

	} else {
		return malloc(sizeof(TERM));
	}
}

// termClone
//
// Creates and returns a clone of a term (and all its subterms).

TERM *termClone(TERM *t) {
	TERM *newTerm;

	newTerm = termNew();
	newTerm->type = t->type;
	newTerm->closed = t->closed;
	newTerm->name = t->name;			// fast copy, strings are interned

	if(t->type == TM_APPL || t->type == TM_ABSTR) {
		newTerm->lterm = termClone(t->lterm);
		newTerm->rterm = termClone(t->rterm);
	}

	return newTerm;
}

// termSubst
//
// Replaces variable x in term M with term N.
// Substitution is performed based on the definition at page 149 of the lecture notes
// (update: 15 years later, the above comment sounds funny and useless :D)
//
// Note:
// closed flags are kept updated during conversions with minimum complexity cost
// (constant time). It is not always possible to detect that a term became closed
// without cost, so a term marked as non-closed could in fact be closed. But the
// opposite should hold, terms marked as closed MUST be closed.

static int termSubst(TERM *x, TERM *M, TERM *N, int mustClone) {
	TERM z, *y, *P, *clone;
	int found = 0;

	// nothing can be substituted in closed terms
	if(M->closed)
		return 0;

	switch(M->type) {
	 case TM_VAR:
	 	if(M->name == x->name) {		// fast compare, strings are interned
			if(mustClone) {
				clone = termClone(N);
				*M = *clone;

				clone->lterm = clone->rterm = NULL;	// free but make sure that the
				termFree(clone);					// subterms are not freed
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

		// case 1: x = y
		if(y->name == x->name)	// fast comparison, strings are interned
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

// Returns 1 if variable 'name' (must be interned) belongs to the
// free variables of term t, otherwise 0.

#ifndef NDEBUG
int freeNo;				// count the number of calls of termIsFree
#endif
static int termIsFreeVar(TERM *t, char *name) {
	// closed terms have no free variables
	if(t->closed)
		 return 0;
#ifndef NDEBUG
	freeNo++;
#endif

	switch(t->type) {
	 case TM_VAR:
		return t->name == name;

	 case TM_APPL:
		return termIsFreeVar(t->lterm, name) ||
				 termIsFreeVar(t->rterm, name);

	 case TM_ABSTR:
		return t->lterm->name != name &&
				 termIsFreeVar(t->rterm, name);
	
	 case TM_ALIAS:
		// aliases must be closed terms (no free variables)!
		return 0;

	 default:		// we never reach here!
		assert(0);
		return 0;
	}
}

// termConv
//
// Performs the left-most beta or eta reduction in term t
//
// Returns
// 	1	If a reduction was found
//		0	If there is no reduction
//		-1	If some error happened
//
// Note:
// closed flags are kept updated during conversions with minimum complexity cost
// (constant time). It is not always possible to detect that a term became closed
// without cost, so a term marked as non-closed could in fact be closed. But the
// opposite should hold, terms marked as closed MUST be closed.

int termConv(TERM *t) {
	static char* str_tilde = NULL;
	if(str_tilde == NULL)
		str_tilde = str_intern("~");

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
			N->name == x->name &&
			!termIsFreeVar(M, x->name)) {

			// eta-conversion
			*t = *M;

			// free terms. By freeing L we also free M,N, but we
			// should _not_ free M's subterms (they are copied in t)
			M->lterm = M->rterm = NULL;
			termFree(L);
			termFree(x);

			return 1;
		}

		return termConv(t->rterm);

	 case TM_APPL:
		// If the left-most term is an alias it needs to be substituted cause it might contain
		// an abstraction. while is needed cause we might still have an alias afterwards.
		while(t->lterm->type == TM_ALIAS)
			if(termAliasSubst(t->lterm) != 0)
				return -1;

		// If no abstraction exist on the left-hand side then a beta-reduction is not possible,
		// so we continue the search in the tree.
		if(t->lterm->type != TM_ABSTR) {
			res = termConv(t->lterm);
			return res != 0
				? res
				: termConv(t->rterm);
		}

		// If the application has beed defined with ~,
		// we perform the reductions in the right subtree first (call-by-value)
		if(t->name == str_tilde && (res = termConv(t->rterm)) != 0)
			return res;

		L = t->lterm;		// L = \x.M
		x = L->lterm;
		M = L->rterm;
		N = t->rterm;

		// beta-reduction: \x.M N -> M[x:=N]
		closed = t->closed;
		found = termSubst(x, M, N, 0);
		*t = *M;

		// if t was closed, it remains closed (bete-conversion does not introduce free variables)
		// however the inverse can happen, a non-closed t can become closed if M becomes
		// closed itself (for example if x does not appear in M then any possible free variables of N
		// will disapear after the beta-conversion)
		t->closed = M->closed || closed;

		// free terms. freeing L will also free x,M, but in case of M we
		// should not free its subterms (which have been copied to t)
		M->lterm = M->rterm = NULL;
		termFree(L);

		if(found)							// if N was substituted in M, we should
			N->lterm = N->rterm = NULL;		// not free its subterms
		termFree(N);

		return 1;

	 case TM_ALIAS:
		// to check for reductions we need to substitute the alias with the corresponding term
		if(termAliasSubst(t) != 0)
			return -1;

		// search for reductions in the new term
		return termConv(t);

	 default:										// we never reach here!
		assert(0);
		return -1;
	}
}

// termNumber
//
// If term t is a numeral then return its corresponding number, otherwise -1.
// Supported:
// - Church:       \f.\x.f^<N>(x)
// - Scott:        <N+1> = \s.\z.s <N>
// - Unprocessed:  (Succ (Succ ... (Succ 0)))

int termNumber(TERM *t) {
	// Unprocessed: Succ(...(Succ(0))
	//
	if(t->type == TM_ALIAS && t->name == str_intern("0"))
		return 0;
	else if(t->type == TM_APPL && t->lterm->type == TM_ALIAS && t->lterm->name == str_intern("Succ"))
		return 1 + termNumber(t->rterm);

	// Scott numerals (with inversed argument order):
	//    0     = \s.\z.z			(same as church-0, so no need to treat separately)
	//    <N+1> = \s.\z.s <N>
	//
	if(t->type == TM_ABSTR && t->rterm->type == TM_ABSTR) {
		TERM *body = t->rterm->rterm;

		if(body->type == TM_APPL && body->lterm->type == TM_VAR && body->lterm->name == t->lterm->name) {
			int n = termNumber(body->rterm);
			if(n != -1)
				return n + 1;
		}
	}

	// church numerals
	TERM *f, *x, *cur;
	int n = 0;

	// first term must be \f.
	if(t->type != TM_ABSTR) return -1;
	f = t->lterm;

	// second term must be \x. with x != f
	if(t->rterm->type != TM_ABSTR) return -1;
	x = (t->rterm->lterm);
	if(f->name == x->name) return -1;

	// recognize term f^n(x), compute n
	for(cur = t->rterm->rterm; ; cur = cur->rterm, n++) {
		if(cur->type == TM_VAR && cur->name == x->name)
			return n;

		if(cur->type != TM_APPL ||
			cur->lterm->type != TM_VAR ||
			cur->lterm->name != f->name)
			return -1;
	}
}	

// termIsList
//
// Returns 1 if t is an encoding of a list, that is of the form
// \s.s Head Tail h Nil: \x.\x.\y.x

static int termIsList(TERM *t) {
	TERM *r;

	if(t->type != TM_ABSTR) return 0;

	r = t->rterm;
	switch(r->type) {
	 case TM_APPL:
		// check for the form \s.s Head Tail
		if(r->lterm->type == TM_APPL &&
			r->lterm->lterm->type == TM_VAR &&
			r->lterm->lterm->name == t->lterm->name)
			return termIsList(r->rterm);
		break;

	 case TM_ABSTR:
		// check for the form Nil: \x.\x.\y.x
		if(r->rterm->type == TM_ABSTR &&
			r->rterm->rterm->type == TM_VAR &&
			r->rterm->rterm->name == r->lterm->name)
			return 1;
		break;

	 default:
		;
	}

	return 0;
}

static int termIsIdentity(TERM *t) {
	return
		t->type == TM_ABSTR &&
		t->rterm->type == TM_VAR &&
		t->lterm->name == t->rterm->name;
}

// termPrintList
//
// Prints a term of the form [A, B, C, ...]. The term must be the encoding of a list
// (termIsList(t) must return 1).

static void termPrintList(TERM *t) {
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
// Substitutes aliast t with its corresponding term. Returns 0 if the term was found
// or 1 if the alias is undefined.

static int termAliasSubst(TERM *t) {
	TERM *newTerm;

	if(!(newTerm = termFromDecl(t->name))) {
		printf("Error: Alias %s is not declared.\n", t->name);
		return 1;
	}

	assert(newTerm->closed == 1);

	// substitute term
	*t = *newTerm;

	// free newTerm, but without its subterms (which are copied in t)
	newTerm->lterm = newTerm->rterm = NULL;
	termFree(newTerm);

	return 0;
}

// termRemoveAliases
//
// Substitutes all aliases in term t with the corresponding terms. Returns 0 if
// all substitutions were successful, or 1 if some alias was undefined.
// If id != NULL the only this specific alias is substituted.

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
		return !id || id == t->name
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
// Replaces all occurrences of an aliast with an already interned variable.
// Used when removing recursion via a fixed point combinator.

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
		if(alias == t->name) {
			t->type = TM_VAR;
			t->name = var;		// already interned
		}
		return;
	}
}

void termRemoveOper(TERM *t) {
	static char* str_tilde = NULL;
	if(str_tilde == NULL)
		str_tilde = str_intern("~");

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

		//	If t has a name the it's an operator. In this case we perform the conversion:
		//		a op b -> 'op' a b
		//
		//	Operator '~' has a special meaning, used during execution to decide the evaluation
		//	order.

		if(t->name && t->name != str_tilde) {
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

static int compare_pointers(const void *key1, const void *key2) {
    return key1 - key2;
}

static list_t* termFreeVars(TERM *t) {
	list_t *vars = NULL, *rvars;
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
		while((node = list_find(vars, t->lterm->name, compare_pointers)) != NULL)
			lnode_destroy(list_delete(vars, node));
		break;

	 case(TM_APPL):
		vars = termFreeVars(t->lterm);
		rvars = termFreeVars(t->rterm);
		list_transfer(vars, rvars, list_first(rvars));
		list_destroy(rvars);
		break;
	
	 default:
		assert(0 /* invalid t-> type */);
	}

	t->closed = list_isempty(vars);
	return vars;
}

// Finds a variable not contained in lists l1 and l2, by trying all strings in
// the following order:
//		a, b, ..., z, aa, ab, .., ba, bb, ..., zz, aaa, aab, ...

static char *getVariable(TERM *t1, TERM *t2) {
	char s[10] = "a";
	int curLen = 1, i;

	// build strings until we find one not contained in one of the lists
	while(termIsFreeVar(t1, s) || termIsFreeVar(t2, s)) {
		// increment the last character. When it becomes 'z' we change it to 'a'
		// and continue incrementing the previous character, etc (after "az" we
		// have "ba").
		for(i = curLen-1; i >= 0 && ++s[i] == 'z'; i--)
			s[i] = 'a';

		// if all characters became 'z' we need to increase the size of the string,
		// and start from 'aa...'
		if(i < 0) {
			s[curLen] = 'a';
			s[++curLen] = '\0';
		}
	}

	// string found, return a copy
	return str_intern(s);
}


