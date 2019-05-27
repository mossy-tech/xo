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

void xo_filter_calculate_sv(struct filter * f,
        float_type fc,
        float_type q,
        float_type sample_rate,
        enum filter_type type)
{

    /*
    float_type g = CAT(tanf, FP)(M_PI * fc / sample_rate);
    float_type k = 1. / q;
    float_type a = 1. / (1. + g * (g + k));

    f->a0 = a;
    f->a1 = g * a;
    f->a2 = g * g * a;

    f->b1 = g;
    f->b2 = k;
    */

    /*
    f->a0 = 2 * CAT(sinf, FP)(M_PI * fc / sample_rate);
    f->a1 = 1. / q;

    f->type = type;
    */
}

#include <stdlib.h>

void xo_correct(struct xo * xo, float_type sample_rate)
{
    for (size_t i = 0; i < xo->n_chains; i++) {
        struct chain * chain = &xo->chains[i];
        for (size_t j = 0; j < chain->n_filters; j++) {
            struct filter * filter = &chain->filters[j];

            if (!filter->z0 && filter->order)
                filter->z0 = malloc(sizeof(float_type) * filter->order);

            if (!filter->z1 && filter->order)
                filter->z1 = malloc(sizeof(float_type) * filter->order);

            for (size_t i = 0; i < filter->order; i++) {
                filter->z0[i] = 0.;
                filter->z1[i] = 0.;
            }

            if (filter->type == XO_FILTER_SV_HP ||
                filter->type == XO_FILTER_SV_LP) {
//                fprintf(stderr, "adjust\n");

                if (filter->a0 > CAT(M_PIf, FP)) {
                    filter->a0 = 2 * CAT(sinf, FP)(
                            CAT(M_PIf, FP) * filter->a0 / (sample_rate * filter->over));

                    filter->a1 = 1. / filter->a1;
                }
            }
        }
    }
//    xo_describe(xo, printf, 1, 0);
}

