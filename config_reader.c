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
#include "xo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef LABEL
#define LABEL "xo"
#endif

#define MAX_LINE_LENGTH 4096


static const char * default_locations[] =
{
    "~/.xo/" LABEL ".xo",
    "~/." LABEL ".xo",
    "/etc/xo/" LABEL ".xo",
    "/usr/local/share/xo/" LABEL ".xo",
    "/usr/share/xo/" LABEL ".xo",
    NULL
};

FILE * xo_config_find(const char * const * where)
{

    FILE * f = NULL;

    if (!where) {
        where = default_locations;
    }

    for (; *where; where++) {
        fprintf(stderr, LABEL ": trying %s\n", *where);
        f = fopen(*where, "r");
        if (!f) {
            if (errno == ENOENT) {
                continue;
            } else {
                fprintf(stderr,
                    LABEL ": error, failed to open config file %s: %s\n",
                    *where, strerror(errno));
                return NULL;
            }
        }
        break;
    }
    if (!f) {
        fprintf(stderr,
            LABEL ": error, no config files found\n");
        return NULL;
    }

    fprintf(stderr,
        LABEL ": found config %s\n", *where);

    return f;
}

struct xo * xo_config_load_file(struct xo * xo,
        FILE * fcfg)
{
    if (xo == NULL) {
        xo = xo_alloc();
    }
    
    static char line [MAX_LINE_LENGTH];
    
    while (fgets(line, MAX_LINE_LENGTH, fcfg), !feof(fcfg)) {
        float_type coefs [5];
        switch (line[0]) {
	    case '\0':
	    case '\n':
	    case ' ': // comments
	    case '\t':
		continue;
	    case '!': // print message
		if (line[1]) {
		    fprintf(stderr, LABEL ": '%s'\n", &line[1]);
		}
		continue;
            case '^':
                if (xo->n_chains == 0) {
		    fprintf(stderr,
                        LABEL ": error, chain begins with ^ source\n");
                    return NULL;
                }
                break;
            case '0' ... '9':
                xo_add_chain(xo, line[0] - '0');
                break;
            case 'M':
                xo_add_chain(xo, XO_MONO);
                break;
            case 'L':
                xo_add_chain(xo, XO_LEFT);
                break;
            case 'R':
                xo_add_chain(xo, XO_RIGHT);
                break;
            default:
		fprintf(stderr,
                    LABEL ": error, chain begins with unknown source type\n");
                return NULL;
        }

        enum filter_type type;

        switch (line[1]) {
            default:
            case '\0':
                fprintf(stderr,
                    LABEL ": error, invalid input\n");
                return NULL;

            case 'B':
                type = XO_FILTER_BQ;
                break;
            case 'L':
                type = XO_FILTER_SV_LP;
                break;
            case 'H':
                type = XO_FILTER_SV_HP;
                break;
        }
        
        char * l = &line[2], * ln;
        for (size_t i = 0; i < 5; i++) {
            coefs[i] = CAT(strtof, FP)(l, &ln);
            if (l == ln) {
		fprintf(stderr,
                    LABEL ": error, strtof empty (malformed coef?)\n");
		return NULL;
	    }
            l = ln;
        }
        
        xo_filter_set(
                xo_add_filter_to_chain(xo),
                coefs[0], coefs[1], coefs[2],
                coefs[3], coefs[4],
                type);
    }
    
    return xo;
}

struct xo * xo_config_load_existing(struct xo * xo,
        struct chain * chains, size_t n_chains)
{
    if (xo == NULL) {
	xo = xo_alloc();
    }
    xo->chains = chains;
    xo->n_chains = n_chains;

    return xo;
}

static const char * coefstr(float_type coef) {
    static char s[256];
    CAT(strfromf, FP)(s, sizeof(s), "%A", coef);
    return s;
}

void xo_config_write_file(struct xo * xo,
        FILE * fcfg)
{
    if (xo == NULL) {
        return;
    }
    for (size_t i = 0; i < xo->n_chains; i++) {
        struct chain * chain = &xo->chains[i];
        for (size_t j = 0; j < chain->n_filters; j++) {
            struct filter * filter = &chain->filters[j];
            if (j == 0) {
                fprintf(fcfg, "%c", '0' + (char)(chain->source % 10));
            } else {
                fprintf(fcfg, "^");
            }
            switch (filter->type) {
                case XO_FILTER_BQ:
                    fprintf(fcfg, "B ");
                    break;
                case XO_FILTER_SV_LP:
                    fprintf(fcfg, "H ");
                    break;
                case XO_FILTER_SV_HP:
                    fprintf(fcfg, "L ");
                    break;
                default:
                    fprintf(stderr, "writer: warning, unknown filter type\n");
                    fprintf(fcfg, "? ");
                    break;
            }
            fprintf(fcfg, "%s %s %s %s %s\n",
                    coefstr(filter->a0),
                    coefstr(filter->a1),
                    coefstr(filter->a2),
                    coefstr(filter->b1),
                    coefstr(filter->b2));
        }
    }
}

