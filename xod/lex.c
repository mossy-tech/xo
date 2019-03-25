/* Copyright © 2019 Noah Santer <personal@mail.mossy-tech.com>
 *
 * This file is part of xo.
 * 
 * xo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * xo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with xo.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

#include "lex.h"
#include "keywords.gen.h"

#include "../xo.h"

#define ISWHITE(c) (c == ' ')
#define ISBREAK(c) (c == ';' || c == '\n')

static struct token tokenize(const char ** sptr)
{
    const char * s, * e;

    // move to next token character
    for (s = *sptr; ISWHITE(*s); s++);
    
    // at end of string?
    if (!*s) return (struct token) { NULL };

    // handle breaker character tokens
    if (ISBREAK(*s)) {
        *sptr = s + 1;
        return (struct token) { s, 1 };
    }

    // move to next boundary (end, white, ';')
    for (e = s; *e && !ISWHITE(*e) & !ISBREAK(*e); e++);

    // resume at e
    *sptr = e;

    return (struct token) { s, e - s };
}

static bool token_get_keyword(struct token token, enum yytokentype * result)
{
    struct keyword * kwd = in_word_set(token.start, token.length);
    if (kwd) {
        *result = kwd->key;
        return true;
    }
    return false;
}

static bool token_get_integer(struct token token, long * result)
{
    char * endptr;
    long n = strtol(token.start, &endptr, 0);
    size_t l = endptr - token.start;
    if (l == token.length) {
        *result = n;
        return true;
    }
    return false;
}

static bool token_get_float_type(struct token token, double * result)
{
    char * endptr;
    double n = strtod(token.start, &endptr);
    size_t l = endptr - token.start;
    if (l == token.length) {
        *result = n;
        return true;
    }
    return false;
}

enum yytokentype yylex(YYSTYPE *lvalp, const char ** sptr)
{
    struct token token = tokenize(sptr);
    if (!token.start) {
        return 0;
    }

    enum yytokentype kwd;

    if (ISBREAK(token.start[0])) {
        return ';';
    }

    if (token_get_keyword(token, &kwd)) {
        return kwd;
    }

    if (token_get_integer(token, &(*lvalp).num)) {
        return LIT_NUMERIC;
    }

    if (token_get_float_type(token, &(*lvalp).flt)) {
        return LIT_FLOAT;
    }

    (*lvalp).str = token;
    return LIT_STRING;
}


