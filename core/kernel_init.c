/*
 * Copyright (C) 2016 Kaspar Schleiser <kaspar@schleiser.de>
 *               2013 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     core_internal
 * @{
 *
 * @file
 * @brief       Platform-independent kernel initilization
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 *
 * @}
 */

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include "kernel_init.h"
#include "thread.h"
#include "irq.h"
#include "log.h"
#include "sancus_helpers.h"
#include "evaluation_helper.h"

#include "periph/pm.h"

#ifdef DEBUG_TIMER
#define ENABLE_DEBUG (1)
#else 
#define ENABLE_DEBUG (0)
#endif
#include "debug.h"

#ifdef MODULE_AUTO_INIT
#include <auto_init.h>
#endif

#ifdef DEBUG_TIMER
/**
 * This is a debugging function useful to debug timers. Usually, the timers are protected 
 * by the scheduler, however if one removes the SM_DATA restriction for:
 * secure_mintimer_timer_list
 * timer_list_head
 * overflow_list_head
 * long_list_head
 * long_cnt
 * _secure_mintimer_high_cnt
 * Then one can debug with the below functions.
 * DANGER: In normal scheduler mode, this function will result in an SM VIOLATION
 */
#include "secure_mintimer.h"
#define TIMER_TYPE_NORMAL 0
#define TIMER_TYPE_LONG   1
void print_timer_struct(secure_mintimer_t *timer, int type){
    switch (type)
    {
    case TIMER_TYPE_LONG:
        LOG_DEBUG("Long timer: \n");
        break;
    
    default:
        LOG_DEBUG("Timer: \n");
        break;
    }
    LOG_DEBUG("    Target: %lu\n", timer->target);
    LOG_DEBUG("    Long Target: %lu\n", timer->long_target);
    // LOG_DEBUG("    Thread PID: %u\n", timer->thread->pid);
}
/**
 * End debug functions.
 * */
#endif

extern int main(void);
static void *main_trampoline(void *arg)
{
    (void) arg;

#ifdef MODULE_AUTO_INIT
    auto_init();
#endif

    LOG_INFO("main(): This is RIOT! (Version: " RIOT_VERSION ")\n");

    main();
    return NULL;
}

static void *idle_thread(void *arg)
{
    (void) arg;
    #ifdef EVALUATION_ENABLED
    ___MACRO_END_TIMING
    #endif
    #ifdef DEBUG_TIMER
    int i = 0;
    secure_mintimer_t *timer = timer_list_head;
    secure_mintimer_t *long_timer = long_list_head;
    #endif

    while (1) {
        #ifdef DEBUG_TIMER
        #ifdef EVALUATION_ENABLED
        ___MACRO_END_TIMING
        LOG_DEBUG("[Idle thread] tick. Long count:%lu, High count:%lu, Timer: %u\n", _long_cnt, _secure_mintimer_high_cnt, TIMER_A->R);
        #else
        LOG_DEBUG("[Idle thread] tick. Time:%llu, Long count:%lu, High count:%lu\n", secure_mintimer_now_usec64(), _long_cnt, _secure_mintimer_high_cnt);
        #endif
        if(timer_list_head == NULL){
            puts("[Idle thread] Timer list head is NULL");
        }
        while(timer != NULL){
            print_timer_struct(timer, TIMER_TYPE_NORMAL);
            if(timer->next != NULL) timer = timer->next;
            else timer = NULL;
        }
        if(long_list_head == NULL){
            puts("[Idle thread] Long timer list head is NULL");
        }
        while(long_timer != NULL){
            print_timer_struct(long_timer, TIMER_TYPE_LONG);
            if(long_timer->next != NULL) long_timer = long_timer->next;
            else long_timer = NULL;
        }
        // print_thread_struct(1);
        // print_thread_struct(2);
        while(i<1000){i++;}

        i = 0;
        timer = timer_list_head;
        long_timer = long_list_head;
        #endif
        // thread_yield_higher();
        // By default, the idle threat just loops the pm_set_lowest CPU dependent instruction.
        pm_set_lowest();
    }


    return NULL;
}

const char *main_name = "main";
const char *idle_name = "idle";

static char main_stack[THREAD_STACKSIZE_MAIN];
static char idle_stack[THREAD_STACKSIZE_IDLE];

void kernel_init(void)
{
    // (void) irq_disable();
    LOG_INFO("Creating idle thread...\n");
    thread_create(idle_stack, sizeof(idle_stack),
            THREAD_PRIORITY_IDLE,
            THREAD_CREATE_WOUT_YIELD | THREAD_CREATE_STACKTEST,
            idle_thread, NULL, idle_name);

    LOG_INFO("Creating main thread...\n");
    thread_create(main_stack, sizeof(main_stack),
            THREAD_PRIORITY_MAIN,
            THREAD_CREATE_WOUT_YIELD | THREAD_CREATE_STACKTEST,
            main_trampoline, NULL, main_name);

    LOG_INFO("Kernel init done. Booting scheduler and switching context.\n");
    
    // Usually, boot normally. But sometimes we may want to delay boot and do it manually
    #ifdef MANUAL_SCHEDULER_BOOT
    LOG_INFO("Manual boot requested. Only yielding to main, not initializing scheduler yet.\n");
    ___MACRO_PREPARE_EXITLESS_CALL(EXITLESS_FUNCTION_TYPE_YIELD)
    UNREACHABLE();
    #else 
    scheduler_kernel_init();
    #endif
}
