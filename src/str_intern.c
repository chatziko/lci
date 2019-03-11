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
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// IMPORTANT(stefanos): That code uses C99 features.

#include "str_intern.h"

#define MAX(x, y) ((x) >= (y) ? (x) : (y))
typedef unsigned char byte_t;

// ---- WRAPPERS FOR ALLOCATION FUNCTIONS

void *srealloc(void *ptr, size_t num_bytes) {
    ptr = realloc(ptr, num_bytes);
    if (!ptr) {
        perror("srealloc failed");
        exit(1);
    }
    return ptr;
}

void *smalloc(size_t num_bytes) {
    void *ptr = malloc(num_bytes);
    if (!ptr) {
        perror("smalloc failed");
        exit(1);
    }
    return ptr;
}

// -------------- STRETCHY BUFFER -----------------

typedef struct buf_hdr {
    size_t cap;
    size_t len;
    byte_t buf[0];  // C99
} buf_hdr_t;

// private
#define buf__hdr(b) ((buf_hdr_t *)((byte_t *)(b) - offsetof(buf_hdr_t, buf)))
#define buf__cap(b) (((b)) ? buf__hdr(b)->cap : 0U)
#define buf__maybegrow(b, n) (((n) <= buf__cap(b)) ? 0 : ((b) = buf__grow((void *)(b), (n), sizeof(*(b)))))

// public
#define buf_len(b) (((b)) ? buf__hdr(b)->len : 0U)
#define buf_push(b, ...) (buf__maybegrow((b), buf_len(b)+1), (b)[buf__hdr(b)->len++] = (__VA_ARGS__))
#define buf_free(b) (((b)) ? (free(buf__hdr(b)), (b) = NULL) : 0)
#define buf_reserve(b, n) buf__maybegrow(b, n)

static void *buf__grow(const void *buf, size_t new_len, size_t elem_size) {
    // NOTE(stefanos): Minimum of 16 elements
    size_t new_cap = MAX(2 * buf__cap(buf), MAX(new_len, 16));
    assert(new_len <= new_cap);
    size_t new_size = offsetof(buf_hdr_t, buf) + new_cap * elem_size;
    buf_hdr_t *new_hdr;
    if (buf) {
        new_hdr = (buf_hdr_t *)srealloc(buf__hdr(buf), new_size);
    }
    else {
        new_hdr = (buf_hdr_t *)smalloc(new_size);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buf;
}


// -------------- STRING INTERNING -----------------

// NOTE(stefanos): NULL-initialized as a global variable and that's needed
// for the stretchy_buffer use.
interned_str_t *interns;

// NOTE(stefanos): That eventually should a hash-table because that has
// O(n) but it's easy and effective enough for template version.
char *str_intern_range(char *start, char *end) {
    size_t len = end - start;
    for (size_t i = 0; i != buf_len(interns); ++i) {
        // NOTE(stefanos): You may assume that because we test interns[i].len == len,
        // that we just need to strcmp, BUT consider that string starting at 'start' is
        // not necessarilly NULL-terminated at 'end' and so strcmp() might yield wrong result.
        if (interns[i].len == len && strncmp(interns[i].str, start, len) == 0) {
            return interns[i].str;
        }
    }
    char *str = smalloc(len);
    memcpy(str, start, len);
    str[len] = 0;
    buf_push(interns, (interned_str_t) { len, str });  // C99
    return str;
}

// NOTE(stefanos): That could be done faster, but that's easier.
char *str_intern(char *str) {
    return str_intern_range(str, str + strlen(str));
}

void initialize_string_interning(char **strings, size_t len) {
    for(size_t i = 0; i != len; ++i)
        str_intern(strings[i]);
}
