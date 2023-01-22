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

#include "../samples.h"

void msg_init(void);
void xmit_fib_msg(void);

#define AICA_CHAN(chan) ((unsigned volatile*)0x800000 + 0x80 * (chan))

#define PLAY_CTRL 0
#define SAMPLE_ADDR_LOW 1
#define LOOP_START 2
#define LOOP_END 3
#define AMPENV1 4
#define AMPENV2 5
#define SAMPLE_RATE_PITCH 6

#define AICA_SAMPLE_ADDR 4096
#define TONE_FREQ 400
#define N_SAMPLES (44100 / TONE_FREQ)

int arm_main(void) {
    /* msg_init(); */
    /* xmit_fib_msg(); */

    // now let's do some dangerous shit
    AICA_CHAN(0)[LOOP_START] = 0;
    AICA_CHAN(0)[LOOP_END] = N_SAMPLES;
    AICA_CHAN(0)[AMPENV1] = 0x1f; // maximum attack, minimum decay/sustain/release
    AICA_CHAN(0)[AMPENV2] = 0xf << 10; // no key-rate-scaling
    AICA_CHAN(0)[SAMPLE_RATE_PITCH] = 0; // set sample-rate to 44.1 KHz
    AICA_CHAN(0)[SAMPLE_ADDR_LOW] = AICA_SAMPLE_ADDR & 0xffff;

    AICA_CHAN(0)[PLAY_CTRL] = (1 << 14) | (1 << 9) | (AICA_SAMPLE_ADDR >> 16) | (1 << 10);

    ((volatile unsigned char *)(AICA_CHAN(0)+9))[4] = 0x24;
    ((volatile unsigned char *)(AICA_CHAN(0)+9))[1] = 0xf;
    ((volatile unsigned char *)(AICA_CHAN(0)+9))[5] = 0;
    ((volatile unsigned char *)(AICA_CHAN(0)+9))[0] = 0;

    AICA_CHAN(0)[PLAY_CTRL] |= (1 << 15);

    return 1337;
}
