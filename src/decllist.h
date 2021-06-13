// Declarations for decllist.c
// Copyright Kostas Chatzikokolakis

#pragma once

#include "ADTVector.h"
#include "parser.h"


typedef struct tag_oper{ 
	char *id;
	int preced;
	ASS_TYPE assoc;
} OPER;


void termAddDecl(char *id, TERM *term);
TERM *termFromDecl(char *id);

int findAndRemoveCycle();

void printDeclList(char *id);

OPER *getOper(char *id);
void addOper(char *id, int preced, ASS_TYPE assoc);
