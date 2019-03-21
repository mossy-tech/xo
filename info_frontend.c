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
#include "xo_describe.h"

#include <string.h>

int main(int argc, char ** argv) {
    
    if (argc < 2) {
        fprintf(stderr, "Syntax: %s CONFIG [...]\n", argv[0]);
        return 1;
    }

    struct xo * xo = NULL;
    
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
    }

    xo_describe(xo, printf, 1, 0);

    xo_free(xo);
    
    return 0;
}
