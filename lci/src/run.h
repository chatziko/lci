/* Declarations for run.c

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

#ifndef RUN_H
#define RUN_H

#include "grammar.h"

#define OPTNO	5
typedef enum {OPT_TRACE = 0, OPT_SHOWPAR, OPT_GREEKLAMBDA, OPT_SHOWEXEC, OPT_READABLE} OPT;


void progInterpret(COMMAND *cmdList);
void execTerm(TERM *t);
void progFree(COMMAND *cmdList);
int execSystemCmd(TERM *t);
int consultFile(char *fname);
int getOption(OPT opt);

void sigHandler(int sig);


#endif


