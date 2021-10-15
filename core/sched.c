/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     core_sched
 * @{
 *
 * @file
 * @brief       Scheduler implementation
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      René Kijewski <rene.kijewski@fu-berlin.de>
 *
 * @}
 */


#include <stdint.h>

#include "sched.h"
// #include "clist.h"
// #include "bitarithm.h"
#include "sm_irq.h"
#include "cpu.h"
#include "thread.h"
#include "log.h"
#include "secure_mintimer.h"


#ifdef MODULE_MPU_STACK_GUARD
#include "mpu.h"
#endif

#define ENABLE_DEBUG 0
#include "debug.h"

// #define SANCUS_DEBUG 1
#include "sancus_helpers.h"

#if ENABLE_DEBUG
/* For PRIu16 etc. */
#include <inttypes.h>
#endif

SM_DATA(sancus_sm_timer) volatile int sched_num_threads = 0;

SM_DATA(sancus_sm_timer) volatile unsigned int sched_context_switch_request;

SM_DATA(sancus_sm_timer) thread_t sched_threads[KERNEL_PID_LAST + 1];
SM_DATA(sancus_sm_timer) volatile thread_t *sched_active_thread = NULL;

SM_DATA(sancus_sm_timer) volatile kernel_pid_t sched_active_pid = KERNEL_PID_UNDEF;

SM_DATA(sancus_sm_timer) clist_node_t sched_runqueues[SCHED_PRIO_LEVELS];
SM_DATA(sancus_sm_timer) static uint32_t runqueue_bitcache = 0;

SM_DATA(sancus_sm_timer) const uint8_t max_threads = ARRAY_SIZE(sched_threads);

SM_DATA(sancus_sm_timer) bool initialization_done = false;
SM_DATA(sancus_sm_timer) volatile void *scheduler_entry;

// We need one scheduler specific timer that the scheduler can use to regain control again after a short time.
SM_DATA(sancus_sm_timer) secure_mintimer_t scheduler_timer;

#ifdef MODULE_SCHED_CB
static void (*sched_cb) (kernel_pid_t active_thread, kernel_pid_t next_thread) = NULL;
#endif

void SM_FUNC(sancus_sm_timer) scheduler_init(void){
    if(!initialization_done){
        // Perform initialization.

        // Initialize the timer
        secure_mintimer_init();

        // scheduler entry is needed when restoring protected threads
        scheduler_entry = SM_GET_ENTRY(sancus_sm_timer);

        // Set up the scheduler specific timer
        scheduler_timer.thread = NULL;
        scheduler_timer.next = NULL;

       initialization_done = true;
    }
}

/**
 * Helper functions for scheduling
 * Namely, clist and bitarithm
 * 
 * */

/**
 * This is a code duplication of the bitarithm.h code. Technically we could also 
 * define it as a MACRO there
 * */
static inline unsigned SM_FUNC(sancus_sm_timer) bitarithm_lsb_sm_timer(unsigned v){
/* Source: http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogObvious */
    unsigned r = 0;

    while ((v & 0x01) == 0) {
        v >>= 1;
        r++;
    };

    return r;
}

/**
 * @brief Appends *new_node* at the end of *list
 *
 * @note Complexity: O(1)
 *
 * @param[in,out]   list        Pointer to clist
 * @param[in,out]   new_node    Node which gets inserted.
 *                              Must not be NULL.
 */
static inline void SM_FUNC(sancus_sm_timer) sm_clist_rpush(clist_node_t *list, clist_node_t *new_node)
{
    if (list->next) {
        new_node->next = list->next->next;
        list->next->next = new_node;
    }
    else {
        new_node->next = new_node;
    }
    list->next = new_node;
}


/**
 * @brief Removes and returns first element from list
 *
 * @note Complexity: O(1)
 *
 * @param[in,out]   list        Pointer to the *list* to remove first element
 *                              from.
 */
static inline clist_node_t* SM_FUNC(sancus_sm_timer) sm_clist_lpop(clist_node_t *list)
{
    if (list->next) {
        clist_node_t *first = list->next->next;
        if (list->next == first) {
            list->next = NULL;
        }
        else {
            list->next->next = first->next;
        }
        return first;
    }
    else {
        return NULL;
    }
}

/**
 * @brief Advances the circle list.
 *
 * The result of this function is will be a list with
 * nodes shifted by one. So second list entry will be
 * first, first is last.
 *
 * [ A, B, C ] becomes [ B, C, A ]
 *
 * @note Complexity: O(1)
 *
 * @param[in,out]   list        The list to work upon.
 */
static inline void SM_FUNC(sancus_sm_timer) sm_clist_lpoprpush(clist_node_t *list)
{
    if (list->next) {
        list->next = list->next->next;
    }
}

