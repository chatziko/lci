// Initialization and main loop
// Copyright Kostas Chatzikokolakis

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <replxx.h>

#include "parser.h"
#include "run.h"
#include "str_intern.h"

#define MAX_HISTORY_ENTRIES 100


int main() {
	char *home = getenv("HOME");
	char *lcirc = ".lcirc";
	char *lci_history = "/.lci_history";

	// TODO: implement our own auto-complete
	Replxx *replxx = replxx_init();

	// load history from ~/.lci_history
	if(home) {
		char *path = (char*)malloc(sizeof(char) * (strlen(home) + strlen(lci_history) + 1));
		strcpy(path, home);
		strcat(path, lci_history);
		replxx_history_load(replxx, path);
		free(path);
	}

	printf("lci - A lambda calculus interpreter\n");
	printf("Type a term, Help for info or Quit to exit.\n\n");

	// consult .lcirc files in various places
	int found = 0;

#ifdef DATADIR
	// DATADIR/lci/.lcirc
	char *path = (char*)malloc(sizeof(char) * (strlen(DATADIR) + strlen(lcirc) + 6));
	strcpy(path, DATADIR);
	strcat(path, "/lci/");
	strcat(path, lcirc);
	if(consultFile(path) == 0)
	   found = 1;
	free(path);
#endif

	// ~/.lcirc
	if(home) {
		char *path = (char*)malloc(sizeof(char) * (strlen(home) + strlen(lcirc) + 2));
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
	while(!quit_called) {
		// read command
		char* line = (char*)replxx_input(replxx, "lci> ");
		if(!line) break;	// if eof exit

		// if empty read again.
		if(!*line) continue;

		replxx_history_add(replxx, line);

		parse_string(line);
	}

	// save history to ~/.lci_history
	if(home) {
		char *path = (char*)malloc(sizeof(char) * (strlen(home) + strlen(lci_history) + 1));
		strcpy(path, home);
		strcat(path, lci_history);

		replxx_set_max_history_size(replxx, MAX_HISTORY_ENTRIES);
		replxx_history_save(replxx, path);

		free(path);
	}

	// cleanup
	replxx_end(replxx);
	str_intern_cleanup();

	return 0;
}


