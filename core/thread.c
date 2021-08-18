/*
 * Copyright (C) 2013 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     core_thread
 * @{
 *
 * @file
 * @brief       Threading implementation
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 *
 * @}
 */

#include <errno.h>
#include <stdio.h>

#include "assert.h"
#include "thread.h"
#include "sm_irq.h"
#include "irq.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"
#include "bitarithm.h"
#include "sched.h"
#include "secure_mintimer.h"

volatile thread_t* SM_FUNC(sancus_sm_timer) thread_get(kernel_pid_t pid)
{
    if (pid_is_valid(pid)) {
        return &sched_threads[pid];
    }
    return NULL;
}

thread_status_t SM_ENTRY(sancus_sm_timer) thread_getstatus(kernel_pid_t pid)
{
    volatile thread_t *thread = thread_get(pid);
    return thread ? thread->status : STATUS_NOT_FOUND;
}

/**
 * @brief Returns the process ID of the currently running thread
 *
 * @return          obviously you are not a golfer.
 */
kernel_pid_t SM_ENTRY(sancus_sm_timer) thread_getpid(void)
{
    return sched_active_pid;
}

void thread_sleep(void)
{
    if (irq_is_in()) {
        return;
    }

    unsigned state = irq_disable();
    sched_set_own_status(STATUS_SLEEPING);
    irq_restore(state);
    thread_yield_higher();
}

int SM_ENTRY(sancus_sm_timer) thread_wakeup(kernel_pid_t pid)
{
    sancus_debug1("thread_wakeup: Trying to wakeup PID %" PRIkernel_pid "...\n", pid);

    unsigned old_state = sm_irq_disable();

    thread_t *thread = (thread_t *) thread_get(pid);

    if (!thread) {
        sancus_debug("thread_wakeup: Thread does not exist!\n");
    }
    else if (thread->status == STATUS_SLEEPING) {
        sancus_debug("thread_wakeup: Thread is sleeping.\n");

        sched_set_status(thread, STATUS_RUNNING);

        sm_irq_restore(old_state);
        // TODO: Switch would break caller, but we can just set status and wait until it is scheduled.
        // sched_switch(thread->priority);

        return 1;
    }
    else {
        sancus_debug("thread_wakeup: Thread is not sleeping!\n");
    }

    sm_irq_restore(old_state);
    return (int)STATUS_NOT_FOUND;
}

void thread_yield(void)
{
    thread_yield_higher();
}

void SM_FUNC(sancus_sm_timer) thread_add_to_list(list_node_t *list, thread_t *thread)
{
    assert (thread->status < STATUS_ON_RUNQUEUE);

    uint16_t my_prio = thread->priority;
    list_node_t *new_node = (list_node_t*)&thread->rq_entry;

    while (list->next) {
        thread_t *list_entry = container_of((clist_node_t*)list->next, thread_t, rq_entry);
        if (list_entry->priority > my_prio) {
            break;
        }
        list = list->next;
    }

    new_node->next = list->next;
    list->next = new_node;
}

kernel_pid_t SM_FUNC(sancus_sm_timer) _thread_create_scheduler_internal(uint8_t priority, bool is_sm, char* thread_sp_init){
    kernel_pid_t pid = KERNEL_PID_UNDEF;
    for (kernel_pid_t i = KERNEL_PID_FIRST; i <= KERNEL_PID_LAST; ++i) {
        // sancus_debug2("Checking pid %i of max pids %i -- ", i, KERNEL_PID_LAST);
        // sancus_debug2("sched_threads is at %p and contains %x",(int) &sched_threads[i], sched_threads[i].status);
        if (sched_threads[i].in_use == 0) { // Check whether thread is unused
            pid = i;
            break;
        } else {
            sancus_debug2("thread_create: PID %i is already registered with stack at %p", i, (int)sched_threads[i].sp);
        }
    }

    sancus_debug3("thread_create: Found unused PID and registered new thread with PID %i, priority %i and stack at %p", pid, priority, (int) thread_sp_init);

    if (pid == KERNEL_PID_UNDEF) {
        sancus_debug("thread_create(): too many threads!");
        return -EOVERFLOW;
    }


    sched_threads[pid].in_use = 1;
    sched_threads[pid].pid = pid;
    sched_threads[pid].priority = priority;
    sched_threads[pid].is_sm = is_sm;
    sched_threads[pid].sp = thread_sp_init;
    sched_threads[pid].rq_entry.next = NULL;
    
    sched_num_threads++;
    sched_set_status(&sched_threads[pid], STATUS_PENDING);
    
    return pid;
}

/**
 * Takes a priority, an unprotected stack pointer (for OCALLs), and the SM address plus its entry idx.
 * */
kernel_pid_t SM_ENTRY(sancus_sm_timer) thread_create_protected_in_scheduler(
            uint8_t priority, 
            char* thread_sp_init, 
            void* thread_entry, 
            entry_idx thread_idx){

    kernel_pid_t pid = _thread_create_scheduler_internal(priority, true, thread_sp_init);
    if(pid == KERNEL_PID_UNDEF){
        return pid;
    }

    // The entry point IDX as can be retrieved by e.g. SM_GET_ENTRY_IDX in sm_support.
    // Is set to 0xffff on interrupts.
    sched_threads[pid].sm_idx = thread_idx;
    // In case of an sm, we use the SP as the static entry pointer to branch to
    sched_threads[pid].sm_entry = thread_entry;
    
    return pid;

}

