// vim:noet:ts=3

// Functions to manipulate the list of declarations
// Copyright Kostas Chatzikokolakis

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ADTMap.h"
#include "decllist.h"
#include "termproc.h"
#include "parser.h"
#include "str_intern.h"


// node in the declaration graph (used for finding cycles)
typedef struct graph_node {
	char* id;
	Vector neighbours;				// ids of neighbours
	int flag;
	struct graph_node *prev;		// previous node in the dfs traversal
} GraphNode;

typedef struct {
	GraphNode *start;
	GraphNode *end;
	int size;
} GraphCycle;


static Map declarations = NULL;		// id => TERM
static Map operators = NULL;		// id => OPER


static TERM *getDeclarationTerm(char *id);

static void findAliases(TERM *t, Vector aliases);
static GraphCycle dfs(GraphNode *curNode, Map graph);
static int getCycleSize(GraphNode *start, GraphNode *end);
static void removeCycle(GraphCycle c);
static TERM *getIndexTerm(int varno, int n, char *tuple);

static int compare_pointers(Pointer a, Pointer b) {
	return a - b;
}

// termAddDecl
//
// Adds the term to the declaration list with the given (interned) id.
// If a term with this id already exists it is replaced.

void termAddDecl(char *id, TERM *term, bool freeOld) {
	// declared term must be closed
	assert(term->closed);

	if(declarations == NULL) {
		// strings are interned, so we can just compare them as pointers
		declarations = map_create(compare_pointers, NULL, NULL);
		map_set_hash_function(declarations, hash_pointer);
	}

	// set termFree as destroy function if we want to free old terms
	map_set_destroy_value(declarations, freeOld ? (DestroyFunc)termFree : NULL);

	// if a declaration with this id exists, it will be replaced
	map_insert(declarations, id, term);
}

// getDeclarationTerm
//
// Returns the term corresponding to the declaration with
// the given (interned) id, or NULL if there is no such declaration.

static TERM *getDeclarationTerm(char *id) {
	return declarations  != NULL
		? map_find(declarations, id)
		: NULL;
}

// termFromDecl
//
// Returns a clone of the term stored with the given id

