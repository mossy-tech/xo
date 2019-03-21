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

void xo_filter_calculate_sv(struct filter * f,
        float_type fc,
        float_type q,
        float_type sample_rate,
        enum filter_type type)
{

    float_type g = CAT(tanf, FP)(M_PI * fc / sample_rate);
    float_type k = 1. / q;
    float_type a = 1. / (1. + g * (g + k));

    f->a0 = a;
    f->a1 = g * a;
    f->a2 = g * g * a;

    f->b1 = g;
    f->b2 = k;

    f->z0 = 0.;
    f->z1 = 0.;
    f->type = type;
}


