// Declarations for run.c
// Copyright Kostas Chatzikokolakis

#pragma once

#include "parser.h"

#define OPTNO	5
typedef enum {OPT_TRACE = 0, OPT_SHOWPAR, OPT_GREEKLAMBDA, OPT_SHOWEXEC, OPT_READABLE} OPT;

extern int quit_called;


void execTerm(TERM *t);
int execSystemCmd(TERM *t);
int consultFile(char *fname);
int getOption(OPT opt);

void sigHandler(int sig);
