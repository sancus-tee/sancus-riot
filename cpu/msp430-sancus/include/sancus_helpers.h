/*
 * Copyright (C) 2014 INRIA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_msp430_common
 * @{
 *
 * @file
 * @brief       time.h for msp430
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/assert.h.html
 *
 * @author      Martine Lenders <mlenders@inf.fu-berlin.de>
 */

#ifndef SANCUS_HELPERS_H
#define SANCUS_HELPERS_H

#include <stdio.h>
#include "log.h"
#include <sancus/sm_support.h>
#include <sancus_support/sm_io.h>
#include "sched.h"
#include "cpu.h"

#include "periph/timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This is a debug function useful to debug threads.
 * Normally, the following structs are protected by the scheduler via the SM_DATA primitive.
 * However, if one removes this protection, the below function can be useful to debug thread issues.
 * sched_threads
 * DANGER: In normal scheduler mode, this function will result in an SM VIOLATION
 * */
void print_thread_struct(kernel_pid_t pid);

// Core Riot function to enable all riot SMs
void riot_enable_sm(struct SancusModule* sm);

/* Secure Scheduler Exitless call macros 
 * These Macros are meant to be called from whithin SMs.
 * Since SMs can not rely on unprotected functions to uphold availability
 *  guarantees, we need to ensure that we jump directly from the SMs to the scheduler.
 * We do this with Macros that directly call into the scheduler and perform one of the exitless calls.
*/
#define ___MACRO_CALL_SLEEP_FROM_SM(offset_lsb, offset_msb, sm)      \
    /* move offset into r13 and r12 */                               \
    __asm__ ("push r12");                                             \
    __asm__ ("push r13");                                             \
    __asm__("mov.w %0, r12" : : "i"(offset_lsb));                    \
    __asm__("mov.w %0, r13" : : "i"(offset_msb));                    \
    /* perform a full save context and exitless call */              \
    ___MACRO_PREPARE_EXITLESS_CALL_FROM_SM_COMMON(EXITLESS_FUNCTION_TYPE_SLEEP, sm)\
    __asm__ ("pop r13");                                             \
    __asm__ ("pop r12");                                             

#define ___MACRO_CALL_THREAD_YIELD_FROM_SM(sm)      \
    ___MACRO_PREPARE_EXITLESS_CALL_FROM_SM_COMMON(EXITLESS_FUNCTION_TYPE_YIELD, sm)

#define ___MACRO_CALL_THREAD_EXIT_FROM_SM(sm)      \
    ___MACRO_PREPARE_EXITLESS_CALL_FROM_SM_COMMON(EXITLESS_FUNCTION_TYPE_EXIT, sm)


// This macro can help to debug issues with the timer. Comment it out to enable protection on the timer.
// and leave it in to disable protections on the timer and enable the idle threat to print out the current timers.
// #define DEBUG_TIMER

// Manual scheduler boot option to allow for attestation setups
// #define MANUAL_SCHEDULER_BOOT

#define SANCUS_COLOR_RED    "\033[1;31m"
#define SANCUS_COLOR_RESET  "\033[0m"
#define SANCUS_COLOR_YELLOW "\033[1;33m"
#define SANCUS_COLOR_WHITE  "\033[1m"
#define SANCUS_COLOR_GREEN  "\033[0;32m"

#define DEBUG_STR(str)      SANCUS_COLOR_YELLOW  "[" __FILE__ "] " str SANCUS_COLOR_RESET "\n"
#define WARN_STR(str)       SANCUS_COLOR_RED     "[" __FILE__ "] " str SANCUS_COLOR_RESET "\n"
#define INFO_MSG_STR(str)   SANCUS_COLOR_WHITE   "[" __FILE__ "] " str SANCUS_COLOR_RESET "\n"
#define SUCC_STR(str)       SANCUS_COLOR_GREEN   "[" __FILE__ "] " str SANCUS_COLOR_RESET "\n"
/* 
 * Sancus debug functions
 * */
#ifndef SANCUS_DEBUG
#define SANCUS_DEBUG (0)
#endif

/**
 * @brief Default log_write function, just maps to printf
 */
#define sancus_debug(str)                if (SANCUS_DEBUG && sched_active_thread != NULL)  printf0(DEBUG_STR(str))
#define sancus_debug1(str, a1)           if (SANCUS_DEBUG && sched_active_thread != NULL)  printf1(DEBUG_STR(str), a1)
#define sancus_debug2(str, a1, a2)       if (SANCUS_DEBUG && sched_active_thread != NULL)  printf2(DEBUG_STR(str), a1, a2)
#define sancus_debug3(str, a1, a2, a3)   if (SANCUS_DEBUG && sched_active_thread != NULL)  printf3(DEBUG_STR(str), a1, a2, a3)

#define sancus_error(str)                if (SANCUS_DEBUG && sched_active_thread != NULL)  printf0( WARN_STR(str))
#define sancus_error1(str)               if (SANCUS_DEBUG && sched_active_thread != NULL)  printf0( WARN_STR(str), a1)
#define sancus_error2(str)               if (SANCUS_DEBUG && sched_active_thread != NULL)  printf0( WARN_STR(str), a1, a2)
#define sancus_error3(str)               if (SANCUS_DEBUG && sched_active_thread != NULL)  printf0( WARN_STR(str), a1, a2, a3)

#define sancus_info(str)                 if (SANCUS_DEBUG && sched_active_thread != NULL)  printf0( INFO_MSG_STR(str))
#define sancus_info1(str)                if (SANCUS_DEBUG && sched_active_thread != NULL)  printf0( INFO_MSG_STR(str), a1)
#define sancus_info2(str)                if (SANCUS_DEBUG && sched_active_thread != NULL)  printf0( INFO_MSG_STR(str), a1, a2)
#define sancus_info3(str)                if (SANCUS_DEBUG && sched_active_thread != NULL)  printf0( INFO_MSG_STR(str), a1, a2, a3)

#define sancus_success(str)                 if (SANCUS_DEBUG && sched_active_thread != NULL)  printf0( SUCC_STR(str))
#define sancus_success1(str)                if (SANCUS_DEBUG && sched_active_thread != NULL)  printf0( SUCC_STR(str), a1)
#define sancus_success2(str)                if (SANCUS_DEBUG && sched_active_thread != NULL)  printf0( SUCC_STR(str), a1, a2)
#define sancus_success3(str)                if (SANCUS_DEBUG && sched_active_thread != NULL)  printf0( SUCC_STR(str), a1, a2, a3)


#ifdef __cplusplus
}
#endif


#endif /* SANCUS_HELPERS_H */
/** @} */
