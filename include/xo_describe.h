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
#ifndef XO_DESCRIBE_H

#include "xo.h"
 
typedef int (*printer_type)(const char * formatter, ...)
    __attribute__((format(printf, 1, 2)));

int xo_describe(struct xo * xo,
        printer_type printer,
        int printer_check_error, int printer_nonzero_is_error);

#endif /* XO_DESCRIBE_H */
