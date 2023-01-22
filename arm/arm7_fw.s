    .global _start

    .global main

    .align 4
_start:
	b reset
	b undefined
	b swi
	b pref_abort
	b data_abort
	b wtf_is_this
	b irq
	b fiq

undefined:
	ldr sp, =excp_stack
	sub sp, sp, #4
	str lr, [sp]
	mov r0, #0
	bl on_error
	ldr lr, [sp]
	add sp, sp, #4
	movs pc, r14
swi:
	ldr sp, =excp_stack
	sub sp, sp, #4
	str lr, [sp]
	mov r0, #1
	bl on_error
	ldr lr, [sp]
	add sp, sp, #4
	movs pc, r14
pref_abort:
	ldr sp, =excp_stack
	sub sp, sp, #4
	str lr, [sp]
	mov r0, #2
	bl on_error
	ldr lr, [sp]
	add sp, sp, #4
	subs pc, r14, #4
data_abort:
	ldr sp, =excp_stack
	sub sp, sp, #4
	str lr, [sp]
	mov r0, #3
	bl on_error
	ldr lr, [sp]
	add sp, sp, #4
	subs pc, r14, #8
wtf_is_this:
	ldr sp, =excp_stack
	sub sp, sp, #4
	str lr, [sp]
	mov r0, #4
	bl on_error
	ldr lr, [sp]
	add sp, sp, #4
	subs pc, r14, #4
irq:
	ldr sp, =excp_stack
	sub sp, sp, #4
	str lr, [sp]
	mov r0, #5
	bl on_error
	ldr lr, [sp]
	add sp, sp, #4
	subs pc, r14, #4
fiq:
	ldr sp, =excp_stack
	sub sp, sp, #4
	str lr, [sp]
	mov r0, #6
	mov r1, lr
	bl on_error
	ldr lr, [sp]
	add sp, sp, #4
	subs pc, r14, #4

on_error:
 	sub sp, sp, #4
 	str lr, [sp]

 	bl notify_exception

on_error_forever:
 	b on_error_forever

 	@@ this will never be executed but is here for posterity
 	ldr lr, [sp]
 	add sp, sp, #4
 	mov pc, lr

excp_stack:
 	.zero 128

    .align 4
reset:
 	mrs r0, cpsr
 	orr r0, r0,#0xc0
 	msr cpsr, r0

    @@ initialize the stack pointer
    mov sp, #0xf000
    add sp, sp, #-4 @@ <= this is probably unnecessary tbh

    @ time to have some fun
    eor r0, r0, r0
    bl get_chanp

    @ r0 now points to channel

    @ set SampleAddrLow
    mov r1, #4096
    lsl r1, r1, #16
    lsr r1, r1, #16
    str r1, [r0, #4]

    @ set LoopStart
    eor r1, r1, r1
    str r1, [r0, #8]

    @ set LoopEnd
    mov r1, #400
    str r1, [r0, #12]

    @ AmpEnv1
    mov r1, #0x1f
    str r1, [r0, #16]

    @ AmpEnv2
    mov r1, #0xf
    lsl r1, r1, #10
    str r1, [r0, #20]

    @ SampleRatePitch
    eor r1, r1, r1
    str r1, [r0, #24]

    @ PlayCtrl
    mov r1, #4096
    lsr r1, r1, #16
    ldr r2, keyon
    orr r1, r1, r2
    str r1, [r0]

    @    bl arm_main
    @@ initialize the message passing system
    @bl msg_init
    @@ XXX - send a simple message to the sh4 to let it know we're alive
    @bl xmit_fib_msg

forever:
    b forever

get_chanp:
    lsl r0, r0, #7
    ldr r1, aica_regs
    add r0, r1
    mov pc, lr

    .align 4
aica_regs:
    .long 0x800000
keyon:
    .long (1<<15) | (1<<14) | (1<<9)

    .align
    .pool
