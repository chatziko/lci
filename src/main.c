/* Initialization and main loop

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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "grammar.h"
#include "parser.h"
#include "run.h"

#define MAX_HISTORY_ENTRIES 100


int main() {
	TERM *t;
	char *home = getenv("HOME"),
		 *lcirc = ".lcirc",
		 *path;

#ifdef USE_READLINE
	char *buffer = NULL;
	char *lci_history = "/.lci_history";

	// disable file auto-complete
	// TODO: implement our own auto-complete
	rl_bind_key ('\t', rl_insert);

	// load history from ~/.lci_history
	if(home) {
		path = (char*)malloc(sizeof(char) * (strlen(home) + strlen(lci_history) + 1));
		strcpy(path, home);
		strcat(path, lci_history);
		read_history(path);
		free(path);
	}
#else
	char buffer[300];
	char *s;
#endif

	printf("lci - A lambda calculus interpreter\n");
	printf("Copyright (C) 2004-8 Kostas Chatzikokolakis\n");
	printf("This is FREE SOFTWARE and comes with ABSOLUTELY NO WARRANTY\n\n");
	printf("Type a term, Help for info or Quit to exit.\n");

	// consult .lcirc files in various places
	int found = 0;

#ifdef DATADIR
	// DATADIR/lci/.lcirc
	path = (char*)malloc(sizeof(char) * (strlen(DATADIR) + strlen(lcirc) + 6));
	strcpy(path, DATADIR);
	strcat(path, "/lci/");
	strcat(path, lcirc);
	if(consultFile(path) == 0)
	   found = 1;
	free(path);
#endif

	// ~/.lcirc
	if(home) {
		path = (char*)malloc(sizeof(char) * (strlen(home) + strlen(lcirc) + 2));
		strcpy(path, home);
		strcat(path, "/");
		strcat(path, lcirc);
		if(consultFile(path) == 0)
		   found = 1;
		free(path);
	}

	// ./.lcirc
	if(consultFile(lcirc) == 0)
	   found = 1;

	if(!found)
	   fprintf(stderr, "warning: no .lcirc file was found\n");

	// read and execute commands
	while(1) {
		// read command
#ifdef USE_READLINE
		free(buffer);
		buffer = readline("lci> ");
		if(!buffer) break;	// if eof exit
#else
		printf("lci> ");
		fflush(stdout);
		if(!fgets(buffer, sizeof(buffer), stdin))
			break;	// if eof exit

		// remove trailing '\n' (if exists)
		for(s = buffer; *s != '\0' && *s != '\n'; s++)
			;
		*s = '\0';
#endif
		// if empty read again.
		if(!*buffer) continue;

#ifdef USE_READLINE
		// add line to readline history
		add_history(buffer);
#endif

		scInputType = SC_BUFFER;
		scInput = buffer;
		getToken(NULL);

		if(parse((void*)&t, TK_TERM) == PAR_OK) {
			if(execTerm(t) != 0)
				break;
		} else
			printf("Syntax error\n\n");
	}

#ifdef USE_READLINE
	// save history to ~/.lci_history
	if(home) {
		path = (char*)malloc(sizeof(char) * (strlen(home) + strlen(lci_history) + 1));
		strcpy(path, home);
		strcat(path, lci_history);

		write_history(path);
#if HAVE_HISTORY_TRUNCATE_FILE
		history_truncate_file(path, MAX_HISTORY_ENTRIES);
#endif
		free(path);
	}
#endif

	return 0;
}


