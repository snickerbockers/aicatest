.global _start

.equ VARBASE,	0x80000

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
	@@ ldr sp, =excp_stack
	@@ sub sp, sp, #4
	@@ str lr, [sp]
	@@ mov r0, #0
	@@ bl on_error
	@@ ldr lr, [sp]
	@@ add sp, sp, #4
	@@ movs pc, r14
swi:
	@@ ldr sp, =excp_stack
	@@ sub sp, sp, #4
	@@ str lr, [sp]
	@@ mov r0, #1
	@@ bl on_error
	@@ ldr lr, [sp]
	@@ add sp, sp, #4
	@@ movs pc, r14
pref_abort:
	@@ ldr sp, =excp_stack
	@@ sub sp, sp, #4
	@@ str lr, [sp]
	@@ mov r0, #2
	@@ bl on_error
	@@ ldr lr, [sp]
	@@ add sp, sp, #4
	@@ subs pc, r14, #4
data_abort:
	@@ ldr sp, =excp_stack
	@@ sub sp, sp, #4
	@@ str lr, [sp]
	@@ mov r0, #3
	@@ bl on_error
	@@ ldr lr, [sp]
	@@ add sp, sp, #4
	@@ subs pc, r14, #8
wtf_is_this:
	@@ ldr sp, =excp_stack
	@@ sub sp, sp, #4
	@@ str lr, [sp]
	@@ mov r0, #4
	@@ bl on_error
	@@ ldr lr, [sp]
	@@ add sp, sp, #4
	@@ subs pc, r14, #4
irq:
	@@ ldr sp, =excp_stack
	@@ sub sp, sp, #4
	@@ str lr, [sp]
	@@ mov r0, #5
	@@ bl on_error
	@@ ldr lr, [sp]
	@@ add sp, sp, #4
	@@ subs pc, r14, #4
fiq:
	@@ ldr sp, =excp_stack
	@@ sub sp, sp, #4
	@@ str lr, [sp]
	@@ mov r0, #6
	@@ mov r1, lr
	@@ bl on_error
	@@ ldr lr, [sp]
	@@ add sp, sp, #4
	@@ subs pc, r14, #4

@@ on_error:
@@ 	sub sp, sp, #4
@@ 	str lr, [sp]

@@ 	bl notify_exception

@@ on_error_forever:
@@ 	b on_error_forever

@@ 	@@ this will never be executed but is here for posterity
@@ 	ldr lr, [sp]
@@ 	add sp, sp, #4
@@ 	mov pc, lr

@@ excp_stack:
@@ 	.zero 128

@@ 	.align 4
reset:
 	mrs r0, cpsr
 	orr r0, r0,#0xc0
 	msr cpsr, r0

@@ initialize the stack pointer
    ldr r0, =stack_top
    add sp, sp, #-4 @@ <= this is probably unnecessary tbh

    @@ initialize the message passing system
    bl msg_init

    @@ XXX - send a simple message to the sh4 to let it know we're alive
    bl xmit_fib_msg

forever:
    b forever

    .pool
stack_top_val:	 .long stack_top
    .align 8
stack_bottom:
 	.space 4096
stack_top:

    .align