void SM_FUNC(sancus_sm_timer) periodic_thread_schedule_next_timer(thread_t *periodic_thread, uint32_t current_time){
    
    // Increment last_reference until it is in the future (but avoiding infinite loops on overflows)
    while (periodic_thread->last_reference < current_time 
        && current_time - periodic_thread->last_reference < current_time){
        periodic_thread->last_reference += periodic_thread->period;
    }
    if(periodic_thread->last_reference < current_time){
        //TODO: CATCH 32 bit overflows
        sancus_debug("ERROR: 32 bit overflow for periodic thread");
    }
    // Set timer to that next reference
    secure_mintimer_t* timer = get_available_timer(periodic_thread->pid);
    // timer->target = (uint32_t) periodic_thread->last_reference;
    // timer->long_target = (uint32_t) (periodic_thread->last_reference >> 32 );
    // timer->thread = periodic_thread;
    sched_set_status(periodic_thread, STATUS_SLEEPING);
    // _secure_mintimer_set_absolute_explicit( timer, current_time);
    _secure_mintimer_set_absolute(timer, periodic_thread->last_reference);
    

    // Also reset the original idx
    periodic_thread->sm_idx = periodic_thread->original_idx;
    periodic_thread->last_runtime = 0;
}

int SM_FUNC(sancus_sm_timer) __attribute__((used)) sched_run_internal(void)
{
    int changed_thread = 0;

    sancus_debug("Sched_run!");

    __disable_irq();

    thread_t *active_thread = (thread_t *)sched_active_thread;

    // If we were executing a period job, check whether this job has run out of its limit (only if it did not yield)
    if( active_thread 
        && active_thread->status == STATUS_RUNNING  // Do not continue threads that want to exit and removed themselves.
        && active_thread->priority == SCHED_PERIODIC_PRIO_LEVEL 
        && !sched_context_switch_request){
        // We interrupted a periodic thread. let's check whether this should put it to sleep again
        uint32_t current_time, long_term;
        _secure_mintimer_now_internal(&current_time, &long_term);
        uint32_t runtime = active_thread->last_runtime - SCHEDULER_OVERHEAD_RUN;
        
        // We ignore 32 bit overflows here for now...
        // TODO: Add 32 bit handling
        runtime += (current_time - active_thread->last_reference);
        
        // Check whether thread is done
        if(runtime >= active_thread->runtime){
            // This periodic job is done for this period. Put it to sleep.
            periodic_thread_schedule_next_timer(active_thread, current_time);
        } else {
            // We will be scheduling this periodic thread again. Update last runtime and keep going
            active_thread->last_runtime = runtime;
            goto end;
        }
    }

    // Only now reset the switch context
    sched_context_switch_request = 0;

    /* Check the bitmask for the next thread to run
     * The bitmask in runqueue_bitcache is never empty,
     * since the threading should not be started before at least the idle thread was started.
     */
    int nextrq = bitarithm_lsb_sm_timer(runqueue_bitcache);
    thread_t *next_thread = container_of(sched_runqueues[nextrq].next->next, thread_t, rq_entry);

    sancus_debug2("sched_run: active thread: %" PRIkernel_pid ", next thread: %" PRIkernel_pid "",
          (kernel_pid_t)((active_thread == NULL) ? KERNEL_PID_UNDEF : active_thread->pid),
          next_thread->pid);

    if (active_thread == next_thread) {
        sancus_debug("sched_run: done, sched_active_thread was not changed.");
        goto end;
    }

    if (active_thread) {
        if (active_thread->status == STATUS_RUNNING) {
            active_thread->status = STATUS_PENDING;
        }
    }

#ifdef MODULE_SCHED_CB
    if (sched_cb) {
        /* Use `sched_active_pid` instead of `active_thread` since after `sched_task_exit()` is
           called `active_thread` is set to NULL while `sched_active_thread` isn't updated until
           `next_thread` is scheduled*/
        sched_cb(sched_active_pid, next_thread->pid);
    }
#endif

    changed_thread = 1;
    next_thread->status = STATUS_RUNNING;
    sched_active_pid = next_thread->pid;
    sched_active_thread = (volatile thread_t *) next_thread;
    sancus_debug("sched_run: done, changed sched_active_thread.");

end:
    if(sched_active_thread->priority == SCHED_PERIODIC_PRIO_LEVEL){
        // Rotate periodic clist
        sm_clist_lpoprpush(&sched_runqueues[SCHED_PERIODIC_PRIO_LEVEL]);

        // We will schedule a periodic job next. Get timestamp for last_reference
        uint32_t short_term, long_term;
        _secure_mintimer_now_internal(&short_term, &long_term);

        // Set scheduler timer to interrupt this periodic job after its runtime. 
        // We use the scheduler specific timer for that
        scheduler_timer.target = short_term + sched_active_thread->runtime - sched_active_thread->last_runtime;
        scheduler_timer.long_target = long_term;
        _secure_mintimer_set_absolute_explicit( &scheduler_timer, short_term);
        // _secure_mintimer_set_absolute(&scheduler_timer, sched_active_thread->runtime - sched_active_thread->last_runtime);
    }
    return changed_thread;
}

/**
 * The sole reason of this entry function is so that we are able to use the sched_run function
 * from other locations inside the SM (good for performance)
 * */
int SM_ENTRY(sancus_sm_timer) __attribute__((used)) sched_run(void){
    return sched_run_internal();
}

