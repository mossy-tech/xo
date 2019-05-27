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
#ifndef COLOR_H
#define COLOR_H

#include <stdbool.h>
#include <stdio.h>

extern bool col_err, col_out;
static bool use_color;

#define COLOR(f) (f == stderr ? col_err : col_out)

#define PRINT(f, args...) \
    do { use_color = COLOR(f); fprintf(f, args); } while (0)

const char * c_off()
{
    if (use_color) return "\x1B[0m";
    return "";
}

const char * c_info()
{
    if (use_color) return "\x1B[30;1m";
    return "";
}

const char * c_err()
{
    if (use_color) return "\x1B[31m";
    return "";
}

const char * c_ok()
{
    if (use_color) return "\x1B[34m";
    return "";
}

#endif /* COLOR_H */
