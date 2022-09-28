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

#include "pvr.h"
#include "romfont.h"
#include "maple.h"
#include "tmu.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

extern unsigned volatile ticks;

#define MAKE_PHYS(addr) ((void*)((((unsigned)(addr)) & 0x1fffffff) | 0xa0000000))

#define SPG_HBLANK_INT (*(unsigned volatile*)0xa05f80c8)
#define SPG_VBLANK_INT (*(unsigned volatile*)0xa05f80cc)
#define SPG_CONTROL    (*(unsigned volatile*)0xa05f80d0)
#define SPG_HBLANK     (*(unsigned volatile*)0xa05f80d4)
#define SPG_LOAD       (*(unsigned volatile*)0xa05f80d8)
#define SPG_VBLANK     (*(unsigned volatile*)0xa05f80dc)
#define SPG_WIDTH      (*(unsigned volatile*)0xa05f80e0)

#define VO_CONTROL     (*(unsigned volatile*)0xa05f80e8)
#define VO_STARTX      (*(unsigned volatile*)0xa05f80ec)
#define VO_STARTY      (*(unsigned volatile*)0xa05f80f0)

#define FB_R_CTRL      (*(unsigned volatile*)0xa05f8044)
#define FB_R_SOF1      (*(unsigned volatile*)0xa05f8050)
#define FB_R_SOF2      (*(unsigned volatile*)0xa05f8054)
#define FB_R_SIZE      (*(unsigned volatile*)0xa05f805c)

#define ISTNRM (*(unsigned volatile*)0xa05f6900)

// when the opaque polygon list has been successfully input
#define OPAQUE_LIST_COMPLETE_SHIFT 7
#define OPAQUE_LIST_COMPLETE_BIT (1 << OPAQUE_LIST_COMPLETE_SHIFT)

#define PVR_RENDER_COMPLETE_SHIFT 2
#define PVR_RENDER_COMPLETE_BIT (1 << PVR_RENDER_COMPLETE_SHIFT)

// basic framebuffer parameters
#define LINESTRIDE_PIXELS  SCREEN_WIDTH
#define BYTES_PER_PIXEL      2
#define FRAMEBUFFER_WIDTH  SCREEN_WIDTH
#define FRAMEBUFFER_HEIGHT SCREEN_HEIGHT

#define TEXT_COLUMNS (SCREEN_WIDTH / 12)
#define TEXT_ROWS (SCREEN_HEIGHT / 24)

/*
 * vram memory map (32-bit area):
 * background plane: 0x05000000
 * display lists: 0x05100000
 * region array: 0x05101000 - 0x05102c20
 * framebuffer:   0x05200000-0x05296000 (frame1), 0x05300000-0x05396000 (frame 2)
 * OPBs: 0x05400000 onwards
 */

#define FRAMEBUFFER_1 ((void volatile*)0xa5200000)
#define FRAMEBUFFER_2 ((void volatile*)0xa5600000)

#define FB_R_SOF1_FRAME1 0x00200000
#define FB_R_SOF2_FRAME1 0x00200500
#define FB_R_SOF1_FRAME2 0x00600000
#define FB_R_SOF2_FRAME2 0x00600500

#define VRAM_REL(addr) (((unsigned)(addr)) & 0x00ffffff)

void *_get_romfont_pointer(void);

static void volatile *cur_framebuffer;
static void configure_video(void) {
    // Hardcoded for 640x476i NTSC video
    SPG_HBLANK_INT = 0x03450000;
    SPG_VBLANK_INT = 0x00150104;
    SPG_CONTROL = 0x00000150;
    SPG_HBLANK = 0x007E0345;
    SPG_LOAD = 0x020C0359;
    SPG_VBLANK = 0x00240204;
    SPG_WIDTH = 0x07d6c63f;
    VO_CONTROL = 0x00160000;
    VO_STARTX = 0x000000a4;
    VO_STARTY = 0x00120012;
    FB_R_CTRL = 0x00000005;
    FB_R_SOF1 = FB_R_SOF1_FRAME2;
    FB_R_SOF2 = FB_R_SOF2_FRAME2;
    FB_R_SIZE = 0x1413b53f;
    *PVR2_FB_W_CTRL = 1; // RGB565
    *PVR2_FB_W_LINESTRIDE = 10;
    *PVR2_FB_W_SOF1 = FB_R_SOF1_FRAME2;
    *PVR2_FB_W_SOF2 = FB_R_SOF2_FRAME2;
    *PVR2_FB_W_LINESTRIDE = (SCREEN_WIDTH * 2) / 8;
    /* *PVR2_FB_X_CLIP = SCREEN_WIDTH << 16; */
    /* *PVR2_FB_Y_CLIP = SCREEN_HEIGHT << 16; */
    *PVR2_FPU_PARAM_CFG = (1 << 21) | (0x1f << 14) |
        (0x1f << 8) | (7 << 4) | 7;

    cur_framebuffer = FRAMEBUFFER_1;
}

