/*
 * Copyright (C) 2014, Freie Universitaet Berlin (FUB) & INRIA.
 * All rights reserved.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    cpu_msp430_common TI MSP430
 * @ingroup     cpu
 * @brief       Texas Instruments MSP430 specific code
 *
 * @{
 * @file
 * @brief       Texas Instruments MSP430 specific code
 *
 */

#ifndef CPU_H
#define CPU_H

#include <msp430.h>
#include "msp430_types.h"
#include <stdio.h>

#include "board.h"

#include "sched.h"
#include "thread.h"
#include "cpu_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Wordsize in bit for MSP430 platforms
 */
#define WORDSIZE 16

/**
 * 
 * Overwrite MSP430 F149 definitions of the IRQ vector table with the sancus version
 * 
 * */
/************************************************************
* Interrupt Vectors (offset from 0xFFE0)
************************************************************/

// #define PORT2_VECTOR        (0x0002)  /* 0xFFE2 Port 2 */
// #define USART1TX_VECTOR     (0x0004)  /* 0xFFE4 USART 1 Transmit */
// #define USART1RX_VECTOR     (0x0006)  /* 0xFFE6 USART 1 Receive */
// #define PORT1_VECTOR        (0x0008)  /* 0xFFE8 Port 1 */
// #define TIMERA1_VECTOR      (0x000A)  /* 0xFFEA Timer A CC1-2, TA */
// #define TIMERA0_VECTOR      (0x000C)  /* 0xFFEC Timer A CC0 */
// #define ADC12_VECTOR          (0x000E)  /* 0xFFEE ADC */
// #define USART0TX_VECTOR     (0x0010)  /* 0xFFF0 USART 0 Transmit */
// #define USART0RX_VECTOR     (0x0012)  /* 0xFFF2 USART 0 Receive */
// #define WDT_VECTOR          (0x0014) /* 0xFFF4 Watchdog Timer */
// #define COMPARATORA_VECTOR  (0x0016) /* 0xFFF6 Comparator A */
// #define TIMERB1_VECTOR      (0x0018) /* 0xFFF8 Timer B CC1-6, TB */
// #define TIMERB0_VECTOR      (0x001A) /* 0xFFFA Timer B CC0 */
// #define NMI_VECTOR          (0x001C) /* 0xFFFC Non-maskable */
// #define RESET_VECTOR        (0x001E) /* 0xFFFE Reset [Highest Priority] */
// 
// #define UART1TX_VECTOR      USART1TX_VECTOR
// #define UART1RX_VECTOR      USART1RX_VECTOR
// #define UART0TX_VECTOR      USART0TX_VECTOR
// #define UART0RX_VECTOR      USART0RX_VECTOR
// #define ADC_VECTOR          ADC12_VECTOR

// undefine vectors
#undef PORT2_VECTOR      
#undef USART1TX_VECTOR   
#undef USART1RX_VECTOR   
#undef PORT1_VECTOR      
#undef TIMERA1_VECTOR    
#undef TIMERA0_VECTOR    
#undef ADC12_VECTOR      
#undef USART0TX_VECTOR   
#undef USART0RX_VECTOR   
// #undef WDT_VECTOR        // Not necessary to redefine this
#undef COMPARATORA_VECTOR
#undef TIMERB1_VECTOR    
#undef TIMERB0_VECTOR    
#undef NMI_VECTOR        
// #undef RESET_VECTOR      // Not necessary to redefine this

// redefine used vectors
#define TIMERA0_VECTOR      (0x0012)  /* 0xFFF2 Timer A CC0 */
#define TIMERA1_VECTOR      (0x0010)  /* 0xFFF0 Timer A CC1-2, TA */
#define USART0RX_VECTOR     (0x000E)  /* 0xFFEE USART 0 Receive */
#define USART0TX_VECTOR     (0x000C)  /* 0xFFEC USART 0 Transmit */

/**
 * End vector table.
 * */

/**
 * @brief   Macro to clear, push, and pop all registers r4-15
 */
