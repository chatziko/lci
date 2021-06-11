/* Declarations for decllist.c

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

#include "ADTVector.h"
#include "grammar.h"


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
