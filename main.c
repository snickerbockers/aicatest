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
#include "memory.h"

#ifndef NULL
#define NULL 0x0
#endif

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

static char const *itoa(int val) {
    static char buf[32];

    int idx;
    for (idx = 0; idx < 32; idx++)
        buf[idx] = '\0';

    int buflen = 0;
    if (val < 0) {
        buf[0] = '-';
        buflen++;
        val = -val;
    } else if (val == 0) {
        return "0";
    }

    int leading = 1;
    val &= 0xffffffff; // in case sizeof(int) > 4
    int power = 1000000000; // billion
    while (power > 0) {
        if (buflen >= sizeof(buf)) {
            buf[sizeof(buf) - 1] = '\0';
            break;
        }
        int digit = val / power;
        if (!(digit == 0 && leading))
            buf[buflen++] = digit + '0';
        if (digit != 0)
            leading = 0;
        val = val % power;
        power /= 10;
    }

    return buf;
}

#define SH4_IPR ((unsigned short volatile*)0xffd00004)

#define AICA_CHAN(chan) ((unsigned volatile*)(0xa0700000 + 0x80 * (chan)))

#define PLAY_CTRL 0
#define SAMPLE_ADDR_LOW 1
#define LOOP_START 2
#define LOOP_END 3
#define AMPENV1 4
#define AMPENV2 5
#define SAMPLE_RATE_PITCH 6
#define DIRECT_PAN_VOL_SEND 9
#define LPF1_VOLUME 10

#define AICA_SAMPLE_ADDR 0x11000
#define SH4_SAMPLE_ADDR (AICA_SAMPLE_ADDR + 0xa0800000)

#define FFST (*(unsigned volatile*)0xa05f688c)

/*
SH4 write 4 bytes to a0700000 <CHAN_0_PLAY_CONTROL> in units of 4
	00008000
SH4 write 4 bytes to a0700014 <CHAN_0_AMP_ENV_2> in units of 4
	0000001f
SH4 write 4 bytes to a0700000 <CHAN_0_PLAY_CONTROL> in units of 4
	00008000
SH4 write 4 bytes to a0700014 <CHAN_0_AMP_ENV_2> in units of 4
	0000001f
SH4 write 4 bytes to a0700000 <CHAN_0_PLAY_CONTROL> in units of 4
	00008000
SH4 write 4 bytes to a0700014 <CHAN_0_AMP_ENV_2> in units of 4
	0000001f
SH4 write 4 bytes to a0700000 <CHAN_0_PLAY_CONTROL> in units of 4
	00008000
SH4 write 4 bytes to a0700014 <CHAN_0_AMP_ENV_2> in units of 4
	0000001f
SH4 write 4096 bytes to a0811000 <AICA MEMORY> in units of 4
SH4 write 4 bytes to a0700000 <CHAN_0_PLAY_CONTROL> in units of 4
	00008000
SH4 write 8 bytes to a0700008 <CHAN_0_LOOP_START> in units of 4
	00000000
	00001000
SH4 write 4 bytes to a0700018 <CHAN_0_SAMPLE_RATE_PITCH> in units of 4
	00000000
SH4 write 8 bytes to a0700024 <CHAN_0_DIRECT_PAN_VOL_SEND> in units of 4
	00000f00
	00000024
SH4 write 4 bytes to a0700010 <CHAN_0_AMP_ENV_1> in units of 4
	0000001f
SH4 write 4 bytes to a0700004 <CHAN_0_SAMPLE_ADDR_LOW> in units of 4
	00001000
SH4 write 4 bytes to a0700000 <CHAN_0_PLAY_CONTROL> in units of 4
	0000c201
*/

static unsigned master_vol = 15;

#define AICA_SYS_REG(offs) (*((unsigned volatile*)(0xa0700000+(offs))))

#define MASTER_VOL AICA_SYS_REG(0x2800)
#define CHAN_INFO_REQ AICA_SYS_REG(0x280c)
#define PLAY_STATUS AICA_SYS_REG(0x2810)

