/*******************************************************************************
 *
 *
 *    Copyright (C) 2022 snickerbockers
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 ******************************************************************************/

#ifndef MEMORY_H_
#define MEMORY_H_

typedef unsigned size_t;

static int memcmp(void const *s1, void const *s2, size_t n_bytes) {
    unsigned char const *lhs = s1;
    unsigned char const *rhs = s2;

    while (n_bytes--)
        if (*lhs < *rhs) {
            return -1;
        } else if (*lhs > *rhs) {
            return 1;
        } else {
            lhs++;
            rhs++;
        }
    return 0;
}

static void *memcpy(void volatile *dst, void const *src, size_t n) {
    char volatile *outp = dst;
    char const *inp = src;
    while (n--)
        *outp++ = *inp++;
}

#endif
