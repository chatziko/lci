/* Functions that are used to "execute" terms or commands

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

//for ioctl function
#include <unistd.h>
#include <sys/ioctl.h>
#include <termio.h>

#include "run.h"
#include "parser.h"
#include "termproc.h"
#include "decllist.h"


int trace;
int options[OPTNO] = {0, 0, 0, 0, 1};


void execTerm(TERM *t) {
	int redno = -1, res = 1,
		showExec = getOption(OPT_SHOWEXEC);
	long stime = clock();
	char c;

	trace = getOption(OPT_TRACE);

	//Αφαίρεση των operators πριν την εκτέλεση
	termRemoveOper(t);

	switch(execSystemCmd(t)) {
	 case 1:
		//kata thn diarkeia ths ekteleshs ta shmata SIGNT energopoioyn to trace
		signal(SIGINT, sigHandler);

		//kanoyme ola ta reductions
		do {
			redno++;

			if(trace) {
				//allazontas tis parametrous sto tio kanoyme thn getchar na epistrefei
				//ameses meta ton prwto xarakthra (xwris enter) kratame tis palies
				//parametrous sto oldtio gia epanafora argotera.
				struct termio tio, oldtio;
				ioctl(0, TCGETA, &tio);						//read settings
				oldtio = tio;									//save settings
				tio.c_lflag &= ~(ICANON | ECHO);			//changes settings
				tio.c_cc[VMIN] = 1;
				tio.c_cc[VTIME] = 0;
				ioctl(0, TCSETA, &tio);						//set new settings

				termPrint(t, 1);
				printf("  ?>");
				do {
					switch(c = getchar()) {
					 case 'c':
						 printf("continue\n");
						 trace = 0;
						 break;
					 case 's':
						 printf("step\n");
						 break;
					 case 'a':
						 printf("abort\n");
						 break;
					 default:
						 c = '?';
						 printf("\nCommands: (s)tep, (c)ontinue, (a)bort  ?>");
					}
				} while(c == '?');

				//epanafora ry8misewn termatikoy
				ioctl(0, TCSETA, &oldtio);

				if(c == 'a') break;

			} else if(showExec) {
				termPrint(t, 1);
				printf("\n");
			}
		} while((res = termConv(t)) == 1);

		//An h metatroph teleiwse kanonika ektypwsh
		if(res == 0) {
			printf("\n");
			termPrint(t, 1);
			printf("\n(%d reductions, %.2fs CPU)\n",
				redno, (double)(clock()-stime) / CLOCKS_PER_SEC);
		}

		//Epanafora toy default handler
		signal(SIGINT, SIG_DFL);

		break;

	 case -1:
		fprintf(stderr, "Error: Missformed sytem command. Type Help for info.\n");
		break;

	 default:
		break;
	}

	//Απελευθέρωση μνήμης
	termFree(t);
}

// execSystemCmd
//
// Ελέγχει αν ο όρος t είναι εντολή συστήματος και στην περίπτωση αυτή εκτελεί
// την εντολή. Οι εντολές συστήματος έχουν την μορφή Cmd param1 param2 ...
// Επιστρέφει
//		1	Αν ο όρος δεν είναι εντολή συστήματος
//		0	Αν ο όρος είναι εντολή συστήματος και εκτελέστηκε κανονικά
//		-1 Αν ο όρος είνια εντολή συστήματος αλλά συνέβη κάποιο σφάλμα κατά την εκτέλεση

int execSystemCmd(TERM *t) {
	TERM *stack[10], **sp = stack, *par;
	int parno = 0;

	//ευρεση αριστερότερου όρου και φύλαξη των παραμέτρων στη στοίβα
	while(t->type == TM_APPL) {
		*sp++ = t->rterm;
		parno++;
		t = t->lterm;
	}

	//Ο αριστερότερος όρος πρέπει να είνει κάποιο από τα προκαθορισμένα ALIASES
	if(t->type != TM_ALIAS)
		return 1;

	if(strcmp(t->name, "DefOp") == 0) {
		// DefOp name preced assoc
		//
		// Καταχωρεί τη δήλωση ενός operator
		char *oper;
		int prec;
		ASS_TYPE ass;

		if(parno != 3) return -1;

		//παραμετρος 1: τελεστής
		par = *--sp;
		if(par->type != TM_ALIAS) return -1;
		oper = par->name;

		//παράμετρος 2: προτεραιότητα
		par = *--sp;
		prec = termNumber(par);
		if(prec == -1) return -1;

		//παράμετρος 3: προσεταιριστικότητα
		par = *--sp;
		if(par->type != TM_ALIAS && par->type != TM_VAR) return -1;
		if(strcmp(par->name, "xfx") == 0)
			ass = ASS_NONE;
		else if(strcmp(par->name, "yfx") == 0)
			ass = ASS_LEFT;
		else if(strcmp(par->name, "xfy") == 0)
			ass = ASS_RIGHT;
		else
			return -1;
	
		//καταχωρηση της δήλωσης του operator
		addOper(strdup(oper), prec, ass);

	} else if(strcmp(t->name, "ShowAlias") == 0) {
		// ShowAlias
		//
		// Τυπώνει τον ορισμό όλων των αποθηκευμένων aliases ή ενός
		// συγκεκριμένου αν δοθεί ως παράμετρος
		char *id = NULL;

		if(parno != 0 && parno != 1) return -1;
		if(parno == 1) {
			par = *--sp;
			if(par->type != TM_ALIAS) return -1;
			id = par->name;
		}

		printDeclList(id);

	} else if(strcmp(t->name, "Print") == 0) {
		// Print
		//
		// Τυπώνει τον όρο που δίνεται ως παράμετρος
		if(parno != 1) return -1;

		termPrint(*--sp, 1);
		printf("\n");

	} else if(strcmp(t->name, "FixedPoint") == 0) {
		// FixedPoint
		//
		// Αφαιρεί την αναδρομή από τα aliases με τη χρήση ενός fixed point combinator
		int n = 0;

		if(parno != 0) return -1;

		while(findCycle())
			n++;

		if(n > 0)
			printf("%d cycles removed using fixed point combinator Y.\n", n);
		else
			printf("No cycles found\n");

	} else if(strcmp(t->name, "Consult") == 0) {
		// Consult file
		//
		// Διαβάζει ένα αρχείο και εκτελεί τις εντολές
		if(parno != 1) return -1;

		par = *--sp;
		if(par->type != TM_ALIAS && par->type != TM_VAR) return -1;

		if(consultFile(par->name) == 0)
			printf("Successfully consulted %s\n", par->name);

	} else if(strcmp(t->name, "Set") == 0) {
		// Set option value
		//
		// Apouhkeyei thn timh ths parametroy
		OPT opt;
		int value;

		if(parno != 2) return -1;

		par = *--sp;
		if(par->type != TM_VAR) return -1;
		if(strcmp(par->name, "trace") == 0)
			opt = OPT_TRACE;
		else if(strcmp(par->name, "showpar") == 0)
			opt = OPT_SHOWPAR;
		else if(strcmp(par->name, "haskell") == 0)
			opt = OPT_HASKELL;
		else if(strcmp(par->name, "showexec") == 0)
			opt = OPT_SHOWEXEC;
		else if(strcmp(par->name, "readable") == 0)
			opt = OPT_READABLE;
		else
			return -1;

		par = *--sp;
		if(strcmp(par->name, "on") == 0)
			value = 1;
		else if(strcmp(par->name, "off") == 0)
			value = 0;
		else return -1;

		options[opt] = value;

	} else if(strcmp(t->name, "Help") == 0) {
		// Help
		//
		// Εκτύπωση μηνύματος βοήθειας
		printf("\nlci - A lambda calculus interpreter\n\n");
		printf("Type a lambda term to compute its normal form\n");
		printf("or enter one of the following system commands:\n\n");

		printf("FixedPoint\t\tRemoves recursion using fixed point comb. Y\n");
		printf("DefOp name prec ass\tDefines an operator\n");
		printf("ShowAlias [name]\tList the specified or all stored aliases\n");
		printf("Print term\t\tDisplays the term\n");
		printf("Consult file\t\tReads and interprets the specified file\n");
		printf("Set option (on|off)\tChanges one of the following options:\n\t\t\ttrace, showexec, showpar, haskell, readable\n");
		printf("Help\t\t\tDisplays this message\n");
		printf("Quit\t\t\tQuit the program (same as Ctrl-D)\n");

		printf("\nCopyright (C) 2003  Kostas Hatzikokolakis\n\n");

		printf("This program is free software; you can redistribute it and/or modify\n");
		printf("it under the terms of the GNU General Public License as published by\n");
		printf("the Free Software Foundation; either version 2 of the License, or\n");
		printf("(at your option) any later version.\n\n");

		printf("This program is distributed in the hope that it will be useful,\n");
		printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
		printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
		printf("GNU General Public License for more details.\n\n");

		printf("You should have received a copy of the GNU General Public License\n");
		printf("along with this program; if not, write to the Free Software\n");
		printf("Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n\n");

	} else if(strcmp(t->name, "Quit") == 0) {
		if(parno != 0) return -1;
		exit(0);

	} else
		//Το alias δεν είναι κανένα από τα προκαθορισμένα
		return 1;

	return 0;
}

// consultFile
//
// Διαβάζει το αρχείο fname και το εκτελει

int consultFile(char *fname) {
	FILE *f;

	if(!(f = fopen(fname, "r"))) {
		fprintf(stderr, "Error: cannot open file %s\n", fname);
		return -1;
	}

	//parse file
	scInputType = SC_FILE;
	scInput = f;
	getToken(NULL);

	if(parse(NULL, -1) != PAR_OK) {
		fprintf(stderr, "Error: syntax error in line %d.\n", scLineNo);
		fclose(f);
		return -2;
	}

	fclose(f);
	return 0;
}

// getOption
// 
// Epistrefei thn timh ths perametroy o
int getOption(OPT opt) {
	return options[opt]; 
}

// sigHandler
//
// Handler του σήματος SIGINT κατά τη διάρκει της εκτέλεσης.
// Ενεργοποιεί τη διαδικασία trace

void sigHandler(int sig) {
	trace = 1;

	//sta suns prin thn klhsh ths synarthshs epaneferetai o handler
	//se SIG_DFL. Etsi prepei na 3anaenergopoih8ei
	signal(SIGINT, sigHandler);
}



//void progInterpret(COMMAND *cmdList) {
	//for(; cmdList; cmdList = cmdList->next)
		//switch(cmdList->type) {
		 //case CM_QUEST:
			 //execTerm(cmdList->term);
			 //break;

		 //case CM_DECL:
			 //termAddDecl(cmdList->id, cmdList->term);
			 //break;
		//}	
//}

//void progFree(COMMAND *cmdList) {
	//COMMAND *tmp;

	//while(cmdList) {
		//switch(cmdList->type) {
		 //case CM_QUEST:
			 //termFree(cmdList->term);
			 //break;

		 //case CM_DECL:
			 ////H mnhmh gia ta declarations den eley8erwnetai dioti exoun
			 ////apo8hkeytei sthn antistoixh list

			 ////free(cmdList->id);
			 ////termFree(cmdList->term);
			 //break;
		//}

		//tmp = cmdList;
		//cmdList = cmdList->next;
		//free(tmp);
	//}
//}

