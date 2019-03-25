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
#ifndef LEX_H
#define LEX_H

#include <stddef.h>

struct token {
    const char * start;
    size_t length;
};

#include "cmd.gen.h"

#endif /* LEX_H */