#define ___MACRO_CLEAR_REGISTERS    \
    __asm__ volatile ("clr r15");             \
    __asm__ volatile ("clr r14");             \
    __asm__ volatile ("clr r13");             \
    __asm__ volatile ("clr r12");             \
    __asm__ volatile ("clr r11");             \
    __asm__ volatile ("clr r10");             \
    __asm__ volatile ("clr r9");              \
    __asm__ volatile ("clr r8");              \
    __asm__ volatile ("clr r7");              \
    __asm__ volatile ("clr r6");              \
    __asm__ volatile ("clr r5");              \
    __asm__ volatile ("clr r4");

#define ___MACRO_SAVE_REGISTERS                              \
    __asm__ ("push r15");                                    \
    __asm__ ("push r14");                                    \
    __asm__ ("push r13");                                    \
    __asm__ ("push r12");                                    \
    __asm__ ("push r11");                                    \
    __asm__ ("push r10");                                    \
    __asm__ ("push r9");                                     \
    __asm__ ("push r8");                                     \
    __asm__ ("push r7");                                     \
    __asm__ ("push r6");                                     \
    __asm__ ("push r5");                                     \
    __asm__ ("push r4");                                     

#define ___MACRO_RESTORE_REGISTERS                            \
    __asm__ ("pop r4");                                       \
    __asm__ ("pop r5");                                       \
    __asm__ ("pop r6");                                       \
    __asm__ ("pop r7");                                       \
    __asm__ ("pop r8");                                       \
    __asm__ ("pop r9");                                       \
    __asm__ ("pop r10");                                      \
    __asm__ ("pop r11");                                      \
    __asm__ ("pop r12");                                      \
    __asm__ ("pop r13");                                      \
    __asm__ ("pop r14");                                      \
    __asm__ ("pop r15");

/**
 * Define some integers for entry into the scheduler: 
 *  On a call with 0, we want yield, on a call of 1 we want thread_exit.
 * */
#define EXITLESS_FUNCTION_TYPE_BOOT  0
#define EXITLESS_FUNCTION_TYPE_YIELD 1
#define EXITLESS_FUNCTION_TYPE_EXIT  2
#define EXITLESS_FUNCTION_TYPE_SCHED_SWITCH  3
#define EXITLESS_FUNCTION_TYPE_SLEEP  4

#define USED_IN_ASM __attribute__ ((unused))

#define ___MACRO_PREPARE_EXITLESS_CALL_COMMON(function_type)                       \
    /* prepare call and do it */                                            \
    __asm__("mov.w #__sm_sancus_sm_timer_entry_exitless_entry_idx, r6");    \
    __asm__("mov.w #0, r7");                                                \
    __asm__("mov.w %0, r15" : : "i"(function_type));                                \
    __asm__("br #__sm_sancus_sm_timer_entry");

#define ___MACRO_PREPARE_EXITLESS_CALL(function_type)                       \
    /* We place r1 in r14 where the scheduler can find it to store it */    \
    __asm__ ("mov r1, r14");                                                \
    ___MACRO_PREPARE_EXITLESS_CALL_COMMON(function_type)                    


#define ___MACRO_DO_EXITLESS_CALL_FROM_SM(function_type, sm)   \
    __asm__ ("push r12");                                             \
    __asm__ ("clr r12");                                             \
    __asm__ ("push r13");                                             \
    __asm__ ("clr r13");                                             \
    ___MACRO_PREPARE_EXITLESS_CALL_FROM_SM_COMMON(function_type, sm)\
    __asm__ ("pop r13");                                             \
    __asm__ ("pop r12");                                             
    
