/* Declarations for termproc.c

	Copyright (C) 2003 Kostas Hatzikokolakis
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

#include "grammar.h"


void termPrint(TERM *t, int isMostRight);
void termFree(TERM *t);
TERM *termClone(TERM *t);

void termSubst(TERM *x, TERM *M, TERM *N);
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

char *getVariable(TERM *t1, TERM *t2);


#endif


