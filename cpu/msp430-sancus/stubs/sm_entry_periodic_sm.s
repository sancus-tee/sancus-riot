    .section ".sm.text"
    .align 2
    .global __sm_entry
    .type __sm_entry,@function

    ; r6: ID of entry point to be called, 0xffff if returning
    ; r7: return address
__sm_entry:
    ; === need a secure stack to handle IRQs ===
    ; dint
    ; First remove ssa base addr to notify violation handler to not use this SMs violation pointer
    mov #0, &__sm_ssa_base_addr
    ; back up r15
    mov r15, &__sm_tmp
    ; set up clix length and call clix (word 0x1389)
    mov #100, r15 
    .word 0x1389
    ; restore r15
    mov &__sm_tmp, r15

    ; initialize SSA frame address for IRQ logic
    ; Technically, this could be set to any SSA but we have just one for now
    mov #__sm_ssa_base, &__sm_ssa_base_addr
    
    ; Our stack pointer is either at __sm_ssa_sp or __sm_sp, depending
    ; on whether we got interrupted last time or not. Pick __sm_sp only if 
    ; __sm_ssa_sp is empty.
    mov &__sm_ssa_sp, r1
    cmp #0, r1
    jne 1f
    ; ssa_sp is empty -> __sm_sp is our stackpointer, it lies at #ssa_base-2
    mov &__sm_sp, r1
    ; initialize sp on first entry
    cmp #0x0, r1
    jne 1f
    mov #__sm_stack_init, r1

1:
    ; check if this is a return from a interrupt
    bit #0x1, &__sm_ssa_sp

    jz 1f
    ; restore execution state if the sm was resumed
    br #__reti_entry ; defined in exit.s

1:
    ; === safe to handle IRQs now ===
    eint
    ; check if this is a return
    cmp #0xffff, r6
    jne 1f
    br #__ret_entry ; defined in exit.s

1:
    ; check if the given index (r6) is within bounds
    cmp #__sm_nentries, r6
    jhs .Lerror

    ; Check caller is scheduler (genuine indexes are only to be called by the scheduler for these SMs)
    ; sancus_get_caller_id()
    ; disabled during development but to be enabled later.
    ;.word 0x1387
    ;cmp #0x1, r15
    ;jne .Lerror

    ; MODIFICATION: Since exclusive SMs are run periodically, all genuine ECALLs reset the stack
    mov #__sm_stack_init, r1
    mov #0, &__sm_ssa_sp

    ; store callee-save registers
    push r4
    push r5
    push r8
    push r9
    push r10
    push r11

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
    br r7

.Lerror:
    br #exit
