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

#include <stdlib.h>

struct xo * xo_alloc()
{
    struct xo * xo = malloc(sizeof(*xo));
    *xo = (struct xo) {
        .chains = NULL,
        .n_chains = 0
    };
    return xo;
}

void xo_free(struct xo * xo)
{
    for (size_t c = 0; c < xo->n_chains; c++) {
        if (xo->chains[c].n_filters) {
            free(xo->chains[c].filters);
        }
    }
    if (xo->n_chains) {
        free(xo->chains);
    }
    free(xo);
}

struct xo * xo_from(struct xo * from)
{
    struct xo * to = xo_alloc();

    *to = (struct xo) {
	.n_chains = from->n_chains,
	.chains = malloc(sizeof(*to->chains) * from->n_chains)
    };

    for (size_t i = 0; i < from->n_chains; i++) {
        struct chain * to_c = &to->chains[i];
        struct chain * from_c = &from->chains[i];
	*to_c = (struct chain) {
	    .limiter = 1.,
	    .source = from_c->source,
	    .n_filters = from_c->n_filters,
	    .filters = malloc(sizeof(to_c->filters) * from_c->n_filters)
	};

	for (size_t j = 0; j < from->chains[i].n_filters; j++) {
	    to->chains[i].filters[j] = from->chains[i].filters[j];
	    to->chains[i].filters[j].z0 = 0.;
	    to->chains[i].filters[j].z1 = 0.;
	}
    }
    return to;
}

void xo_add_chain(struct xo * xo, size_t input)
{
    xo->chains = realloc(xo->chains,
            sizeof(*xo->chains) * (xo->n_chains + 1));
    xo->chains[xo->n_chains] = (struct chain) {
        .source = input,
        .filters = NULL,
        .limiter = 1.,
        .n_filters = 0
    };
    xo->n_chains++;
}

struct filter * xo_add_filter_to_chain(struct xo * xo)
{
    struct chain * c = &xo->chains[xo->n_chains - 1];
    c->filters = realloc(c->filters, sizeof(*c->filters) * (c->n_filters + 1));
    c->filters[c->n_filters] = (struct filter){ 0 };
    /*
    c->filters[c->n_filters] = (struct filter) {
        .a0 = a0,
        .a1 = a1,
        .a2 = a2,
        .b1 = b1,
        .b2 = b2,
        .z0 = 0.,
        .z1 = 0.
    };
    */
    return &c->filters[c->n_filters++];
}

void xo_filter_set(struct filter * f,
        float_type a0, float_type a1, float_type a2,
        float_type b1, float_type b2,
        enum filter_type type)
{
    *f = (struct filter) {
        .a0 = a0,
        .a1 = a1,
        .a2 = a2,
        .b1 = b1,
        .b2 = b2,
        .z0 = 0.,
        .z1 = 0.,
        .type = type
    };
}

void xo_reset(struct xo * xo)
{
    for (size_t i = 0; i < xo->n_chains; i++) {
	struct chain * chain = &xo->chains[i];
	for (size_t j = 0; j < chain->n_filters; j++) {
	    struct filter * filter = &chain->filters[j];
	    filter->z0 = 0.;
	    filter->z1 = 0.;
	}
    }
}

static float_type step_biquad(struct filter * f, float_type d)
{
    float_type y;
    y = f->z0 + d * f->a0;
    f->z0 = f->z1 + d * f->a1 - y * f->b1;
    f->z1 = d * f->a2 - y * f->b2;
    return y;
}

static float_type step_sv_lp(struct filter * f, float_type d)
{
    float_type v3 = d - f->z1;
    float_type v1 = f->a0 * f->z0 + f->a1 * v3;
    float_type v2 = f->z1 + f->a1 * f->z0 + f->a2 * v3;

    f->z0 = 2. * v1 - f->z0;
    f->z1 = 2. * v2 - f->z1;

    return v2;
}

static float_type step_sv_hp(struct filter * f, float_type d)
{
    float_type v3 = d - f->z1;
    float_type v1 = f->a0 * f->z0 + f->a1 * v3;
    float_type v2 = f->z1 + f->a1 * f->z0 + f->a2 * v3;

    f->z0 = 2. * v1 - f->z0;
    f->z1 = 2. * v2 - f->z1;

    return d - f->b2 * v1 - v2;
}

void xo_process_chain(struct xo * xo,
        input_type * din_0,
        input_type * din_1,
        size_t n_din,
        output_type ** dout)
{
    for (size_t n = 0; n < n_din; n++) {
	float_type din[3] = {
	    din_0[n],
	    din_1[n],
	    din_0[n] + din_1[n]
	};

	for (size_t cn = 0; cn < xo->n_chains; cn++) {
	    if (!dout[cn]) {
		continue;
	    }
	    struct chain * c = &xo->chains[cn];
	    float_type d = din[c->source];

	    for (size_t fn = 0; fn < c->n_filters; fn++) {
		struct filter * f = &c->filters[fn];
                switch (f->type) {
                    case XO_FILTER_BQ:
                        d = step_biquad(f, d);
                        break;
                    case XO_FILTER_SV_LP:
                        d = step_sv_lp(f, d);
                        break;
                    case XO_FILTER_SV_HP:
                        d = step_sv_hp(f, d);
                        break;
                    default:
                        fprintf(stderr, "uh oh!\n");
                        break;
                }
	    }

#if LIMITER
	    d *= c->limiter;
	    if (CAT(fabsf, FP)(d) > 1.) {
		c->limiter /= CAT(fabsf, FP)(d);
	    }
#endif
	    dout[cn][n] = (output_type)d;
	}
    }
}

void xo_filter_unity(struct filter * f){
    *f = (struct filter) {
        .a0 = 1.,
        .a1 = 0.,
        .a2 = 0.,
        .b1 = 0.,
        .b2 = 0.,
        .z0 = 0.,
        .z1 = 0.,
        .type = XO_FILTER_BQ
    };
}

