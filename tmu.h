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

#ifndef TMU_H_
#define TMU_H_

#define TMU_REG(addr) ((unsigned volatile*)addr)

#define TMU_TOCR  ((unsigned char volatile*)0xffd80000)
#define TMU_TSTR  ((unsigned char volatile*)0xffd80004)

/*
 * timer constant registers
 *
 * when a timer's TCNT register underflows, the value in this register is
 * copied to TCNT.
 */
#define TMU_TCOR0 TMU_REG(0xffd80008)
#define TMU_TCOR1 TMU_REG(0xffd80014)
#define TMU_TCOR2 TMU_REG(0xffd80020)

/*
 * timer counters
 *
 * these cownt down based on the clock selected by the TPSC bits in TCR
 *
 * upon underflowing, the UNF flag is set in the corresponding TCR register and
 * TCNT is reset to the value contained in TCOR.
 */
#define TMU_TCNT0 TMU_REG(0xffd8000c)
#define TMU_TCNT1 TMU_REG(0xffd80018)
#define TMU_TCNT2 TMU_REG(0xffd80024)

#define TMU_TCR0  ((unsigned short volatile*)0xffd80010)
#define TMU_TCR1  ((unsigned short volatile*)0xffd8001c)
#define TMU_TCR2  ((unsigned short volatile*)0xffd80028)

#define TMU_TCPR2 TMU_REG(0xffd8002c)

// set these bits to enable the corresponding timer channel
#define TMU_TSTR_STR0 1
#define TMU_TSTR_STR1 2
#define TMU_TSTR_STR2 4

// underflow flag
#define TMU_TCR_UNF (1 << 8)

// underflow interrupt enable
#define TMU_TCR_UNIE (1 << 5)

#define TMU_TCR_CKEG_SHIFT 3
#define TMU_TCR_CKEG_MASK (3 << 3)

#define TMU_TCR_CKEG_RISING_EDGE 0
#define TMU_TCR_CKEG_FALLING_EDGE (1 << TMU_TCR_CKEG_SHIFT)
#define TMU_TCR_CKEG_BOTH_EDGE (2 << TMU_TCR_CKEG_SHIFT)

/*
 * the clock input to the TMU is 50MHz.  It is divided based on the TPSC bits
 * in TCR.  SH4 documentation allows for using an RTC (the sh4 RTC not AICA) or
 * an external clock, but AFAIK neither of these are implemented on Dreamcast
 * (maybe the RTC would work but i've never seen it done that way).  Since the
 * minimum clock division is 4, the fastest clock you can get is 12.5 MHz
 */
#define TMU_TCR_12_5_MHZ 0    // divide by 4
#define TMU_TCR_3_125_MHZ 1   // divide by 16
#define TMU_TCR_800_KHZ 2 // divide by 64
#define TMU_TCR_200_KHZ 3 // divide by 256
#define TMU_TCR_50_KHZ 4 // divide by 1024

#endif
