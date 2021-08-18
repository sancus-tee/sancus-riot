/*
 * Copyright (C) 2016 Kaspar Schleiser <kaspar@schleiser.de>
 *               2014, Freie Universitaet Berlin (FUB) & INRIA.
 * All rights reserved.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include "secure_mintimer.h"
#include "cpu.h"
#include "irq.h"
#include "sched.h"
#include "thread.h"

#define ENABLE_DEBUG 0
#include "debug.h"

const SM_DATA(sancus_sm_timer) int thread_sm_idx_offset = offsetof(thread_t, sm_idx);

void thread_yield_higher(void){
    //store pc as continue label
    __asm__ ("mov #yield_higher_continue, r10");

    // perform a full save context and exitless call
    ___MACRO_EXITLESS_CALL_WITH_RESUME(EXITLESS_FUNCTION_TYPE_YIELD)

    __asm__ ("yield_higher_continue:");

    return;
}

NORETURN void scheduler_kernel_init(void)
{
    ___MACRO_PREPARE_EXITLESS_CALL(EXITLESS_FUNCTION_TYPE_BOOT)

    UNREACHABLE();
}

NORETURN void cpu_switch_context_exit(void)
{
    ___MACRO_PREPARE_EXITLESS_CALL(EXITLESS_FUNCTION_TYPE_EXIT)

    UNREACHABLE();
}

/*
 * we must prevent the compiler to generate a prologue or an epilogue
 * for thread_yield_higher(), since we rely on the RETI instruction at the end
 * of its execution
 */
NORETURN SM_FUNC(sancus_sm_timer) __attribute__((naked)) void thread_yield_higher_internal(USED_IN_ASM bool do_thread_yield)
{
    // r2 pushed by caller
    // __asm__("push r2"); /* save SR */
    
    // Scheduler is non-interruptible
    // __disable_irq();
    // ___MACRO_DISABLE_IRQ

    // Context saved by caller
    // __save_context();
    // ___MACRO_SAVE_CONTEXT

    // Call the yield in the scheduler if requested. This replaces the old thread.c yield.
    __asm__("cmp #1, r15");
    __asm__("jne 1f");
    __asm__("call %0" : : "i"(sched_yield));
    __asm__("1:");

    /* have sched_active_thread point to the next thread */
    // sched_run();
    __asm__("call %0" : : "i"(sched_run_internal));

    // __restore_context();
    ___MACRO_RESTORE_CONTEXT

    // UNREACHABLE();
    ___MACRO_UNREACHABLE
}

/**
 * This is the function to call for exitless entries such as thread_yield_higher or cpu_exit.
 * Depending on whether being called from inside an SM or from untrusted code, the second argument
 * is either to be the current SP or the sm_entry of the SM
 * */
