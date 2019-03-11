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

#pragma once

typedef struct interned_str {
    size_t len;
    char *str;
} interned_str_t;

char *str_intern_range(char *start, char *end);
char *str_intern(char *str);
void initialize_string_interning(char **strings, size_t len);
