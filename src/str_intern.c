// Basic string interning utilities
// Copyright Kostas Chatzikokolakis and Stefanos Baziotis

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
