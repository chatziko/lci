// vim:noet:ts=3

// Functions that are used to "execute" terms or commands
// Copyright Kostas Chatzikokolakis

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
#elif __has_include(<conio.h>)
	#include <conio.h>
#elif __has_include(<sys/ioctl.h>)
	#include <sys/ioctl.h>
	#if __has_include(<termio.h>)		// linux
	#include <termio.h>
	typedef struct termio TERMIO;

	#elif __has_include(<termios.h>)	// OSX
	#include <termios.h>
	typedef struct termios TERMIO;
	#define TCGETA TIOCGETA
	#define TCSETA TIOCSETA
	#endif
#endif

#include "dparse.h"
#include "run.h"
#include "parser.h"
#include "termproc.h"
#include "decllist.h"
#include "str_intern.h"


int trace;
int options[OPTNO] = {0, 0, 0, 0, 1};
int quit_called = 0;

#ifndef NDEBUG
extern int freeNo;
#endif

#ifdef __EMSCRIPTEN__
EM_JS(int, js_read_char, (), {
	return Asyncify.handleAsync(Module.readChar);
});
#endif

// read a single character (without buffering)
static int read_single_char() {
	#ifdef __EMSCRIPTEN__
	return js_read_char();

	#elif __has_include(<conio.h>)				// available on windows
	return _getch();

	#elif __has_include(<sys/ioctl.h>)
	// change tio settings to make getchar return immediately after the
	// first character (without pressing enter). We keep the old
	// settings to restore later.
	TERMIO tio, oldtio;
	ioctl(0, TCGETA, &tio);						// read settings
	oldtio = tio;								// save settings
	tio.c_lflag &= ~(ICANON | ECHO);			// change settings
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 0;
	ioctl(0, TCSETA, &tio);						// set new settings

	int res = getchar();
	ioctl(0, TCSETA, &oldtio);					// restore terminal settings
	return res;

	#else
	return getchar();
	#endif
}

void execTerm(TERM *t) {
	int redno = -1, res = 1,
		showExec = getOption(OPT_SHOWEXEC);
	long stime = clock();
	char c;

	#ifdef __EMSCRIPTEN__
	double last_sleep = emscripten_get_now();
	#endif

	trace = getOption(OPT_TRACE);

	// remove operators before executing
	termRemoveOper(t);

	// calculate closed flag for all sub-terms (must be done after termRemoveOper)
	termSetClosedFlag(t);
#ifndef NDEBUG
	freeNo = 0;
#endif

	switch(execSystemCmd(t)) {
	 case 1:
		// during execution, SIGINT signals enable trace
		signal(SIGINT, sigHandler);

		// perform all reductions
		do {
			redno++;

			#ifdef __EMSCRIPTEN__
			// occasionally sleep to allow the browser to be responsive
			if(redno % 100 == 0 && (emscripten_get_now() - last_sleep) > 400) {
				emscripten_sleep(1);
				last_sleep = emscripten_get_now();
			}
			#endif

			if(trace) {
				termPrint(t, 1);
				printf("  ?> ");
				fflush(stdout);
				do {
					switch(c = read_single_char()) {
					 case 'c':
						 printf("continue\n");
						 trace = 0;
						 break;
					 case 's':
						 printf("step\n");
						 break;
					 case 'a':
					 case EOF:
						 printf("abort\n");
						 break;
					 default:
						 c = '?';
						 printf("\nCommands: (s)tep, (c)ontinue, (a)bort  ?> ");
						 fflush(stdout);
					}
				} while(c == '?');

				if(c == 'a' || c == EOF)
					break;

			} else if(showExec) {
				termPrint(t, 1);
				printf("\n");
			}

		} while((res = termConv(t)) == 1);

		// if execution is finished, print result
		if(res == 0) {
			printf("\n");
			termPrint(t, 1);
			printf("\n(%d reductions, %.2fs CPU)\n",
				redno, (double)(clock()-stime) / CLOCKS_PER_SEC);
#ifndef NDEBUG
			printf("%d termIsFree's\n", freeNo);
#endif
		}

		// restore default signal handler
		signal(SIGINT, SIG_DFL);

		break;

	 case -1:
		fprintf(stderr, "Error: Missformed sytem command. Type Help for info.\n");
		break;

	 case -2:
		// Quit command. Return 1 to exit the program
		quit_called = 1;
		break;

	 default:
		break;
	}

	// free memory
	termFree(t);
	termGC();					// call terms garbageCollector
}