#define SUSTAIN_RATE 0
#define DECAY_RATE 1
#define ATTACK_RATE 2
#define RELEASE_RATE 3
#define DECAY_LEVEL 4
#define N_RATES 5


void playtone(unsigned const rates[N_RATES]) {
#define TONE_FREQ 400
#define N_SAMPLES (44100 / TONE_FREQ)
#include "samples.h"

    // set mastervolume
    MASTER_VOL = master_vol <= 15 ? master_vol : 15;

    static unsigned short volatile *samplep = (unsigned short volatile*)SH4_SAMPLE_ADDR;
    memcpy(samplep, samples, sizeof(samples));

    /* for (unsigned sampleno = 0; sampleno < N_SAMPLES; sampleno++) { */
    /*     samplep[sampleno] = 0x7fff * sin(sampleno * 2.0f * 3.1416f / N_SAMPLES); */
    /* } */

    AICA_CHAN(0)[PLAY_CTRL] = 0x8000;
    AICA_CHAN(0)[LOOP_START] = 0;
    AICA_CHAN(0)[LOOP_END] = N_SAMPLES;
    AICA_CHAN(0)[SAMPLE_RATE_PITCH] = 0; // set sample-rate to 44.1 KHz
    AICA_CHAN(0)[DIRECT_PAN_VOL_SEND] = 0xf00;
    AICA_CHAN(0)[LPF1_VOLUME] = 0x24;
    AICA_CHAN(0)[SAMPLE_ADDR_LOW] = AICA_SAMPLE_ADDR & 0xffff;

    AICA_CHAN(0)[AMPENV1] = rates[2] | (rates[1] << 6) | (rates[0] << 11);
    AICA_CHAN(0)[AMPENV2] = rates[3] | (rates[4] << 5) | (0xf << 10);

    AICA_CHAN(0)[PLAY_CTRL] = 0xc201;

    /* AICA_CHAN(0)[LOOP_START] = 0; */
    /* AICA_CHAN(0)[LOOP_END] = N_SAMPLES; */
    /* AICA_CHAN(0)[AMPENV1] = 0x1f; // maximum attack, minimum decay/sustain/release */
    /* AICA_CHAN(0)[AMPENV2] = 0xf << 10; // no key-rate-scaling */
    /* AICA_CHAN(0)[SAMPLE_RATE_PITCH] = 0; // set sample-rate to 44.1 KHz */
    /* AICA_CHAN(0)[SAMPLE_ADDR_LOW] = AICA_SAMPLE_ADDR & 0xffff; */

    /* AICA_CHAN(0)[PLAY_CTRL] = (1 << 14) | (1 << 9) | (AICA_SAMPLE_ADDR >> 16); */

    /* ((volatile unsigned char *)(AICA_CHAN(0)+9))[4] = 0x24; */
    /* ((volatile unsigned char *)(AICA_CHAN(0)+9))[1] = 0xf; */
    /* ((volatile unsigned char *)(AICA_CHAN(0)+9))[5] = 0; */
    /* ((volatile unsigned char *)(AICA_CHAN(0)+9))[0] = 0; */

    /* AICA_CHAN(0)[PLAY_CTRL] |= (1 << 15); */
}

void silence(void) {
    unsigned chan;
    for (chan = 0; chan < 64; chan++)
        AICA_CHAN(chan)[PLAY_CTRL] = 0x8000;
}

#define ARM7_RESET (*(volatile unsigned*)0xa0702c00)

static void disable_arm(void) {
    while (FFST & 0x10)
        ;
    ARM7_RESET |=1;
}

static void enable_arm(void) {
    while (FFST & 0x10)
        ;
    ARM7_RESET &= ~1;
}

static void
get_chan_state(unsigned ch, unsigned *statep,
               unsigned *attenp, unsigned *loopp) {
    CHAN_INFO_REQ = ch << 8;
    unsigned playstat = PLAY_STATUS;

    if (statep)
        *statep = (playstat >> 13) & 3;

    if (attenp)
        *attenp = playstat & 0x1fff;

    if (loopp)
        *loopp = (playstat >> 15) & 1;
}

#include "arm7_fw.h"

