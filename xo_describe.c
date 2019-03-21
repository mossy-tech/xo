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

#include <stdlib.h>

#define PRINTER_CHECK(ret) (printer_check_error && ((printer_nonzero_is_error && !ret) || (printer_nonzero_is_error && ret)))
#define PRINTER_AND_CHECK(fmt, ...) do { int ret = printer(fmt __VA_OPT__(,) __VA_ARGS__); if (PRINTER_CHECK(ret)) { return ret; } } while (0)
#define PRINTER_EXITOK return !printer_nonzero_is_error
#define TAB1 "  "
#define TAB2 TAB1 TAB1
#define TAB3 TAB2 TAB1
#define TAB4 TAB2 TAB2
#define TAB5 TAB2 TAB2 TAB1

static const char * coefstr(float_type coef) {
    static char s[256];
    CAT(strfromf, FP)(s, sizeof(s), "%A", coef);
    return s;
}


#ifndef NO_DESCRIBE
int xo_describe(struct xo * xo, printer_type printer, int printer_check_error, int printer_nonzero_is_error) {
    if (!xo) {
        int ret = printer("xo = (nil)\n");
        if (PRINTER_CHECK(ret)) {
            return ret;
        }
        PRINTER_EXITOK;
    }


    PRINTER_AND_CHECK("xo = {\n");

    if (xo->n_chains == 0) {
        PRINTER_AND_CHECK(TAB1 "chains[]: (empty)\n");
    } else {
        if (xo->chains) {
            PRINTER_AND_CHECK(TAB1 "chains[%zu]:\n", xo->n_chains);

            for (size_t i = 0; i < xo->n_chains; i++) {
                PRINTER_AND_CHECK(TAB2 "[%zu] = {\n", i);
#if LIMITER
                PRINTER_AND_CHECK(TAB3 "limiter = %g\n", (double)xo->chains[i].limiter);
#else
                PRINTER_AND_CHECK(TAB3 "limiter = (disabled)\n");
#endif
                switch (xo->chains[i].source) {
                    case XO_LEFT:
                        PRINTER_AND_CHECK(TAB3 "source = XO_LEFT\n");
                        break;
                    case XO_RIGHT:
                        PRINTER_AND_CHECK(TAB3 "source = XO_RIGHT\n");
                        break;
                    case XO_MONO:
                        PRINTER_AND_CHECK(TAB3 "source = XO_MONO\n");
                        break;
                    default:
                        PRINTER_AND_CHECK(TAB3 "source = %zu [INVALID!]\n", xo->chains[i].source);
                        break;
                }

                if (xo->chains[i].n_filters == 0) {
                    PRINTER_AND_CHECK(TAB3 "filters[]: (empty)\n");
                } else {
                    if (xo->chains[i].filters) {
                        PRINTER_AND_CHECK(TAB3 "filters[%zu]:\n", xo->chains[i].n_filters);
                        for (size_t j = 0; j < xo->chains[i].n_filters; j++) {
                            PRINTER_AND_CHECK(TAB4 "[%zu] = {\n", j);
                            PRINTER_AND_CHECK(TAB5 "A0 = %s\n", coefstr(xo->chains[i].filters[j].a0));
                            PRINTER_AND_CHECK(TAB5 "A1 = %s\n", coefstr(xo->chains[i].filters[j].a1));
                            PRINTER_AND_CHECK(TAB5 "A2 = %s\n", coefstr(xo->chains[i].filters[j].a2));
                            PRINTER_AND_CHECK(TAB5 "B1 = %s\n", coefstr(xo->chains[i].filters[j].b1));
                            PRINTER_AND_CHECK(TAB5 "B2 = %s\n", coefstr(xo->chains[i].filters[j].b2));

                            /* end of filter */
                            PRINTER_AND_CHECK(TAB4 "}\n");
                        }
                        /* end of all filters */
                    } else {
                        PRINTER_AND_CHECK(TAB3 "filters[%zu] = (nil) [INVALID!]\n", xo->chains[i].n_filters);
                    }
                }

                /* end of chain */
                PRINTER_AND_CHECK(TAB2 "}\n");
            }
            /* end of all chains */
        } else {
            PRINTER_AND_CHECK(TAB1 "chains[%zu] = (nil) [INVALID!]\n", xo->n_chains);
        }
    }

    /* end of xo */
    PRINTER_AND_CHECK("}\n");

    PRINTER_EXITOK;
}
#else
int xo_describe(struct xo * xo, printer_type printer, int printer_check_error, int printer_nonzero_is_error)
{
    (void)xo;
    (void)printer;
    (void)printer_check_error;
    (void)printer_nonzero_is_error;
    return 0;
}
#endif