static void clear_screen(void volatile* fb, unsigned short color) {
    unsigned color_2pix = ((unsigned)color) | (((unsigned)color) << 16);

    unsigned volatile *row_ptr = (unsigned volatile*)fb;

    unsigned row, col;
    for (row = 0; row < FRAMEBUFFER_HEIGHT; row++)
        for (col = 0; col < (FRAMEBUFFER_WIDTH / 2); col++)
            *row_ptr++ = color_2pix;
}

static int check_vblank(void) {
    int ret = (ISTNRM & (1 << 3)) ? 1 : 0;
    if (ret)
        ISTNRM = (1 << 3);
    return ret;
}

static void swap_buffers(void) {
    if (cur_framebuffer == FRAMEBUFFER_1) {
        FB_R_SOF1 = FB_R_SOF1_FRAME1;
        FB_R_SOF2 = FB_R_SOF2_FRAME1;
        *PVR2_FB_W_SOF1 = FB_R_SOF1_FRAME1;
        *PVR2_FB_W_SOF2 = FB_R_SOF2_FRAME1;
        cur_framebuffer = FRAMEBUFFER_2;
    } else {
        FB_R_SOF1 = FB_R_SOF1_FRAME2;
        FB_R_SOF2 = FB_R_SOF2_FRAME2;
        *PVR2_FB_W_SOF1 = FB_R_SOF1_FRAME2;
        *PVR2_FB_W_SOF2 = FB_R_SOF2_FRAME2;
        cur_framebuffer = FRAMEBUFFER_1;
    }
}

static char const *hexstr(unsigned val) {
    static char txt[32];
    unsigned nib_no;
    for (nib_no = 0; nib_no < 8; nib_no++) {
        unsigned shift_amt = (7 - nib_no) * 4;
        unsigned nibble = (val >> shift_amt) & 0xf;
        static char const tbl[16] = {
            '0', '1', '2', '3',
            '4', '5', '6', '7',
            '8', '9', 'A', 'B',
            'C', 'D', 'E', 'F'
        };
        txt[nib_no] = tbl[nibble];
    }
    txt[8] = '\0';
    return txt;
}

#define SH4_IPR ((unsigned short volatile*)0xffd00004)

int dcmain(int argc, char **argv) {
    static unsigned short font[288 * 24 * 12];
    create_font(font, make_color(255, 255, 255), make_color(0, 0, 0));
    configure_video();

    // initialize TMU
    *TMU_TSTR = 0; // disable TMU
    *TMU_TCR0 = TMU_TCR_50_KHZ | TMU_TCR_UNIE;
    *TMU_TCOR0 = 49;
    *TMU_TCNT0 = 49;
    SH4_IPR[0] = 0xf000;       // set TMU interrupts to highest priority
    *TMU_TSTR = TMU_TSTR_STR0; //re-enable TMU

    while (!((~get_controller_buttons()) & (1 << 3))) {
        clear_screen(cur_framebuffer, make_color(0, 0, 0));
        blitstring_centered(cur_framebuffer, SCREEN_WIDTH, SCREEN_WIDTH, SCREEN_HEIGHT,
                            font, "i hate my fucking life", 8);
        blitstring_centered(cur_framebuffer, SCREEN_WIDTH, SCREEN_WIDTH, SCREEN_HEIGHT,
                            font, hexstr(ticks/1000), 9);
        while (!check_vblank())
            ;
        swap_buffers();
    }
}
