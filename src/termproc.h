// Declarations for termproc.c
// Copyright Kostas Chatzikokolakis

#pragma once

#include "kazlib/list.h"
#include "parser.h"


void termPrint(TERM *t, int isMostRight);
TERM *termNew();
void termFree(TERM *t);
void termGC();
TERM *termClone(TERM *t);

int termConv(TERM *t);

int termNumber(TERM *t);

int termRemoveAliases(TERM *t, char *id);
void termAlias2Var(TERM *t, char *alias, char *var);

void termRemoveOper(TERM *t);
void termSetClosedFlag(TERM *t);