#define ___MACRO_PREPARE_EXITLESS_CALL_FROM_SM_COMMON(function_type, sm)   \
    ___MACRO_SAVE_REGISTERS                                         \
    /* Back up r10 and store next PC jump point at end of this macro*/\
    __asm__ ("mov #9f, r10");                   \
    /* Push next PC (in r10) ret later */               \
    __asm__ ("push r10");                                           \
    /* Push context */                                              \
    __asm__ ("push r6");                                            \
    __asm__ ("clr r6");                                             \
    __asm__ ("push r7");                                            \
    __asm__ ("clr r7");                                             \
    __asm__ ("push r8");                                            \
    __asm__ ("clr r8");                                             \
    __asm__ ("push r4");                                            \
    __asm__ ("clr r4");                                             \
    __asm__ ("push r5");                                            \
    __asm__ ("clr r5");                                             \
    __asm__ ("push r9");                                            \
    __asm__ ("clr r9");                                             \
    __asm__ ("push r10");                                           \
    __asm__ ("clr r10");                                            \
    __asm__ ("push r11");                                           \
    __asm__ ("clr r11");                                            \
    /* Save Scheduler ID as our ocall_id (SM ID 1) */               \
    /* This will help us later to make sure returns are legit */    \
    asm("mov #1, &__sm_" #sm "_ssa_ocall_id");                      \
    /* Prepare untrusted sp */                                      \
    __asm__ ("mov &__unprotected_sp, r14");                         \
    /* Save r1 as SP */                                             \
    /* __sm_foo_ssa_sp */             \
    __asm__ ("mov r1, &__sm_" #sm "_sp");                       \
    /* Perform the call to scheduler */                             \
    ___MACRO_PREPARE_EXITLESS_CALL_COMMON(function_type)            \
    /* End label and pop r10 again */                               \
    __asm__ ("9:");                            \
    ___MACRO_RESTORE_REGISTERS                                       

/**
 * Internal version for yielding. Is called by the scheduler.c
 * */
NORETURN SM_FUNC(sancus_sm_timer) __attribute__((naked)) void thread_yield_higher_internal(bool do_thread_yield);


// For this macro, r10 must hold a valid address for PC resumption!!
#define ___MACRO_EXITLESS_CALL_WITH_RESUME(function_type)       \
    /* store r10 */                                     \
    __asm__ ("push r10");                               \
    /* store pc as continue label */                    \
    __asm__ ("mov #9f, r10");                           \
    /* Push PC (in r10) and r2 for reti later */        \
    __asm__ ("push r10");                               \
    __asm__ ("push r2");                                \
    /* Push context */                                  \
    ___MACRO_SAVE_REGISTERS                             \
    /* Then do the exitless call */                     \
    ___MACRO_PREPARE_EXITLESS_CALL(function_type)       \
    ___MACRO_RESTORE_REGISTERS                          \
    __asm__ ("9:");                                     \
    /* restore r10 */                                   \
    __asm__ ("pop r10");                                

/**
 * Function to use as an entry point for exitless calls (calls that are not direclty
 * returned to but indirectly through the next scheduling.)
 * */
void SM_ENTRY(sancus_sm_timer)  __attribute__((naked, noreturn)) exitless_entry(
        USED_IN_ASM int FUNCTION_TYPE, 
        USED_IN_ASM void* sp_or_return, 
        USED_IN_ASM uint8_t optional1, 
        USED_IN_ASM uint8_t optional2);

/**
 * @brief   Macro for defining interrupt service routines
 */
#define ISR(a,b)        void __attribute__((naked, interrupt (a))) b(void)

/**
 * @brief Globally disable IRQs
 */
#define ___MACRO_DISABLE_IRQ                        \
__asm__ __volatile__("bic  %0, r2" : : "i"(GIE)) ;  \
__asm__ __volatile__("nop") ;                       
    // the NOP is needed to handle a "delay slot" that all MSP430 MCUs
   //   impose silently after messing with the GIE bit, DO NOT REMOVE IT!
static inline void __attribute__((always_inline)) __disable_irq(void)
{
    ___MACRO_DISABLE_IRQ
}

/**
 * @brief Globally enable IRQs
 */
#define ___MACRO_ENABLE_IRQ                                     \
    __asm__ __volatile__("bis  %0, r2" : : "i"(GIE));                 \
    __asm__ __volatile__("nop");                                      
    /* the NOP is needed to handle a "delay slot" that all MSP430 MCUs
       impose silently after messing with the GIE bit, DO NOT REMOVE IT! */
static inline void __attribute__((always_inline)) __enable_irq(void)
{
   ___MACRO_ENABLE_IRQ
}

/**
 * @brief   The current ISR state (inside or not)
 */
extern volatile int __sm_irq_is_in;

/**
 * @brief   Memory used as stack for the interrupt context
 */
extern char SM_DATA(sancus_sm_timer) __isr_stack[ISR_STACKSIZE];

/**
 * @brief   Restore the TRUSTED thread context from inside a protected ISR
 * In contrast to an untrusted code, we do not restore registers but
 * leave this to the reti sm_exit stub of the restored function. 
 * Here, we prepare r6, push pc and sr on our stack and reti.
 * We also prepare r7 to point to the sm_entry of the scheduler.
 * This guarantees that all called sms return to the scheduler.
 */
#define ___MACRO_RESTORE_TRUSTED_CONTEXT                          \
    /* Restore untrusted SP from thread structure before clearing all registers */\
    __asm__ ("mov %0, &__unprotected_sp" : "=m"(sched_active_thread->sp));      \
    ___MACRO_CLEAR_REGISTERS                                       \
    /* Put sm_idx in r6 */                                        \
    __asm__ volatile ("mov.w %0,r6"  : : "m"(sched_active_thread->sm_idx)); \
    /* Put sm_entry of scheduler in r7 */                         \
    __asm__ volatile ("mov.w &%0,r7"  : : "m"(scheduler_entry));             \
    /* Put sm_entry of restored func on our stack */                  \
    __asm__ volatile ("mov.w %0,r5"  : : "m"(sched_active_thread->sm_entry));     \
    __asm__ volatile ("push r5");                                           \
    /* Put empty sr on stack */                                   \
    __asm__ volatile ("mov.w #8,r5");                                       \
    __asm__ volatile ("push r5");


/**
 * @brief   Save the current UNTRUSTED thread context from inside a protected ISR
 */
#define ___MACRO_SAVE_UNTRUSTED_CONTEXT                     \
    ___MACRO_SAVE_REGISTERS                                  \
    __asm__ ("mov.w r1,%0" : "=m"(sched_active_thread->sp)); \
    __asm__ ("mov r1, &__unprotected_sp");

/**
 * @brief   Restore the UNTRUSTED thread context from inside a protected ISR
 */
#define ___MACRO_RESTORE_UNTRUSTED_CONTEXT                        \
    __asm__ volatile ("mov.w %0,r1" : : "m"(sched_active_thread->sp)); \
    ___MACRO_RESTORE_REGISTERS                                      \
    __asm__ ("mov r1, &__unprotected_sp");


/**
 * @brief   Save the current thread context from inside a protected ISR
 * This means first checking whether the thread is an sm or not and then
 * executing the corresponding save macro.
 * Side effect: If r1 was zero, it will point to isr_stack after.
 */
#define ___MACRO_SAVE_CONTEXT                                               \
    /* If sm was interrupted, r1 might be 0*/                               \
    __asm__ volatile ("tst r1");                                            \
    __asm__ volatile ("jnz 1f");                                            \
    /* TODO: Technically, we want to delete this thread if it turns out that r1 was zero while it is not an SM. */\
    /*       otherwise, we would write the context to the ISR stack which will get overwritten on the next IRQ */ \
    /*       However we ignore this for now as it mostly just breaks the current thread with side effects */ \
    /* In that case, use isr stack */                                       \
    __asm__ volatile ("mov.w %0,r1" : : "i"(__isr_stack + ISR_STACKSIZE));  \
    /* push r15 to stack (old or new) */                                    \
    __asm__ volatile ("1: push r10");                                       \
    __asm__ volatile ("push r11");                                          \
    /* First: Check whether we interrupted ourself (the scheduler). */\
    /*        This may happen if we returned from the scheduler and the timer fires right away. */\
    /*        NOTE: After the sancus core changes in January 2021 this MAY never happen anymore */\
    __asm__ volatile ("bit #0x1, &__sm_sancus_sm_timer_ssa_sp");\
    __asm__ volatile ("jz 1f");\
            /* We indeed seem to have interrupted ourself. This never happens as the scheduler is */\
            /* non-interruptable. Thus, we can safely assume that we interrupted a freshly resumed thread*/\
            /* --> Check if active thread is SM or not. On SM do nothing and skip to end of this macro. */\
            /*     On untrusted, pop the SSA frame to the SP stored in the SSA. */                       \
            __asm__ volatile ("mov.w &%0, r11" : : "m"(sched_active_thread));           \
            __asm__ volatile ("mov.w 0(r11), r10");                                     \
            __asm__ volatile ("tst r10");                                               \
            __asm__ ("jnz 3f");                                                         \
                /* We interrupted ourselves after restoring an unprotected thread */\
                /* First, push PC and r2 to original stack */\
                __asm__ volatile ("mov &__sm_sancus_sm_timer_ssa_sp, r1");\
                __asm__ volatile ("mov &__sm_sancus_sm_timer_pc, r4");\
                __asm__ volatile ("mov &__sm_sancus_sm_timer_ssa_base-4, r5");\
                __asm__ volatile ("push r4");\
                __asm__ volatile ("push r5");\
                /* Then back up the SP again , Pop the SSA, and restore backed up r1 */\
                /*   This sets us up to perform the normal untrusted context save below.  */\
                __asm__ volatile ("mov r1, &__sm_sancus_sm_timer_ssa_sp");\
                __asm__ volatile ("mov &__sm_sancus_sm_timer_ssa_base-28, r1");\
                __asm__ volatile ("pop r4");\
                __asm__ volatile ("pop r5");\
                __asm__ volatile ("pop r6");\
                __asm__ volatile ("pop r7");\
                __asm__ volatile ("pop r8");\
                __asm__ volatile ("pop r9");\
                __asm__ volatile ("pop r10");\
                __asm__ volatile ("pop r11");\
                __asm__ volatile ("pop r12");\
                __asm__ volatile ("pop r13");\
                __asm__ volatile ("pop r14");\
                __asm__ volatile ("pop r15");\
                __asm__ volatile ("mov &__sm_sancus_sm_timer_ssa_sp, r1");\
                /* Lastly, clear the scheduler SSA frame */\
                __asm__ ("mov #0, &__sm_sancus_sm_timer_ssa_sp");\
                /* Now we are ready to jump ahead to save the untrusted context */\
                __asm__ ("jmp 2f");\
    __asm__ ("1:");\
    /* Check Bit 15 in R2 since it tells us whether we interrupted an SM.*/ \
    /*  We also need sched_active_thread for later, is_sm is first in that struct*/\
    __asm__ volatile ("mov.w &%0, r11" : : "m"(sched_active_thread));           \
    __asm__ volatile ("bit #0x8000, r2");                                          \
        /* Jump forward if we interrupted an SM */                                  \
        /* We mostly ignore the trusted context here */                         \
        /* In contrast to an untrusted context, we do not store anything    */  \
        /* The reason is that the SM will recognize an IRQ itself and would */  \
        /* simply be scheduled again from start if this is not an exit call. */ \
        /* If this is a sleep call, the timer will make sure to set 0xff and */ \
        /* put this thread to sleep. It will also take care of saving the */    \
        /* entry point if necessary.*/                                          \
    __asm__ ("jnz 1f");                                                         \
        /* Do untrusted context here */                                         \
        /* First, we store a 0 for the is_sm in active_thread */                \
        __asm__ volatile ("mov.w #0, 0(r11)");                                  \
        /* Untrusted context needs the r10 and r11 from stack that were pushed earlier*/\
        /* Note that not all traces perform this pop which is fine as we always revert SP*/\
        /* to the beginning of the ISR stack when entering*/\
        __asm__ ("pop r11");                                                    \
        __asm__ ("pop r10");                                                    \
        /* If the untrusted context will be saved from our SSA frame, we do not pop*/\
        /*r10 and 11 and can jump directly to the label here.*/\
        __asm__ ("2:");\
            ___MACRO_SAVE_UNTRUSTED_CONTEXT                                         \
            /* Jump forward to the end of this macro */                             \
            __asm__ ("jmp 3f");                                                     \
    /* Jump target for trusted context */                                   \
    __asm__ ("1:");                                                             \
        /** The only thing we do on trusted context safe is to update is_sm and store the untrusted sp*/  \
        __asm__ volatile ("mov.w #1, 0(r11)");                                      \
        __asm__ ("mov &__unprotected_sp, %0" : "=m"(sched_active_thread->sp));      \
    /* End jump target for untrusted part and also end of macro. */                  \
    __asm__ ("3:");


/**
 * @brief   Restore the current thread context from inside a protected ISR
 * This means first checking whether the scheduled thread is an sm or not and then
 * executing the corresponding restore macro
 */
#define ___MACRO_RESTORE_CONTEXT                                             \
    /* First, restore the untrusted SP */                                    \
    __asm__ volatile ("mov.w %0,&__unprotected_sp" : : "m"(sched_active_thread->sp));\
    /* Check whether we restore an sm. At this point we still allow the compiler to clobber registers */\
    __asm__ volatile ("mov.w %0, r15" : : "m"(sched_active_thread->is_sm));  \
    __asm__ volatile ("tst r15");                                            \
    /* restore either trusted or untrusted context */                        \
    __asm__ volatile ("jz 1f");                                              \
        ___MACRO_RESTORE_TRUSTED_CONTEXT                                     \
        __asm__ volatile ("jmp 2f");                                         \
    __asm__ volatile ("1:");                                                 \
        ___MACRO_RESTORE_UNTRUSTED_CONTEXT                                   \
    /* Finish with a reti */                                                 \
    /* We have to be extremely careful with the reti here as the scheduler has powers */\
    /*  that no other SM has: It controls the CPUOFF and the SCG1 bits. */     \
    /*  As such, we can not simply restore any R2, we have to manually disable */       \
    /*  them before a reti. */                              \
    __asm__ volatile("bic  %0, 0(r1)" : : "i"(CPUOFF));                                 \
    __asm__ volatile("bic  %0, 0(r1)" : : "i"(SCG1));                                   \
    __asm__ volatile ("2: reti");


/**
 * @brief   Run this code on entering interrupt routines
 * For the Sancus compiled code, we provide both the C function and the pure assembly macro.
 * While clang will only allow the pure assembly code, the c code may help in understanding.
 */
#define ___MACRO_ENTER_ISR_NO_CONTEXT_STORE                                 \
    __asm__ volatile ("mov #1, &%0"  : "=m"(__sm_irq_is_in));               \
    __asm__ volatile ("mov.w %0,r1" : : "i"(__isr_stack + ISR_STACKSIZE));
    
#define __MACRO_ENTER_ISR                                                   \
    ___MACRO_SAVE_CONTEXT                                                   \
    ___MACRO_ENTER_ISR_NO_CONTEXT_STORE


// static inline void __attribute__((always_inline)) __enter_isr(void)
// {
//     __save_context();
//     __asm__ volatile ("mov.w %0,r1" : : "i"(__isr_stack + ISR_STACKSIZE));
//     __sm_irq_is_in = 1;
// }

/**
 * @brief   Run this code on exiting interrupt routines
 * For the Sancus compiled code, we provide both the C function and the pure assembly macro.
 * While clang will only allow the pure assembly code, the c code may help in understanding.
 * Note: The assembly code ignores the R15 integer return value from the call sched_run line.
 * Note: Currently we do not clean up the isr stack because we assume scheduler interrupts 
 *       run without interrupts enabled.
 */
#define __MACRO_EXIT_ISR                                                     \
    /* Modification: Yield on all IRQs to enforce a round robin on same prio level*/\
    /*__asm__ volatile ("call %0" : : "i"(sched_yield));                     */\
    /*__asm__ volatile ("mov #1, &%0"  : "=m"(sched_context_switch_request));*/\
    /* Modification: end (potential duplication of r12 below) */\
    __asm__ volatile ("mov &%0, r12" : : "m"(sched_context_switch_request)); \
    __asm__ volatile ("tst r12");                                            \
    __asm__ volatile ("jz $+6");                                             \
    __asm__ volatile ("call %0" : : "i"(sched_run_internal));                \
    __asm__ volatile ("mov #0, &%0"  : "=m"(__sm_irq_is_in));                \
    ___MACRO_RESTORE_CONTEXT

// static inline void __attribute__((always_inline)) __exit_isr(void)
// {
//     __sm_irq_is_in = 0;
    
//     if (sched_context_switch_request) {
//         sched_run();
//     }

//     __restore_context();
// }

/**
 * @brief   Initialize the cpu
 */
void msp430_cpu_init(void);

/**
 * @brief   Print the last instruction's address
 *
 * @todo:   Not supported
 */
static inline void cpu_print_last_instruction(void)
{
    puts("n/a");
}

#define ___MACRO_UNREACHABLE \
__asm__(".UNREACHABLE:");      \
__asm__("jmp .UNREACHABLE");

#ifdef __cplusplus
}
#endif

#endif /* CPU_H */
/** @} */