// execSystemCmd
//
// Checks if term t is a system command and if so executes the command. System commands
// are of the form Cmd param1 param2 ...
// Returns
// 	 1 If the term is not a system command
// 	 0 If the term is a system command and execution was successful
// 	-1 If the term is a system command but there was some error during execution
// 	-2 If the term is a Quit system command

typedef enum {
	STR_DEFOP, STR_SHOWALIAS, STR_PRINT, STR_FIXPOINT, STR_CONSULT, STR_SET, STR_HELP, STR_QUIT,
	STR_XFX, STR_YFX, STR_XFY, STR_TRACE, STR_SHOWPAR, STR_GREEKLAMBDA, STR_SHOWEXEC, STR_READABLE, STR_ON, STR_OFF
} STR_CONSTANTS;

static char* str_constants[] = {
	"DefOp", "ShowAlias", "Print", "FixedPoint", "Consult", "Set", "Help", "Quit",
	"xfx", "yfx", "xfy", "trace", "showpar", "greeklambda", "showexec", "readable", "on", "off"
};

int execSystemCmd(TERM *t) {
	// interned constants, initialize on first execution
	static char* icon[sizeof(str_constants)/sizeof(char*)];
	if(icon[0] == NULL)
		for(int i = 0; i < sizeof(str_constants)/sizeof(char*); i++)
			icon[i] = str_intern(str_constants[i]);

	TERM *stack[10], **sp = stack, *par;
	int parno = 0;

	// find the left-most term and keep params in the stack
	while(t->type == TM_APPL) {
		*sp++ = t->rterm;
		parno++;
		t = t->lterm;
	}

	// the left-most term must be one of the predefined aliases
	if(t->type != TM_ALIAS)
		return 1;

	if(t->name == icon[STR_DEFOP]) {
		// DefOp name preced assoc
		//
		// Stores an operator's declaration
		char *oper;
		int prec;
		ASS_TYPE ass;

		if(parno != 3) return -1;

		// param 1: operator
		par = *--sp;
		if(par->type != TM_ALIAS) return -1;
		oper = par->name;

		// param 2: precedence
		par = *--sp;
		prec = termNumber(par);
		if(prec == -1) return -1;

		// param 3: associativity
		par = *--sp;
		if(par->type != TM_ALIAS && par->type != TM_VAR) return -1;
		if(par->name == icon[STR_XFX])
			ass = ASS_NONE;
		else if(par->name == icon[STR_YFX])
			ass = ASS_LEFT;
		else if(par->name == icon[STR_XFY])
			ass = ASS_RIGHT;
		else
			return -1;
	
		// add the operator's declaration
		addOper(str_intern(oper), prec, ass);

	} else if(t->name == icon[STR_SHOWALIAS]) {
		// ShowAlias
		//
		// Prints the definition of all stored aliases, or of a specific one
		// if given as a parameter.
		char *id = NULL;

		if(parno != 0 && parno != 1) return -1;
		if(parno == 1) {
			par = *--sp;
			if(par->type != TM_ALIAS) return -1;
			id = par->name;
		}

		printDeclList(id);

	} else if(t->name == icon[STR_PRINT]) {
		// Print
		//
		// Prints the term given as a parameter
		if(parno != 1) return -1;

		termPrint(*--sp, 1);
		printf("\n");

	} else if(t->name == icon[STR_FIXPOINT]) {
		// FixedPoint
		//
		// Removes recursion from aliases using a fixed point combinator
		int n = 0;

		if(parno != 0) return -1;

		while(findAndRemoveCycle())
			n++;

		if(n > 0)
			printf("%d cycles removed using fixed point combinator Y.\n", n);
		else
			printf("No cycles found\n");

	} else if(t->name == icon[STR_CONSULT]) {
		// Consult file
		//
		// Reads a file and executes its commands
		if(parno != 1) return -1;

		par = *--sp;
		if(par->type != TM_ALIAS && par->type != TM_VAR) return -1;

		switch(consultFile(par->name)) {
		 case 0:
			printf("Successfully consulted %s\n", par->name);
			break;
		 case -1:
			printf("Error: cannot open %s\n", par->name);
			break;
		}

	} else if(t->name == icon[STR_SET]) {
		// Set option value
		//
		// Changes the value of an option
		OPT opt;
		int value;

		if(parno != 2) return -1;

		par = *--sp;
		if(par->type != TM_VAR) return -1;
		if(par->name == icon[STR_TRACE])
			opt = OPT_TRACE;
		else if(par->name == icon[STR_SHOWPAR])
			opt = OPT_SHOWPAR;
		else if(par->name == icon[STR_GREEKLAMBDA])
			opt = OPT_GREEKLAMBDA;
		else if(par->name == icon[STR_SHOWEXEC])
			opt = OPT_SHOWEXEC;
		else if(par->name == icon[STR_READABLE])
			opt = OPT_READABLE;
		else
			return -1;

		par = *--sp;
		if(par->name == icon[STR_ON])
			value = 1;
		else if(par->name == icon[STR_OFF])
			value = 0;
		else return -1;

		options[opt] = value;

	} else if(t->name == icon[STR_HELP]) {
		// Help
		//
		// Prints help message
		printf("\nlci - A lambda calculus interpreter\n\n");
		printf("Type a lambda term to compute its normal form\n");
		printf("or enter one of the following system commands:\n\n");

		printf("FixedPoint\t\tRemoves recursion using fixed point comb. Y\n");
		printf("DefOp name prec ass\tDefines an operator\n");
		printf("ShowAlias [name]\tList the specified or all stored aliases\n");
		printf("Print term\t\tDisplays the term\n");
		printf("Consult file\t\tReads and interprets the specified file\n");
		printf("Set option (on|off)\tChanges one of the following options:\n\t\t\ttrace, showexec, showpar, greeklambda, readable\n");
		printf("Help\t\t\tDisplays this message\n");
		printf("Quit\t\t\tQuit the program (same as Ctrl-D)\n\n");

	} else if(t->name == icon[STR_QUIT]) {
		if(parno != 0) return -1;
		return -2;

	} else
		// The alias is not one of the predefined ones
		return 1;

	return 0;
}

// consultFile
//
// Reads and executes fname file
// Return value:
//   0   ok
//   -1  cannot open file
//   -2  syntax error

int consultFile(char *fname) {
	FILE *f = fopen(fname, "rb");		// need binary to make fseek/ftell work on windows
	if(f == NULL)
		return -1;

	// read the whole file in 'source'
	fseek(f, 0, SEEK_END);
	int length = ftell(f);
	fseek(f, 0, SEEK_SET);
	char *source = malloc((length+1) * sizeof(char));
	source[length] = '\0';
	int read = fread(source, 1, length, f);
	fclose(f);
	
	if(read != length)
		return -1;

	// parse (see dparser/sample_parser.c)
	if(!parse_string(source)) {
		fprintf(stderr, "Errors found in %s.\n", fname);
		return -2;
	}

	return 0;
}

// getOption
// 
// Returns the value of option opt
int getOption(OPT opt) {
	return options[opt]; 
}

// sigHandler
//
// Handles SIGINT signal during execution. It enables the trace functionality.

void sigHandler(int sig) {
	trace = 1;

	// in old solaris systems (?) SIG_DFL is restored before calling this
	// handler, so we have to re-enable it.
	signal(SIGINT, sigHandler);
}