int dcmain(int argc, char **argv) {
    unsigned rates[5] = {
        0, 0, 0x1f, 0, 0
    };

    char const *rate_names[5] = {
        "sustain rate",
        "decay rate",
        "attack rate",
        "release rate",
        "decay level"
    };

    unsigned cursor = 0;

    disable_arm();
    *((unsigned volatile*)0xa0800000) = 0xeafffffe;
    /* memcpy((unsigned volatile*)0xa0800000, arm7fw, sizeof(arm7fw)); */

    // awaken ARM7
    enable_arm();

    static unsigned short font[2][288 * 24 * 12];
    create_font(font[0], make_color(255, 255, 255), make_color(0, 0, 0));
    create_font(font[1], ~0, make_color(0, 0, 255));
    configure_video();

    // initialize TMU
    *TMU_TSTR = 0; // disable TMU
    *TMU_TCR0 = TMU_TCR_50_KHZ | TMU_TCR_UNIE;
    *TMU_TCOR0 = 49;
    *TMU_TCNT0 = 49;
    SH4_IPR[0] = 0xf000;       // set TMU interrupts to highest priority
    /* *TMU_TSTR = TMU_TSTR_STR0; //re-enable TMU */

    int btns, btns_prev = 0;
    int paused = 1;
    while (!((btns = ~get_controller_buttons()) & (1 << 3))) {
        if (btns & (1 << 2) && !(btns_prev & (1 << 2))) {
            // A button
            
            /* *TMU_TSTR ^= TMU_TSTR_STR0; */
            /* paused = !paused; */
            silence();
            playtone(rates);
        }

        if ((btns & (1 << 7)) && !(btns_prev & (1 << 7))) {
            // right
            if (rates[cursor] < 0x1f)
                rates[cursor]++;
        }
        if ((btns & (1 << 6)) && !(btns_prev & (1 << 6))) {
            // left
            if (rates[cursor] > 0)
                rates[cursor]--;
        }

        if ((btns & (1 << 4)) && !(btns_prev & (1 << 4))) {
            // up
            if (cursor > 0)
                cursor--;
        }
        if ((btns & (1 << 5)) && !(btns_prev & (1 << 5))) {
            // down
            if (cursor < 4)
                cursor++;
        }

        if (btns & (1 << 1) && !(btns_prev & (1 << 1))) {
            // B button
            silence();
        }

        /* if (btns & (1 << 1) && !(btns_prev & (1 << 1))) { */
        /*     // B button - reset the timer and pause */
        /*     paused = 1; */
        /*     *TMU_TSTR = 0; */
        /*     *TMU_TCOR0 = 49; */
        /*     *TMU_TCNT0 = 49; */
        /*     ticks = 0; */
        /* } */

        clear_screen(cur_framebuffer, make_color(0, 0, 0));
        blitstring_centered(cur_framebuffer, SCREEN_WIDTH, SCREEN_WIDTH, SCREEN_HEIGHT,
                            font[0], "WELCOME TO AICA TEST", 3);
        unsigned row = 4, col = 0;

        unsigned atten;
        get_chan_state(0, NULL, &atten, NULL);
        printstr(cur_framebuffer, font[0], "attenuation: ", &row, &col);
        printstr(cur_framebuffer, font[0], hexstr(atten), &row, &col);

        unsigned rateno;
        for (rateno = 0; rateno < 5; rateno++) {
            unsigned short *cur_font = (rateno == cursor ? font[1] : font[0]);
            row++;
            col = 4;
            printstr(cur_framebuffer, font[0], rate_names[rateno], &row, &col);
            printstr(cur_framebuffer, font[0], ": ", &row, &col);
            col = MAX_CHARS_X - 12;
            printstr(cur_framebuffer, cur_font, hexstr(rates[rateno]), &row, &col);
        }

        /* printstr(cur_framebuffer, font, "MASTER VOLUME: ", &row, &col); */
        /* printstr(cur_framebuffer, font, hexstr(master_vol), &row, &col); */


        while (!check_vblank())
            ;
        swap_buffers();
        btns_prev = btns;
    }
    silence();
    return 0;
}
