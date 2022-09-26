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

#ifndef PVR_H_
#define PVR_H_

typedef unsigned u32;

/*
 * i *think* these should be all the registers i need to render things on
 * WashingtonDC.
 * For real hardware, I will definitely need many more registers
 */

#define TA_FIFO ((u32 volatile *)0xb0000000)

// determines the display list address executed by STARTRENDER
#define PVR2_PARAM_BASE ((u32 volatile*)0xa05f8020)

/*
 * determines the address where display lists are written by TA
 * this probably does not apply to real hardware, but in WashingtonDC this is
 * the only register the TA cares about.
 */
#define PVR2_TA_VERTBUF_POS ((u32 volatile*)0xa05f8138)

// used for a bunch of things related to polygon sorting and/or transparency
#define PVR2_ISP_FEED_CFG ((u32 volatile*)0xa05f8098)

// used for a bunch of things related to textures
#define PVR2_TEXT_CONTROL ((u32 volatile*)0xa05f80e4)

// bias value for polygon back-side culling
#define PVR2_FPU_CULL_VAL ((u32 volatile*)0xa05f8078)

#define PVR2_TA_RESET ((u32 volatile*)0xa05f8144)

#define PVR2_STARTRENDER ((u32 volatile*)0xa05f8014)

#define PVR2_REG_IDX(addr) ((u32 volatile*)(addr | 0xa0000000))

#define PVR2_FB_R_CTRL PVR2_REG_IDX(0x5f8044)
#define PVR2_FB_W_CTRL PVR2_REG_IDX(0x5f8048)
#define PVR2_FB_W_LINESTRIDE PVR2_REG_IDX(0x5f804c)
#define PVR2_FB_R_SOF1 PVR2_REG_IDX(0x5f8050)
#define PVR2_FB_R_SOF2 PVR2_REG_IDX(0x5f8054)
#define PVR2_FB_R_SIZE PVR2_REG_IDX(0x5f805c)
#define PVR2_FB_W_SOF1 PVR2_REG_IDX(0x5f8060)
#define PVR2_FB_W_SOF2 PVR2_REG_IDX(0x5f8064)
#define PVR2_TA_VERTBUF_START PVR2_REG_IDX(0x5f8128)
#define PVR2_TA_GLOB_TILE_CLIP PVR2_REG_IDX(0x5f813c)

#define PVR2_FB_X_CLIP PVR2_REG_IDX(0x5f8068)
#define PVR2_FB_Y_CLIP PVR2_REG_IDX(0x5f806c)

#define PVR2_ISP_BACKGND_D PVR2_REG_IDX(0x5f8088)
#define PVR2_ISP_BACKGND_T PVR2_REG_IDX(0x5f808c)
#define PVR2_TA_OL_LIMIT PVR2_REG_IDX(0x5f812c)
#define PVR2_TA_ALLOC_CTRL PVR2_REG_IDX(0x5f8140)
#define PVR2_TA_OL_BASE PVR2_REG_IDX(0x5f8124)
#define PVR2_TA_ISP_BASE PVR2_REG_IDX(0x5f8128)
#define PVR2_TA_ISP_LIMIT PVR2_REG_IDX(0x5f8130)
#define PVR2_FPU_PARAM_CFG PVR2_REG_IDX(0x5f807c)

/*
 * polygon header and vertex structs will both be 8 bytes if the following
 * conditions are met:
 *     - poly_type is not modifier volume
 *     - textures are disabled
 *     -     granted, not all texture types need 16 byte verts
 *     - color type is not intensity mode 1
 */
struct poly_header {
    /*
     * PARAMETER CONTROL WORD:
     *     para type (bit 31-29): 4
     *     end-of-strip (bit 28): 0
     *     list-type (bit 26-24): 0 for opaque
     *     group_en (bit 23): 0
     *     unused (bit 22-20): 0
     *     strip_len (bit 19-18): 0
     *     user_clip (bit 17-16): 0
     *     reserved (bit 15-8): 0
     *     shadow (bit 7): 0
     *     volume (bit 6): 0
     *     color type (bit 5-4): 0 for packed color, 1 for floating-point color
     *     texture (bit 3): 1 for enable, 0 for disable
     *     offset (bit 2): 1 for offset color, 0 for no offset color
     *     gourad shading (bit 1): 1 for gourad shading, 0 for flat shading
     *     16bit uv (bit 0): 1 for 16-bit texture coordinates, 0 for 32-bit texture coordinates
     *
     * ISP/TSP INSTRUCTION WORD:
     *     depth compae mode (bit 31-29): 7 for always
     *     culling mode (bit 28-27): 0 for none
     *     z-write-disble (bit 26): 0 to *enable* z-write
     *     texture (bit 25): 0 for none
     *     offset (bit 24): 1 for offset color, 0 for no offset color
     *     gourad shading (bit 23): 1 for gourad shading, 0 for flat shading
     *     16-bit UV (bit 22): 1 for 16-bit texture coordinates, 0 for 32-bit texture coordinates
     *     cache bypass (bit 21): idk, guess 1 seems safer but 0 should be more performant...
     *     dcalc ctrl (bit 20): 1 is better, 0 is faster, either way its
     *                          irrelevant without enabling texturing and mipmaps
     *
     * TSP Instruction Word:
     *     SRC alpha instruction (bit 31-29): 7 for 1-DST_alpha
     *     DST Alpha inst (bit 28-26): 6 for DST_alpha
     *     SRC select (bit 25): 0 for primary color buffer
     *     DST select (bit 24): 0 for primary color buffer
     *     Fog control (bit 23-22): 2 to disable fog
     *     color clamp (bit 21): doesnt matter with fog disabled so make it 0?
     *     use alpha (bit 20): 1 to use alpha, 0 to replace all alpha with 1.0 (no blending)
     *     ignore texture alpha (bit 19): useless without texture so make it 0
     *     flip UV (bit 18-17): 0
     *     clamp UV (bit 16-15): 0
     *     texture filter mode (bit 14-13): 0
     *     super-sample texture (bit 12): 0
     *     mip-map D adjust (bit 11-8): 4
     *     texture/shading instruction (bit 7-6): 0
     *     texture u-size (bit 5-3): 0
     *     texture v-size (bit 2-0): 0
     *
     * Texture control word: it should be safe to just make the whole thing 0
     */

    // parameter contorl word
    // ISP/TSP instruction word
    // TSP instrucion word
    // texture control word
    // ignored
    // ignored
    // ignored unless sort-dma
    // ignored unless sort-dma
};

/*
 * vertex structure:
 * PARAMETER CONTROL WORD:
 *     parameter type (bit 31-29): 7
 *     end-of-strip (bit 28): self-explanatory
 *     unused (bit 27): 0
 *     list type (bit 26-24): N/A, make it 0
 *     group control (bit 23-16): N/A, make it 0
 *     obj control (bit 15-0): N/A, make it 0
 *
 * X-POS: 32-bit float
 * Y-POS: 32-bit float
 * Z-POS: 32-bit float
 * IF PACKED COLOR:
 *     8 UNUSED BYTES
 *     PACKED COLOR AS 32-BIT INT (8bpp)
 *     4 UNUSED BYTES
 * ELIF FLOATING POINT COLOR
 *     ALPHA (32-bit float)
 *     RED (32-bit float)
 *     GREEN (32-bit float)
 *     BLUE (32-bit float)
 */

#endif
