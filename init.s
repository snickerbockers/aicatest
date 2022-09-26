!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
!
!    Copyright (C) 2022 snickerbockers
!
!    This program is free software: you can redistribute it and/or modify
!    it under the terms of the GNU General Public License as published by
!    the Free Software Foundation, either version 3 of the License, or
!    (at your option) any later version.
!
!    This program is distributed in the hope that it will be useful,
!    but WITHOUT ANY WARRANTY; without even the implied warranty of
!    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
!    GNU General Public License for more details.
!
!    You should have received a copy of the GNU General Public License
!    along with this program.  If not, see <http://www.gnu.org/licenses/>.
!
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

.globl _start
.globl _get_romfont_pointer
.globl _have_dcload
.globl _ticks

.text

! gcc's sh calling convention (I'm sticking to this even though I don't use C)
! r0 - return value, not preseved
! r1 - not preserved
! r2 - not preserved
! r3 - not preserved
! r4 - parameter 0, not preserved
! r5 - parameter 1, not preserved
! r6 - parameter 2, not preserved
! r7 - parameter 3, not preserved
! r8 - preserved
! r9 - preserved
! r10 - preserved
! r11 - preserved
! r12 - preserved
! r13 - preserved
! r14 - base pointer, preserved
! r15 - stack pointer, preserved

_start:
    ! save nonvolatile registers
    mova after_orig_sr, r0
    stc.l sr, @-r0
    stc.l vbr, @-r0
    sts.l pr, @-r0
    mov.l r15, @-r0
    mov.l r14, @-r0
    mov.l r13, @-r0
    mov.l r12, @-r0
    mov.l r11, @-r0
    mov.l r10, @-r0
    mov.l r9, @-r0
    mov.l r8, @-r0

	! put a pointer to the top of the stack in r15
	mov.l stack_top_addr, r15

    ! get a cache-free pointer to configure_cache (OR with 0xa0000000)
	mova configure_cache, r0
	mov.l nocache_ptr_mask, r2
	mov.l nocache_ptr_val, r1
	and r2, r0
	or r1, r0
	jsr @r0
	nop

    ! check for dcload
    mov.l dcload_magic_addr, r0
    mov.l dcload_magic_value, r1
    mov.l @r0, r0
    cmp/eq r0, r1
    bf done_checking_for_dcload
    mov #1, r1
    mova _have_dcload, r0
    mov.l r1, @r0
done_checking_for_dcload:

    ! point VBR to our exception handling code
    mov.l vbr_val, r0
    ldc r0, vbr

    ! set IMASK = 0
    stc sr, r0
    mov #0xf, r1
    shll2 r1
    shll2 r1
    not r1, r1
    and r1, r0

    ! set BL = 0
    mov.l bl_mask, r1
    not r1, r1
    and r1, r0

    ! commit above changes to SR
    ldc r0, sr

    ! call main
    mov.l main_addr, r0
	jsr @r0
	nop

after_main:
    ! restore registers
    mov.l _have_dcload, r0
    tst #1, r0
    bt exit_no_dcload

    ! do a dc-load exit
    mov.l dcload_magic_addr, r0
    mov.l @(4, r0), r0
    jsr @r0
    mov #15, r4

exit_no_dcload:
    ! restore original register state
    mova orig_r8, r0
    mov.l @r0+, r8
    mov.l @r0+, r9
    mov.l @r0+, r10
    mov.l @r0+, r11
    mov.l @r0+, r12
    mov.l @r0+, r13
    mov.l @r0+, r14
    mov.l @r0+, r15
    lds.l @r0+, pr
    ldc.l @r0+, vbr
    rts
    nop

    .align 4
ticksp:
    .long _ticks
bl_mask:
    .long 1 << 28
vbr_val:
    .long vbr_addr
orig_r8:
    .long 0
orig_r9:
    .long 0
orig_r10:
    .long 0
orig_r11:
    .long 0
orig_r12:
    .long 0
orig_r13:
    .long 0
orig_r14:
    .long 0
orig_r15:
    .long 0
orig_pr:
    .long 0
orig_vbr:
    .long 0
orig_sr:
    .long 0
after_orig_sr:
dcload_magic_addr:
    .long 0x8c004004
dcload_magic_value:
    .long 0xdeadbeef
_have_dcload:
    .long 0
configure_cache:
	mov.l ccr_addr, r0
	mov.l ccr_val, r1
	mov.l r1, @r0
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	rts
	nop

	.align 4
ccr_addr:
	.long 0xff00001c
ccr_val:
	! enable caching
	.long 0x090d

	.align 4
main_addr:
	.long _dcmain
nocache_ptr_mask:
	.long 0x1fffffff
nocache_ptr_val:
	.long 0xa0000000

	.align 4
_get_romfont_pointer:
	mov.l romfont_fn_ptr, r1
	mov.l @r1, r1

	! TODO: not sure if all these registers here need to be saved, or if the
	! bios function can be counted on to do the honorable thing.
	! I do know that at the very least pr needs to be saved since the jsr
	! instruction will overwrite it
	mov.l r8, @-r15
	mov.l r9, @-r15
	mov.l r10, @-r15
	mov.l r11, @-r15
	mov.l r12, @-r15
	mov.l r13, @-r15
	mov.l r14, @-r15
	sts.l pr, @-r15
	mova stack_ptr_bkup, r0
	mov.l r15, @r0

	jsr @r1
	xor r1, r1

	mov.l stack_ptr_bkup, r15
	lds.l @r15+, pr
	mov.l @r15+, r14
	mov.l @r15+, r13
	mov.l @r15+, r12
	mov.l @r15+, r11
	mov.l @r15+, r10
	mov.l @r15+, r9
	rts
	mov.l @r15+, r8

	.align 4
stack_ptr_bkup:
	.long 0
romfont_fn_ptr:
	.long 0x8c0000b4

stack_top_addr:
	.long stack_top

! 4 kilobyte stack
	.align 8
stack_bottom:
	.space 4096
stack_top:
	.long 4

vbr_addr:
    .space 0x600
exc_handler:
    ! check INTEVT
!    mov.l intevtp, r0
!    mov.l @r0, r0
!    mov #0x04, r1
!    shll8 r1
!    cmp/eq r0, r1
!    bt tmu_tuni0

!    rte
!    nop

tmu_tuni0:
    ! DO NOT COMMIT
!    bra tmu_tuni0
!    nop

    ! acknowledge the underflow to the TMU
    xor r0, r0
    mov.l tcr0p, r0
    mov.w @r0, r1
    mov.l unf_clear_mask, r2
    and r2, r1
    mov.w r1, @r0

    ! increment _ticks and return
    mova _ticks, r0
    mov.l @r0, r1
    add #1, r1
    rte
    mov.l r1, @r0

    .align 4
_ticks:
    .long 0
tcr0p:
    .long 0xffd80010
unf_clear_mask:
    .long ~(1 << 8)
intevtp:
    .long 0xff000028
