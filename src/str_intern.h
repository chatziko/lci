// Basic string interning utilities
// Copyright Kostas Chatzikokolakis and Stefanos Baziotis

#pragma once

char *str_intern(char *str);
char *str_intern_range(char *start, char *end);
void str_intern_cleanup();
