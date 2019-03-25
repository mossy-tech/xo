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
#ifndef XO_H
#define XO_H

#define _GNU_SOURCE

#include <stddef.h>
#include <stdio.h>
#include <math.h>

#ifndef FP
#error "Must #define FP"
#endif /* FP */

#ifndef LIMITER
#define LIMITER 0
#endif /* LIMITER */

#ifndef SYNC_LIMITER
#define SYNC_LIMITER 0
#endif /* SYNC_LIMITER */

#ifndef BAKED_CHAINS
#define BAKED_CHAINS 0
#endif /* BAKED_CHAINS */

#define XO_LEFT (size_t)0
#define XO_RIGHT (size_t)1
#define XO_MONO (size_t)2
#define XO_SOURCE_MAX (int)XO_MONO

typedef float input_type;
typedef float output_type;

#define __FLOAT_NAME(x) "FP-" # x
#define _FLOAT_NAME(x) __FLOAT_NAME(x)
#define FLOAT_NAME _FLOAT_NAME(FP)

#define _CAT(x, y) x ## y
#define CAT(x, y) _CAT(x, y)
typedef CAT(_Float, FP) float_type;

struct filter {
    float_type a0, a1, a2, b1, b2;
    float_type z0, z1;
    enum filter_type {
        XO_FILTER_BQ,
        XO_FILTER_SV_LP,
        XO_FILTER_SV_HP
    } type;
};

struct chain {
    float_type limiter;
    size_t source;
    size_t n_filters;
    struct filter * filters;
};

struct xo {
    struct chain * chains;
    size_t n_chains;
};

/* xo.c */
struct xo * xo_alloc();

void xo_free(struct xo * xo);

struct xo * xo_from(struct xo * from);

void xo_add_chain(struct xo * xo,
        size_t input);

struct filter * xo_add_filter_to_chain(struct xo * xo);

void xo_reset(struct xo * xo);

void xo_process_chain(struct xo * xo,
        input_type * din_0, input_type * din_1, size_t n_din,
        output_type ** dout);

void xo_filter_set(struct filter * f,
        float_type a0, float_type a1, float_type a2,
        float_type b1, float_type b2,
        enum filter_type type);

void xo_filter_unity(struct filter * f);

/* sv.c */
void xo_filter_calculate_sv(struct filter * f,
        float_type fc,
        float_type q,
        float_type sample_rate,
        enum filter_type type);

/* config_reader.c */
FILE * xo_config_find(const char * const * where);

struct xo * xo_config_load_file(struct xo * xo,
        FILE * fcfg);

struct xo * xo_config_load_existing(struct xo * xo,
        struct chain * chains, size_t n_chains);

#endif /* XO_H */