/**
 * Takes a priority and a stack pointer in unprotected memory to register an unprotected thread.
 * */
kernel_pid_t SM_ENTRY(sancus_sm_timer) thread_create_unprotected_in_scheduler(uint8_t priority, char* thread_sp_init){
    if (priority < SCHED_MAX_PRIO_LEVEL_UNPROTECTED) {
        return -EINVAL;
    }
    
    kernel_pid_t pid = _thread_create_scheduler_internal(priority, false, thread_sp_init);
    return pid;
}

kernel_pid_t thread_create(char *stack, int stacksize, uint8_t priority, int flags, thread_task_func_t function, void *arg, const char *name)
{
    if (priority < SCHED_MAX_PRIO_LEVEL_UNPROTECTED) {
        return -EINVAL;
    }
    // unsigned int state = irq_disable();
    /* align the stack on a 16/32bit boundary */
    uintptr_t misalignment = (uintptr_t) stack % ALIGN_OF(void *);
    if (misalignment) {
        misalignment = ALIGN_OF(void *) - misalignment;
        stack += misalignment;
        stacksize -= misalignment;
    }

    /* round down the stacksize to a multiple of thread_t alignments (usually 16/32bit) */
    stacksize -= stacksize % ALIGN_OF(thread_t);

    if (stacksize < 0) {
        DEBUG("thread_create: stacksize is too small!\n");
    }

    char* thread_sp_init = thread_stack_init(function, arg, stack, stacksize);

    kernel_pid_t pid = thread_create_unprotected_in_scheduler(priority, thread_sp_init);
    DEBUG("Created thread '%s'. PID: %" PRIkernel_pid ". Priority: %u. Stack with size %x starts at address %p\n", name, pid, priority, stacksize, thread_sp_init);

    // irq_restore(state);
    if (!(flags & THREAD_CREATE_WOUT_YIELD)) {
        sched_switch(priority);
    }

// #ifdef MODULE_CORE_MSG
//     thread->wait_data = NULL;
//     thread->msg_waiters.next = NULL;
//     cib_init(&(thread->msg_queue), 0);
//     thread->msg_array = NULL;
// #endif

    return pid;
}

kernel_pid_t thread_create_protected(
        char *unprotected_stack, 
        int unprotected_stack_size, 
        uint8_t priority, 
        int flags, 
        void *sm_entry, 
        entry_idx sm_idx, 
        const char *name)
{
    // unsigned int state = irq_disable();
    /* align the stack on a 16/32bit boundary */
    uintptr_t misalignment = (uintptr_t) unprotected_stack % ALIGN_OF(void *);
    if (misalignment) {
        misalignment = ALIGN_OF(void *) - misalignment;
        unprotected_stack += misalignment;
        unprotected_stack_size -= misalignment;
    }

    /* round down the stacksize to a multiple of thread_t alignments (usually 16/32bit) */
    unprotected_stack_size -= unprotected_stack_size % ALIGN_OF(thread_t);

    if (unprotected_stack_size < 0) {
        DEBUG("thread_create: unprotected_stack_size is too small!\n");
    }

    char* thread_sp_init = thread_unprotected_stack_init(unprotected_stack, unprotected_stack_size);

    kernel_pid_t pid = thread_create_protected_in_scheduler(priority, thread_sp_init, sm_entry, sm_idx);
    DEBUG("Created protected thread '%s'. PID: %" PRIkernel_pid 
        ". Priority: %u. unprotected_stack with unprotected_stack_size %x" 
        " starts at address %p\n", name, pid, priority, unprotected_stack_size, thread_sp_init);

    if (!(flags & THREAD_CREATE_WOUT_YIELD)) {
        sched_switch(priority);
    }

// #ifdef MODULE_CORE_MSG
//     thread->wait_data = NULL;
//     thread->msg_waiters.next = NULL;
//     cib_init(&(thread->msg_queue), 0);
//     thread->msg_array = NULL;
// #endif

    return pid;
}

void SM_ENTRY(sancus_sm_timer) thread_change_to_periodical(kernel_pid_t pid, uint16_t runtime, uint32_t period){

    sched_set_status(&sched_threads[pid], STATUS_SLEEPING);
    sched_threads[pid].priority = SCHED_PERIODIC_PRIO_LEVEL;
    sched_threads[pid].period = period;

    uint32_t short_term, long_term;
    _secure_mintimer_now_internal(&short_term, &long_term);
    sched_threads[pid].last_reference = short_term;
    
    sched_threads[pid].last_runtime = 0;
    sched_threads[pid].runtime = runtime;
    sched_threads[pid].original_idx = sched_threads[pid].sm_idx; // store original idx for later
    
    _secure_mintimer_tsleep_specific_pid(period, pid);
    
}