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

#include "maple.h"

#define ISTNRM (*(unsigned volatile*)0xa05f6900)

#define REG_MDSTAR (*(unsigned volatile*)0xa05f6c04)
#define REG_MDTSEL (*(unsigned volatile*)0xa05f6c10)
#define REG_MDEN   (*(unsigned volatile*)0xa05f6c14)
#define REG_MDST   (*(unsigned volatile*)0xa05f6c18)
#define REG_MSYS   (*(unsigned volatile*)0xa05f6c80)
#define REG_MDAPRO (*(unsigned volatile*)0xa05f6c8c)
#define REG_MMSEL  (*(unsigned volatile*)0xa05f6ce8)

#define MAKE_PHYS(addr) ((void*)((((unsigned)(addr)) & 0x1fffffff) | 0xa0000000))

static void volatile *align32(void volatile *inp) {
    unsigned uptr = (unsigned)inp;
    return (void volatile*)(32 * ((uptr + 31) / 32));
}

void wait_maple(void) {
    while (!(ISTNRM & (1 << 12)))
           ;

    // clear the interrupt
    ISTNRM = (1 << 12);
}

int check_controller(void) {
    // clear any pending interrupts (there shouldn't be any but do it anyways)
    ISTNRM |= (1 << 12);

    // disable maple DMA
    REG_MDEN = 0;

    // make sure nothing else is going on
    while (REG_MDST)
        ;

    // 2mpbs transfer, timeout after 1ms
    REG_MSYS = 0xc3500000;

    // trigger via CPU (as opposed to vblank)
    REG_MDTSEL = 0;

    // let it write wherever it wants, i'm not too worried about rogue DMA xfers
    REG_MDAPRO = 0x6155407f;

    // construct packet
    static char volatile devinfo0[1024];
    static unsigned volatile frame[36 + 31];

    unsigned volatile *framep = (unsigned*)MAKE_PHYS(align32(frame));
    char volatile *devinfo0p = (char*)MAKE_PHYS(align32(devinfo0));

    framep[0] = 0x80000000;
    framep[1] = ((unsigned)devinfo0p) & 0x1fffffff;
    framep[2] = 0x2001;

    // set SB_MDSTAR to the address of the packet
    REG_MDSTAR = ((unsigned)framep) & 0x1fffffff;

    // enable maple DMA
    REG_MDEN = 1;

    // begin the transfer
    REG_MDST = 1;

    wait_maple();

    // transfer is now complete, receive data
    if (devinfo0p[0] == 0xff || devinfo0p[4] != 0 || devinfo0p[5] != 0 ||
        devinfo0p[6] != 0 || devinfo0p[7] != 1)
        return 0;

    char const *expect = "Dreamcast Controller         ";
    char const volatile *devname = devinfo0p + 22;

    while (*expect)
        if (*devname++ != *expect++)
            return 0;
    return 1;
}

unsigned get_controller_buttons(void) {
    if (!check_controller())
        return ~0;

    // clear any pending interrupts (there shouldn't be any but do it anyways)
    ISTNRM |= (1 << 12);

    // disable maple DMA
    REG_MDEN = 0;

    // make sure nothing else is going on
    while (REG_MDST)
        ;

    // 2mpbs transfer, timeout after 1ms
    REG_MSYS = 0xc3500000;

    // trigger via CPU (as opposed to vblank)
    REG_MDTSEL = 0;

    // let it write wherever it wants, i'm not too worried about rogue DMA xfers
    REG_MDAPRO = 0x6155407f;

    // construct packet
    static char unsigned volatile cond[1024];
    static unsigned volatile frame[36 + 31];

    unsigned volatile *framep = (unsigned*)MAKE_PHYS(align32(frame));
    char unsigned volatile *condp = (char unsigned*)MAKE_PHYS(align32(cond));

    framep[0] = 0x80000001;
    framep[1] = ((unsigned)condp) & 0x1fffffff;
    framep[2] = 0x01002009;
    framep[3] = 0x01000000;

    // set SB_MDSTAR to the address of the packet
    REG_MDSTAR = ((unsigned)framep) & 0x1fffffff;

    // enable maple DMA
    REG_MDEN = 1;

    // begin the transfer
    REG_MDST = 1;

    wait_maple();

    // transfer is now complete, receive data
    return ((unsigned)condp[8]) | (((unsigned)condp[9]) << 8);
}
