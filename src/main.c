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
	printf("lci - A lambda calculus interpreter\n");
	printf("Type a term, Help for info or Quit to exit.\n\n");

	// load history from ~/lci_history (if HOME is available) or "./lci_history"
	char *home = getenv("HOME");
	char *history_dir = home ? home : ".";
	char *history_file = "/.lci_history";
	char *history_path = malloc(strlen(history_dir) + strlen(history_file) + 1);

	strcpy(history_path, history_dir);
	strcat(history_path, history_file);

	Replxx *replxx = replxx_init();				// TODO: implement our own auto-complete
	replxx_history_load(replxx, history_path);

	// consult .lcirc files in various places
	char *lcirc = ".lcirc";
	int found = 0;

	#ifdef DATADIR
	// DATADIR/lci/.lcirc
	char *path = malloc(strlen(DATADIR) + strlen(lcirc) + 6);
	strcpy(path, DATADIR);
	strcat(path, "/lci/");
	strcat(path, lcirc);
	if(consultFile(path) == 0)
	   found = 1;
	free(path);
	#endif

	// ~/.lcirc
	if(home) {
		char *path = malloc(strlen(home) + strlen(lcirc) + 2);
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
	replxx_set_max_history_size(replxx, MAX_HISTORY_ENTRIES);
	replxx_history_save(replxx, history_path);
	free(history_path);

	// cleanup
	replxx_end(replxx);
	str_intern_cleanup();

	return 0;
}


