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

#pragma once

#include "kazlib/list.h"
#include "parser.h"


void termPrint(TERM *t, int isMostRight);
TERM *termNew();
void termFree(TERM *t);
void termGC();
TERM *termClone(TERM *t);

int termConv(TERM *t);

TERM *termChurchNum(int n);
int termNumber(TERM *t);

int termRemoveAliases(TERM *t, char *id);
void termAlias2Var(TERM *t, char *alias, char *var);

void termRemoveOper(TERM *t);
void termSetClosedFlag(TERM *t);

