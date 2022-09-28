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

#include "romfont.h"

/* void *get_romfont_pointer(void) { */
/*     static unsigned romfont_fn_ptr = 0x8c0000b4; */
/*     void *romfont_p; */
/*     asm volatile */
/*         ( */
/*          "mov.l @%1, %1\n" */
/*          "jsr @%1\n" */
/*          "xor r1, r1\n" */
/*          "mov r0, %0\n" */
/*          : // output registers */
/*            "=r"(romfont_p) */
/*          : // input registers */
/*            "r"(romfont_fn_ptr) */
/*          : // clobbered registers */
/*            "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "pr"); */
/*     return romfont_p; */
/* } */

// basic framebuffer parameters
#define LINESTRIDE_PIXELS  640
#define BYTES_PER_PIXEL      2
#define FRAMEBUFFER_WIDTH  640
#define FRAMEBUFFER_HEIGHT 476

unsigned short make_color(unsigned red, unsigned green, unsigned blue) {
    if (red > 255)
        red = 255;
    if (green > 255)
        green = 255;
    if (blue > 255)
        blue = 255;

    red >>= 3;
    green >>= 2;
    blue >>= 3;

    return blue | (green << 5) | (red << 11);
}

void
create_font(unsigned short *font,
            unsigned short foreground, unsigned short background) {
    char const *romfont = get_romfont_pointer();

    unsigned glyph;
    for (glyph = 0; glyph < 288; glyph++) {
        unsigned short *glyph_out = font + glyph * 24 * 12;
        char const *glyph_in = romfont + (12 * 24 / 8) * glyph;

        unsigned row, col;
        for (row = 0; row < 24; row++) {
            for (col = 0; col < 12; col++) {
                unsigned idx = row * 12 + col;
                char const *inp = glyph_in + idx / 8;
                char mask = 0x80 >> (idx % 8);
                unsigned short *outp = glyph_out + idx;
                if (*inp & mask)
                    *outp = foreground;
                else
                    *outp = background;
            }
        }
    }
}

void blit_glyph(void volatile *fb, unsigned linestride_pixels,
                unsigned xclip, unsigned yclip,
                unsigned short const *font,
                unsigned glyph_no, unsigned x, unsigned y) {
    if (glyph_no > 287)
        glyph_no = 0;
    unsigned short volatile *outp = ((unsigned short volatile*)fb) +
        y * linestride_pixels + x;
    unsigned short const *glyph = font + glyph_no * 24 * 12;

    unsigned row;
    for (row = 0; row < 24 && (row + y) < yclip; row++) {
        unsigned col;
        for (col = 0; col < 12 && (col + x) < xclip; col++) {
            outp[col] = glyph[row * 12 + col];
        }
        outp += linestride_pixels;
    }
}

void blit_char(void volatile *fb, unsigned linestride_pixels,
               unsigned xclip, unsigned yclip,
               unsigned short const *font,
               char ch, unsigned row, unsigned col) {
    /* if (row >= MAX_CHARS_Y || col >= MAX_CHARS_X) */
    /*     return; */

    unsigned x = col * 12;
    unsigned y = row * 24;

    unsigned glyph;
    if (ch >= 33 && ch <= 126)
        glyph = ch - 33 + 1;
    else
        return;

    blit_glyph(fb, linestride_pixels, xclip, yclip, font, glyph, x, y);
}

void blitstring(void volatile *fb, unsigned linestride_pixels,
                unsigned xclip, unsigned yclip,
                unsigned short const *font, char const *msg,
                unsigned row, unsigned col) {
    while (*msg) {
        /* if (col >= MAX_CHARS_X) { */
        /*     col = 0; */
        /*     row++; */
        /* } */
        if (*msg == '\n') {
            col = 0;
            row++;
            msg++;
            continue;
        }
        blit_char(fb, linestride_pixels, xclip, yclip, font, *msg++, row, col++);
    }
}

static unsigned romfont_strlen(char const *str) {
    unsigned len = 0;
    while (*str++)
        len++;
    return len;
}

void blitstring_centered(void volatile *fb, unsigned linestride_pixels,
                         unsigned xclip, unsigned yclip,
                         unsigned short const *font, char const *msg,
                         unsigned row) {
    unsigned n_columns = xclip / GLYPH_WIDTH;
    unsigned len = romfont_strlen(msg);
    blitstring(fb, linestride_pixels, xclip, yclip, font, msg, row, (n_columns - len) / 2);
}
