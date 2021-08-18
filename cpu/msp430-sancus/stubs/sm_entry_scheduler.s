; Special version of sm_entry for the Scheduler that is always SM 1
; The difference to the normal sm_entry stub is as follows:
;   1) There are no return calls to be expected. As such we use
;       return calls as a special NOEXIT function that expects the caller 
;       to be able to be resumed via a normal reti routine (pop all registers)
;       Importantly, we do not store callee save registers etc for these calls.
;       Note: Breaking the calling convention like this may come with possible side effects :-).
;   MODIFICATION: Scheduler has no reti (which means __sm_ssa_sp is unused)
;   MODIFICATION: Scheduler is not interruptable

    .section ".sm.text"
    .align 2
    .global __sm_entry
    .type __sm_entry,@function

    ; r6: ID of entry point to be called, 0xffff if returning
    ; r7: return address
__sm_entry:
    ; === need a secure stack to handle IRQs ===
    dint

    ; MODIFICATION: We will need the current SP later. 
    ; In contrast to normal SMs, the scheduler needs to remember the old SP
    ; This is because we can not necessarily trust our own untrusted SP if we get
    ; interrupted right after we leave; it might change before we return to that PC 
    ; to restore it.
    mov r1, &__sm_tmp

    ; initialize SSA frame address for IRQ logic
    ; Technically, this could be set to any SSA but we have just one for now
    mov #__sm_ssa_base, &__sm_ssa_base_addr
    
    ; Our stack pointer is either at __sm_ssa_sp or __sm_sp, depending
    ; on whether we got interrupted last time or not. Pick __sm_sp only if 
    ; __sm_ssa_sp is empty.
    ; MODIFICATION: Scheduler does not have a reti, so we never care about the 
    ; SP in ssa_sp for our own stack. We always start at __sm_sp.
    ; ssa_sp is empty -> __sm_sp is our stackpointer, it lies at #ssa_base-2
    mov &__sm_sp, r1
    ; initialize sp on first entry
    cmp #0x0, r1
    jne 1f
    mov #__sm_stack_init, r1

1:
    ; check if this is an IRQ
    push r15
    ; sancus_get_caller_id()
    .word 0x1387
    cmp #0xfff0, r15
    jlo 1f
    ; SEMI-HACK: If we are not protected, and no other SM has ever been
    ; executed, the caller ID will be that of the last IRQ because entering this
    ; SM was no protection domain switch. This basically means that once an IRQ
    ; has occurred, we cannot call normal entry points anymore. Since it is nice
    ; to be able to use unprotected SMs during testing, and it is quiet common
    ; to have interrupts disabled then, the caller ID will always be that of the
    ; reset IRQ (0xffff). Since there is no valid use case of actually handling
    ; a reset inside an SM (since the reset will disable all SMs), we simply
    ; ignore it here so that normal entry points can still be used.
    cmp #0xffff, r15
    jeq 1f
    ; If we just do je __sm_isr we get a PCREL relocation which our runtime
    ; linker doesn't understand yet.
    br #__sm_isr

1:
    ; We are not called by an IRQ. If __sm_ssa_caller_id is not set, fill it with the caller id
    mov #__sm_ssa_caller_id, r15
    cmp #0x0, r15
    jne 1f
    mov r15, &__sm_ssa_caller_id

1:
    ; Pop r15 again from the stack (we don't need the caller_id anymore)
    pop r15

1:
    ; check if this is an exitless entry
    cmp #__sm_sancus_sm_timer_entry_exitless_entry_idx, r6
    ; If we call exitless entry, we jump over storing callee save registers
    jeq 2f 

    ; The Scheduler makes outcalls to the MMIO module
    ; check if this is a return
    cmp #0xffff, r6
    jne 1f
    br #__ret_entry ; defined in exit.s

1:
    ; check if the given index (r6) is within bounds
    cmp #__sm_nentries, r6
    jhs .Lerror

    ; store callee-save registers
    push r4
    push r5
    push r8
    push r9
    push r10
    push r11

2:
    ; calculate offset of the function to be called (r6 x 6)
    rla r6
    mov r6, r11
    rla r6
    add r11, r6

    ; function address
    mov __sm_table(r6), r11

    ; call the sm
    call r11

    ; restore callee-save registers
    pop r11
    pop r10
    pop r9
    pop r8
    pop r5
    pop r4

    ; clear the arithmetic status bits (0, 1, 2 and 8) of the status register
    and #0x7ef8, r2

    ; clear the return registers which are not used
    mov 4+__sm_table(r6), r6
    rra r6
    jc 1f
    clr r12
    clr r13
    rra r6
    jc 1f
    clr r14
    rra r6
    jc 1f
    clr r15

1:
    mov #0xffff, r6
    mov #0, &__sm_ssa_caller_id
    mov #0, &__sm_sp
    ; Restore old SP of caller
    mov &__sm_tmp, r1

    ; TODO: We need a clix instruction here that enables interrupts again in 2 cycles
    ; The problem here is that with an eint at this position, we may trigger an interrupt
    ; right after the next instruction when r1 is still zero and the IRQ logic thinks we 
    ; are still inside an SM. This means that it will push to the scheduler SSA but has no
    ; SP to store. This is tricky since the thread will have no SP to resume and the scheduler SSA
    ; will be monopolized. It is better if we can allow interrupts one cycle later when r1 has been filled
    ; with the unprotected SP and the SM has been left (so that IRQ logic does not use the scheduler SSA).
    ; This then means that the untrusted or other SM is responsible for setting the r1 in this time
    ; which when it does not do that means that it kills its own thread. 
    eint
    br r7

.Lerror:
    br #exit
