// vim:noet:ts=3

// Declarations for parser.c
// Copyright Kostas Chatzikokolakis

#pragma once

#include "dparse.h"

// Add __attribute__((packed)) if compiler supports it
// #if defined(__GNUC__) || defined(__clang__)
// #define ATTR_PACKED __attribute__((packed))
// #else
#define ATTR_PACKED
// #endif

// Precedence and associativity of applications
#define APPL_PRECED	100
#define APPL_ASSOC	ASS_LEFT


// Structs for representing a lambda-program as a tree. During parsing, we will
// use the parse tree to construct the program tree used during execution.

// Note: ATTR_PACKED (#defined __atribute__((packed))) instructs the compiler to
// use 1 byte instead of 4 for the enum
typedef enum { TM_ABSTR, TM_APPL, TM_VAR, TM_ALIAS } ATTR_PACKED TERM_TYPE;
typedef enum { ASS_LEFT, ASS_RIGHT, ASS_NONE } ATTR_PACKED ASS_TYPE;

// Note: put bigger fields first (lterm, rterm, name) to allow
// the compiler to align the struct without wasting space.
// results in a smaller sizeof(TERM)
//
typedef struct term_tag {
	struct term_tag *lterm;					// left and right children
	struct term_tag *rterm;					// (for applications and abstractions)
	char *name;								// name (for variables, aliases and applications with an operator)
	TERM_TYPE type;							// variable, application or abstraction
	char closed;							// during parsing: 1 if application in parenthesis. during execution: 1 if no free vars
} ATTR_PACKED TERM;


int parse_string(char *source);

// used in grammar.g
TERM *create_variable(char *name);
TERM *create_number(char *s);
TERM *create_alias(char *name);
TERM *create_abstraction(TERM *var, TERM *right);
TERM *create_application(TERM *left, char *oper_name, TERM *right);
TERM *create_list(TERM *first, D_ParseNode *rest);
TERM *create_bracket(TERM *t);

void parse_cmd_declaration(char *id, TERM *t);
void parse_cmd_term(TERM *t);