/* Initialization and main loop

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
#include <string.h>

#include "parser.h"
#include "run.h"


int main() {
	TERM *t;
	char buffer[300];

	printf("lci - A lambda calculus interpreter\n");
	printf("Copyright (C) 2003 Kostas Hatzikokolakis\n");
	printf("This is FREE SOFTWARE and comes with ABSOLUTELY NO WARRANTY\n\n");
	printf("Type a term, Help for info or Quit to exit.\n");

	// read and execute .lcirc
	consultFile(".lcirc");

	// read and execute commands
	while(!feof(stdin)) {
		printf("lci> ");
		if(!fgets(buffer, sizeof(buffer), stdin)) break;
		if(strcmp(buffer, "\n") == 0) continue;

		scInputType = SC_BUFFER;
		scInput = buffer;
		getToken(NULL);

		if(parse((void*)&t, TK_TERM) == PAR_OK)
			execTerm(t);
		else
			printf("Syntax error\n\n");
	}

	return 0;
}


