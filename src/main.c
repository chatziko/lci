// Initialization and main loop
// Copyright Kostas Chatzikokolakis

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <replxx.h>

#include "parser.h"
#include "run.h"
#include "decllist.h"
#include "str_intern.h"

#define MAX_HISTORY_ENTRIES 100


static void completion_hook(char const* context, replxx_completions* lc, int* contextLen, void* ud);
static void hint_hook(char const* context, replxx_hints* lc, int* contextLen, ReplxxColor* c, void* ud);
static void completion_or_hint(char const* context, replxx_completions* lc_compl, replxx_hints* lc_hints);

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

	// init replxx, load history and set hooks
	Replxx *replxx = replxx_init();
	replxx_history_load(replxx, history_path);

	replxx_set_completion_callback( replxx, completion_hook, NULL);
	replxx_set_hint_callback( replxx, hint_hook, NULL );

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



// --- Completion and hints -------------------------------------------------------------------

static void completion_hook(char const* context, replxx_completions* lc, int* contextLen, void* ud) {
	completion_or_hint(context, lc, NULL);
}

static void hint_hook(char const* context, replxx_hints* lc, int* contextLen, ReplxxColor* c, void* ud) {
	completion_or_hint(context, NULL, lc);
}

static void completion_or_hint(char const* context, replxx_completions* lc_compl, replxx_hints* lc_hints) {
	// Context contains the full text up to the pointer, eg "foo bar".
	// We find the start of the last alphanumeric word, eg "bar".
	const char *start = context + strlen(context);
	while(start > context && (isalnum(start[-1]) || start[-1] == 'c'))
		start--;

	// completions/hints are aliases
	Vector candidates = decl_get_ids();

	// if we are at the first word of the string, complete also system commands
	if(start == context) {
		char *commands[] = { "DefOp", "ShowAlias", "Print", "FixedPoint", "Consult", "Set", "Help", "Quit" };
		for(int i = 0; i < sizeof(commands)/sizeof(char*); i++) {

			vector_insert_last(candidates, commands[i]);
		}
	}
	vector_sort(candidates, (CompareFunc)strcmp);

	// search for a match
	char *hint = NULL;
	for(int i = 0; i < vector_size(candidates); i++) {
		char *cand = vector_get_at(candidates, i);

		// we don't want operators
		if(!isalpha(cand[0]) && cand[0] != '_')
			continue;

		if(strncmp(start, cand, strlen(start)) == 0) {
			if(lc_compl) {
				replxx_add_completion(lc_compl, cand);
			} else if(hint != NULL) {
				hint = NULL;
				break;					// we show a hint only if there is exactly one match
			} else {
				hint = cand;
			}
		}
	}

	if(lc_hints && hint != NULL)
		replxx_add_hint(lc_hints, hint);

	vector_destroy(candidates);
}
