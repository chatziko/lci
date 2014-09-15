/* Declarations for termproc.c

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

#ifndef TERMPROC_H
#define TERMPROC_H

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "kazlib/list.h"
#include "grammar.h"


void termPrint(TERM *t, int isMostRight);
TERM *termNew();
void termFree(TERM *t);
void termGC();
TERM *termClone(TERM *t);

int termSubst(TERM *x, TERM *M, TERM *N, int mustClone);
int termIsFreeVar(TERM *t, char *name);
int termConv(TERM *t);

TERM *termPower(TERM *f, TERM *a, int pow);
TERM *termChurchNum(int n);
int termNumber(TERM *t);
int termIsList(TERM *t);
void termPrintList(TERM *t);

int termAliasSubst(TERM *t);
int termRemoveAliases(TERM *t, char *id);
void termAlias2Var(TERM *t, char *alias, char *var);

void termRemoveOper(TERM *t);
void termSetClosedFlag(TERM *t);
list_t* termFreeVars(TERM *t);

char *getVariable(TERM *t1, TERM *t2);


#endif


