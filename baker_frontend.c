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

#include <string.h>
#include <stdlib.h>

#define SMAX 256

static char sa0[SMAX];
static char sa1[SMAX];
static char sa2[SMAX];
static char sb1[SMAX];
static char sb2[SMAX];
 
static const char * coefstr(char s[], float_type coef) {
    CAT(strfromf, FP)(s, SMAX, "%A", coef);
    return s;
}

int main(int argc, char ** argv) {
    
    if (argc < 2) {
        fprintf(stderr, "Syntax: %s CONFIG [...]\n", argv[0]);
        return 1;
    }
    
    struct xo * xo = NULL;
    
    FILE * out = stdout;
    fprintf(out, "/* baked from:\n");
    
    for (int i = 1; i < argc; i++) {
        FILE * f = fopen(argv[i], "r");
        if (!f) {
            fprintf(stderr, "Cannot open file %s\n", argv[i]);
            return 1;
        }
        xo = xo_config_load_file(xo, f);
        if (!xo) {
            fprintf(stderr, "Failed to parse file %s\n", argv[i]);
            return 1;
        }
	fprintf(out, " * %s\n", argv[i]);
	fclose(f);
    }
    fprintf(out, " */\n\n");

    fprintf(out, "#include \"xo.h\"\n\n");

//    xo_describe(xo, printf, 1, 0);


    fprintf(out, "struct xo * xo_add_baked_chains(struct xo * xo)\n{\n");

    for (size_t i = 0; i < xo->n_chains; i++) {
	fprintf(out,
		"  static struct filter filters_%zu[] = {\n",
		i);
	
	struct chain * chain = &xo->chains[i];
	for (size_t j = 0; j < chain->n_filters; j++) {
	    struct filter * f = &chain->filters[j];
            fprintf(out,
                     "    { .a0 = %s, .a1 = %s, .a2 = %s,"
                     ".b1 = %s, .b2 = %s },\n",
                     coefstr(sa0, f->a0),
                     coefstr(sa1, f->a1),
                     coefstr(sa2, f->a2),
                     coefstr(sb1, f->b1),
                     coefstr(sb2, f->b2));
	}
	fprintf(out, "  };\n\n");
    }
    
    fprintf(out,
	    "  static struct chain baked_chains[] = {\n");

    for (size_t i = 0; i < xo->n_chains; i++) {
	fprintf(out,
		"    { .source = %zu, .n_filters = %zu, .filters = filters_%zu, .limiter = 1 },\n",
		xo->chains[i].source, xo->chains[i].n_filters, i);
    }
    fprintf(out, "  };\n\n");
   
    fprintf(out, "  return xo_config_load_existing(xo, &baked_chains[0], %zu);\n", xo->n_chains);
    fprintf(out, "}\n");

    xo_free(xo);
    
    return 0;
}