TERM *termFromDecl(char *id) {
	TERM *term = getDeclarationTerm(id);

	return term != NULL
		? termClone(term)
		: NULL;
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

void printCycle(GraphNode *start, GraphNode *end) {
	for(GraphNode *curNode = end; curNode != start; curNode = curNode->prev)
		printf("%s, ", curNode->id);
	printf("%s\n", start->id);
}

static void destroy_graph_node(Pointer n) {
	GraphNode *node = n;
	vector_destroy(node->neighbours);
	free(node);
}

int findAndRemoveCycle() {
	// create the graph
	Map graph = map_create(compare_pointers, NULL, destroy_graph_node);		// id => GraphNode
	map_set_hash_function(graph, hash_pointer);

	for(MapNode node = map_first(declarations); node != MAP_EOF; node = map_next(declarations, node)) {
		char *id = map_node_key(declarations, node);
		TERM *term = map_node_value(declarations, node);

		GraphNode *node = malloc(sizeof(GraphNode));
		node->id = id;
		node->flag = 0;
		node->prev = NULL;
		node->neighbours = vector_create(0, NULL);		// vector of ids

		findAliases(term, node->neighbours);

		map_insert(graph, id, node);
	}

	// DFS might need to be executed multiple times if the graph is not connected
	GraphCycle bestCycle = { .size = 0 };

	for(MapNode map_node = map_first(graph); map_node != MAP_EOF; map_node = map_next(graph, map_node)) {
		GraphNode *node = map_node_value(graph, map_node);
		if(node->flag == 0) {
			GraphCycle newCycle = dfs(node, graph);
			if(newCycle.size > bestCycle.size)
				bestCycle = newCycle;
		}
	}

	// if a cycle was found, remove it
	int res = 0;
	if(bestCycle.size > 0) {
		//printf("best cycle size: %d\n", bestCycle.size);
		//printCycle(bestCycle.start, bestCycle.end);
		removeCycle(bestCycle);
		res = 1;
	}

	map_destroy(graph);

	return res;
}

// dfs
//
// Depth first search for finding cycles. Finds and returns a maximal
// cycle, i.e. one not contained in some other cycle.

static GraphCycle dfs(GraphNode *curNode, Map graph) {
	GraphCycle bestCycle, newCycle;
	int curSize;

	bestCycle.size = 0;
	bestCycle.start = bestCycle.end = NULL;
	curNode->flag = 1;

	// process neighborhoods
	for(int i = 0; i < vector_size(curNode->neighbours); i++) {
		char *newNode_id = vector_get_at(curNode->neighbours, i);
		GraphNode *newNode = map_find(graph, newNode_id);

		if(!newNode) // if the alias is not defined, ignore it
			continue;

		switch(newNode->flag) {
		 case 0:
			 newNode->prev = curNode;
			 newCycle = dfs(newNode, graph);

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

static int getCycleSize(GraphNode *start, GraphNode *end) {
	int size = 1;

	for(GraphNode *cur = end; cur != start; cur = cur->prev)
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

static void removeCycle(GraphCycle cycle) {
	TERM *t;
	char *newId;		// interned newId

	// if there is more than one alias in the cycle, merge them in a tuple
	if(cycle.size > 1) {
		//construct new id
		char newId_raw[50] = "";
		GraphNode *node;
		int i;
		for(i = 0, node = cycle.end; i < cycle.size; i++, node = node->prev) {
			if(i > 0) strcat(newId_raw, "_");
			strcat(newId_raw, node->id);
		}
		newId = str_intern(newId_raw);

		// construct tupled function  \y.y Alias1 Alias2 ... Alias<n>
		TERM *body = create_variable(str_intern("y"));
		for(i = 0, node = cycle.end; i < cycle.size; i++, node = node->prev)
			body = create_application(body, NULL, create_alias(node->id));

		t = create_abstraction(str_intern("y"), body);

		termSetClosedFlag(t);				// mark sub-terms as closed
		termAddDecl(newId, t, true);

		// replace aliases with their corresponding terms
		termRemoveAliases(t, NULL);

		// Aliases contained in the cycle have been merged in a tuple.
		// So their appearances are replaced by Index calls
		for(i = 0, node = cycle.end; i < cycle.size; i++) {
			char *tmpId = node->id;
			node = node->prev;

			TERM *tmpTerm = getIndexTerm(cycle.size, i, newId);
			termSetClosedFlag(tmpTerm);
			termAddDecl(tmpId, tmpTerm, true);		// free the old terms, we got clones from termRemoveAliases

			// replace this specific alias with its definition in the whole program
			for(MapNode node = map_first(declarations); node != MAP_EOF; node = map_next(declarations, node)) {
				TERM *term = map_node_value(declarations, node);
				termRemoveAliases(term, tmpId);
			}
		}
	} else {
		t = getDeclarationTerm(cycle.start->id);
		newId = cycle.start->id;
	}

	// To remove recursion, appearances of the alias within its body are replaced with the
	// variable _me and we add a fixed point combinator.
	// Hence the term A=N becomes A=Y \_me.N[A:=_me]
	termAlias2Var(t, newId, str_intern("_me"));						// Change alias to _me

	TERM *newTerm = termNew();										// Application of Y to the term
	newTerm->type = TM_APPL;
	newTerm->name = NULL;

	newTerm->lterm = termNew();								// Y
	newTerm->lterm->type = TM_ALIAS;
	newTerm->lterm->name = str_intern("Y");

	newTerm->rterm = termNew();								// Remove \_me.
	TERM *tmpTerm = newTerm->rterm;
	tmpTerm->type = TM_ABSTR;
	tmpTerm->name = NULL;
	tmpTerm->rterm = t;

	tmpTerm->lterm = termNew();								// _me variable
	tmpTerm->lterm->type = TM_VAR;
	tmpTerm->lterm->name = str_intern("_me");

	// Change declaration
	termSetClosedFlag(newTerm);
	termAddDecl(newId, newTerm, false);						// don't free the old term
}

// getIndexTerm
//
// Returns the following lambda-term:
//   TUPLE \x1.\x2. ... \xvarno.x<n>
// which chooses the n-th element of a varno-tuple

static TERM *getIndexTerm(int varno, int n, char *tuple) {
	char name[20];
	sprintf(name, "x%d", n);
	TERM *var = create_variable(str_intern(name));	// x<n>

	TERM *abstr = var;								// \x1.\x2...\x<varno>.x<n>
	for(int i = varno-1; i >= 0; i--) {
		sprintf(name, "x%d", i);
		abstr = create_abstraction(str_intern(name), abstr);
	}

	return create_application(create_alias(tuple), NULL, abstr);
}

// printDeclList
//
// Prints the entire declaration list

void printDeclList(char *id) {

	if(id) {
		TERM *term = getDeclarationTerm(id);
		if(term == NULL) {
			printf("Error: alias %s not found.\n", id);
		} else {
			printf("%s = ", id);
			termPrint(term, 1);
			printf("\n");
		}
	} else {
		for(MapNode node = map_first(declarations); node != MAP_EOF; node = map_next(declarations, node)) {
			char *id = map_node_key(declarations, node);
			TERM *term = map_node_value(declarations, node);

			printf("%s = ", id);
			termPrint(term, 1);
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

Vector decl_get_ids() {
	Vector res = vector_create(0, NULL);

	for(MapNode node = map_first(declarations); node != MAP_EOF; node = map_next(declarations, node)) {
		char *id = map_node_key(declarations, node);
		vector_insert_last(res, id);
	}
	return res;
}

void decl_cleanup() {
	if(declarations != NULL) {
		map_set_destroy_value(declarations, (DestroyFunc)termFree);		// we want to free
		map_destroy(declarations);
		declarations = NULL;
	}

	if(operators != NULL) {
		map_destroy(operators);
		operators = NULL;
	}
}