void SM_FUNC(sancus_sm_timer) sched_set_status(thread_t *process, thread_status_t status)
{
    if (status >= STATUS_ON_RUNQUEUE) {
        if (!(process->status >= STATUS_ON_RUNQUEUE)) {
            sancus_debug2("sched_set_status: adding thread %" PRIkernel_pid " to runqueue %" PRIu8 ".",
                  process->pid, process->priority);
            sm_clist_rpush(&sched_runqueues[process->priority], &(process->rq_entry));
            runqueue_bitcache |= 1 << process->priority;
        }
    }
    else {
        if (process->status >= STATUS_ON_RUNQUEUE) {
            sancus_debug2("sched_set_status: removing thread %" PRIkernel_pid " from runqueue %" PRIu8 ".",
                  process->pid, process->priority);
            sm_clist_lpop(&sched_runqueues[process->priority]);

            if (!sched_runqueues[process->priority].next) {
                runqueue_bitcache &= ~(1 << process->priority);
            }
        }
    }
    sancus_debug3("sched_set_status: changed priority of thread %u from %u to %u.",
                  process->pid, process->status, status);
    process->status = status;
}

void SM_ENTRY(sancus_sm_timer) sched_set_own_status(thread_status_t status){
    thread_t *me = (thread_t*)sched_active_thread;
    sched_set_status(me, status);
}

void sched_switch(USED_IN_ASM uint16_t other_prio)
{   
    // move other_prio which is in r15 into r13
    // This is required as exitless_entry needs it as a third argument
    __asm__("mov r15, r13");

    //store pc as continue label
    __asm__ ("mov #yield_higher_continue, r10");

    // perform a full save context and exitless call
    ___MACRO_EXITLESS_CALL_WITH_RESUME(EXITLESS_FUNCTION_TYPE_SCHED_SWITCH)

    __asm__ ("yield_higher_continue:");

    return;
}

void SM_FUNC(sancus_sm_timer) sched_switch_internal_allow_yield(uint16_t other_prio, bool yield_allowed)
{
    thread_t *active_thread = (thread_t *) sched_active_thread;
    uint16_t current_prio = active_thread->priority;
    int on_runqueue = (active_thread->status >= STATUS_ON_RUNQUEUE);

    sancus_debug1("sched_switch: active pid=%" PRIkernel_pid "", active_thread->pid);
    sancus_debug3(" prio=%" PRIu16 " on_runqueue=%i, other_prio=%" PRIu16 "",
          current_prio, on_runqueue, other_prio);

    if (!on_runqueue || (current_prio > other_prio)) {
        sched_context_switch_request = 1;
        if (sm_irq_is_in()) {
            sancus_debug("sched_switch: only setting sched_context_switch_request.");
        }
        else if(yield_allowed){
            sancus_debug("sched_switch: yielding immediately.");
            thread_yield_higher_internal(true);
        }
    }
    else {
        sancus_debug("sched_switch: continuing without yield.");
    }
}

void SM_FUNC(sancus_sm_timer) sched_switch_internal(uint16_t other_prio){
    sched_switch_internal_allow_yield(other_prio, true);
}

/**
 * This is a replacement for the old thread_yield that was in thread.c.
 * It simply places the thread back on the runqueue. After this, yield_higher should be called.
 * */
void SM_FUNC(sancus_sm_timer) sched_yield(void){
    thread_t *me = (thread_t *)sched_active_thread;
    if (me != NULL && me->status >= STATUS_ON_RUNQUEUE) {
        sm_clist_lpoprpush(&sched_runqueues[me->priority]);
    }

    if(me->priority == SCHED_PERIODIC_PRIO_LEVEL){
        // Disable the pending scheduler timer
        secure_mintimer_remove(&scheduler_timer);

        // A periodic thread is sleeping --> Schedule next wakeup by making it sleep
        uint32_t short_term, long_term;
        _secure_mintimer_now_internal(&short_term, &long_term);
        periodic_thread_schedule_next_timer(me, short_term);

        sched_context_switch_request = 1;
    }
}

void SM_FUNC(sancus_sm_timer) sched_task_exit_internal(void){

    if(sched_active_pid != KERNEL_PID_UNDEF){
        sancus_debug1("sched_task_exit: ending thread %" PRIkernel_pid "...\n", sched_active_thread->pid);
    
        sched_threads[sched_active_pid].in_use = 0;
        
        sched_num_threads--;

        sched_set_status((thread_t *)sched_active_thread, STATUS_STOPPED);

        sched_active_thread = NULL;
    }
}

/**
 * Optional function to shut down
 * */
void SM_ENTRY(sancus_sm_timer) sched_shut_down(){
    __asm__("bis %0,r2" : : "i"(CPUOFF)); // With restrictions on G2 this won't work.
}

NORETURN void sched_task_exit(void)
{
    // (void) irq_disable();
    // sched_task_exit is now called inside cpu_switch_context_exit
    // sched_task_exit_internal();
    cpu_switch_context_exit();
}

#ifdef MODULE_SCHED_CB
void sched_register_cb(void (*callback)(kernel_pid_t, kernel_pid_t))
{
    sched_cb = callback;
}
#endif