void SM_ENTRY(sancus_sm_timer)  __attribute__((naked, noreturn)) exitless_entry(
        USED_IN_ASM int FUNCTION_TYPE, 
        USED_IN_ASM void* sp_or_return, 
        USED_IN_ASM uint8_t optional1, 
        USED_IN_ASM uint8_t optional2){
    // Check caller_id and check whether we are executing an sm.
    // If that is the case, then set the entry point for next scheduling to 0xff
    // to denote a return. If not, we do not change anything.
    // we can use r15 for caller id since it is not used by the function call
    
    /*
     * First, store context of active thread.
    */
    // Prepare r11 with active thread
    __asm__("mov.w &%0, r11\t\n" : : "m"(sched_active_thread));
    // We have an edge case now, where if we are called by kernel_init, the active thread
    // may be empty. When this happens, we jump directly over storing the context.
    // The same applies for exit calls. They do not store the context.
    __asm__("tst r11");
    __asm__("jz 2f");
    __asm__("cmp %0, r15" : : "i"(EXITLESS_FUNCTION_TYPE_EXIT));
    __asm__("jeq 2f");

    // If we are not an exit call and if there is an active thread,
    // place r14 in active_thread SP. Irrespective of whether this is an SM,
    // this will be set as the unprotected SP on exits. This is also relevant for resumed SMs
    __asm__("mov.w r14, 2(r11)");

    // Check caller id and cmp it with UNPROTECTED_ID (usually 0)
    __asm__("push r15");
    __asm__(".word 0x1387"); // sancus_get_caller_id
    __asm__("cmp %0, r15" : : "i"(SM_ID_UNPROTECTED));
    __asm__("pop r15");

    __asm__("jeq 1f");
    // caller_id != PROTECTED
    __asm__("mov.w #1,0(r11)"); // Mark as sm
    __asm__("add.w %0,r11" : : "i"(thread_sm_idx_offset));// Move r10 to sm_idx 
    __asm__("mov.w #0xffff,0(r11)");// Mark sm_idx as return
    __asm__("jmp 2f");
    __asm__("1:");
    // caller_id == UNPROTECTED
    __asm__("mov.w #0,0(r11)"); // Mark as unprotected
    __asm__("2:");
    __asm__("clr r11");



    /*
     * Second, select function to jump to and branch there.
    */
    // R15 keeps the int what function to call
    // Check whether input is EXITLESS_FUNCTION_TYPE_BOOT
    __asm__("cmp %0, r15" : : "i"(EXITLESS_FUNCTION_TYPE_BOOT));
    __asm__("jne 1f");
        // We are booting.
        // Here, we call the sched_init function and yield as we are not in an active thread yet.
        // An attacker gains nothing by calling it as it behaves like a thread_yield after the first call.
        // Theoretically we could also have thread_yield perform this action on its first call.
        __asm__("call %0" : : "i"(scheduler_init));
        __asm__("jmp .default");
    // Check whether input is EXITLESS_FUNCTION_TYPE_EXIT
    __asm__("1:");
    __asm__("cmp %0, r15" : : "i"(EXITLESS_FUNCTION_TYPE_EXIT));
    __asm__("jne 1f");
        // We are an exit
        // The difference between cpu_switch_context_exit and yield_higher is simply
        // that we call sched_task_exit_internal first.
        __asm__("call %0" : : "i"(sched_task_exit_internal));
        __asm__("jmp .default");

    __asm__("1:");
    __asm__("cmp %0, r15" : : "i"(EXITLESS_FUNCTION_TYPE_SCHED_SWITCH));
    __asm__("jne 3f"); // MACRO RESTORE CONTEXT uses 1 and 2 labels
        // We are a sched switch
        // On a sched_switch, we have a uint16_t in r13.
        // Use these to call sched_switch. This will either yield itself or return which is 
        // When we have to do our own restore context.
        __asm__("mov r13, r15");
        __asm__("call %0" : : "i"(sched_switch_internal));
        ___MACRO_RESTORE_CONTEXT

    __asm__("3:");
    __asm__("cmp %0, r15" : : "i"(EXITLESS_FUNCTION_TYPE_SLEEP));
    __asm__("jne 1f");
        // We are timer sleep
        // In this case, we have a uint32_t in r13 and r12.
        __asm__("mov r13, r15");
        __asm__("mov r12, r14");
        __asm__("call %0" : : "i"(_secure_mintimer_tsleep_internal));
        // After sleeping, yield to a higher thread.
        __asm__("jmp .default");

    // default: yield
    // on yield, set sched_context_switch_request to 1. 
    __asm__("1:");
    __asm__(".default:");
    // Check whether a timer overflow happened in the meantime
        __asm__("mov %0, r15" : : "m"(TIMER_BASE->CTL));
        __asm__("bit %0, r15" : : "i"(TIMER_CTL_IFG));
        __asm__("jnz 1f"); 
            __asm__("mov %0, r15" : : "m"(TIMER_BASE->CCTL[0]));
            __asm__("bit %0, r15" : : "i"(TIMER_CCTL_CCIFG));
            __asm__("jz default_continue"); 
            __asm__("1:");
            // timer overflowed! handle this first
            __asm__("mov #1, &%0"  : "=m"(__sm_irq_is_in));
            __asm__("mov.w %0,r1" : : "i"(__isr_stack + ISR_STACKSIZE));
            ___MACRO_ENTER_ISR_NO_CONTEXT_STORE
            __asm__ volatile ("clr r15");
            __asm__ volatile ("clr r14");
            __asm__("call %0" : : "i"(secure_mintimer_timer_callback));
            __asm__ volatile ("mov #1, &%0" : "=m"(sched_context_switch_request));
            __MACRO_EXIT_ISR
        __asm__("default_continue:");

    // Call thread yield higher with the argument 1 to enable thread_yield
    __asm__("mov #1, r15");
    __asm__("br %0": : "i"(thread_yield_higher_internal));

    // We will never reach this location due to yield handling the resumption later.
    // ___MACRO_UNREACHABLE
    __asm__("jmp .UNREACHABLE");
    
}

/**
 * mspgcc handles main specially - it does not return but falls
 * through to section .fini9.
 * To "fix" this, we put a return in section .fini9 to make main
 * behave like a regular function. This enables a common
 * thread_stack_init behavior. */
__attribute__((section (".fini9"))) void __main_epilogue(void) { __asm__("ret"); }




/* ------------------------------------------------------------------------- */
/*  Processor specific routine - here for MSP */
/* ------------------------------------------------------------------------- */
char *thread_stack_init(thread_task_func_t task_func, void *arg, void *stack_start, int stack_size)
{
    DEBUG("[CPU] thread_stack_init: Called with stack at location %p and size %x\n", stack_start, stack_size);
    unsigned short stk = (unsigned short)((uintptr_t) stack_start + stack_size);

    /* ensure correct stack alignment (on 16-bit boundary) */
    stk &= 0xfffe;
    unsigned short *stackptr = (unsigned short *)stk;

    /* now make SP point on the first AVAILABLE slot in stack */
    --stackptr;
    DEBUG("[CPU] thread_stack_init: Top of the stack is: %p\n", stackptr);

    --stackptr;
    *stackptr = (unsigned short) sched_task_exit;

    --stackptr;
    *stackptr = (unsigned short) task_func;
    DEBUG("[CPU] thread_stack_init: Pointer to function is %p stored at %p\n", task_func, stackptr);

    /* initial value for SR */

    --stackptr;
    *stackptr = GIE;

    /* set argument to task_func 
     * This will go into r15 as argument
    */
    --stackptr;
    *stackptr = (unsigned short) arg;

    /* Space for 11 remaining registers (r4-r14). */
    for (unsigned int i = 4; i < 15; i++) {
        --stackptr;
        *stackptr = i;
    }
    DEBUG("[CPU] thread_stack_init: Returning pointer to thread stack: %p\n", stackptr);
    return (char *) stackptr;
}

char *thread_unprotected_stack_init(void *stack_start, int stack_size){
DEBUG("[CPU] thread_stack_init: Called with stack at location %p and size %x\n", stack_start, stack_size);
    unsigned short stk = (unsigned short)((uintptr_t) stack_start + stack_size);

    /* ensure correct stack alignment (on 16-bit boundary) */
    stk &= 0xfffe;
    unsigned short *stackptr = (unsigned short *)stk;

    /* now make SP point on the first AVAILABLE slot in stack */
    --stackptr;
    DEBUG("[CPU] thread_stack_init: Top of the stack is: %p\n", stackptr);

    
    return (char *) stackptr;
}