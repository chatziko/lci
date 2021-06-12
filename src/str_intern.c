/* Basic string interning utilities

	Copyright (C) 2019 Stefanos Baziotis
	This file is part of LCI

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details. */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ADTMap.h"
#include "str_intern.h"

// -------------- STRING INTERNING -----------------

Map interns = NULL;

char *str_intern(char *str) {
    if(str == NULL)
        return NULL;
    
    if(interns == NULL) {
        interns = map_create((CompareFunc)strcmp, free, NULL);
        map_set_hash_function(interns, hash_string);
    }

    char* res = map_find(interns, str);

    if(res == NULL) {
        res = strdup(str);
        map_insert(interns, res, res);
    }

    return res;
}

char *str_intern_range(char *start, char *end) {
    char old = *end;
    *end = '\0';                    // quick and dirty
    char *res = str_intern(start);  // the string is copied
    *end = old;                     // restore
    return res;
}

void str_intern_cleanup() {
    if(interns != NULL)
        map_destroy(interns);
    interns = NULL;
}
