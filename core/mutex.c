/*
 * Copyright (C) 2015 Kaspar Schleiser <kaspar@schleiser.de>
 *               2013 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     core_sync
 * @{
 *
 * @file
 * @brief       Kernel mutex implementation
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Joakim Nohlgård <joakim.nohlgard@eistec.se>
 *
 * @}
 */

#include <stdio.h>
#include <inttypes.h>

#include "mutex.h"
#include "thread.h"
#include "sched.h"
#include "list.h"


#define ENABLE_DEBUG    (0)
#include "debug.h"

int SM_FUNC(sancus_sm_timer) _mutex_lock(mutex_t *mutex, int blocking)
{

    sancus_debug1("PID[%" PRIkernel_pid "]: Mutex in use.\n", sched_active_pid);

    if (mutex->queue.next == NULL) {
        /* mutex is unlocked. */
        mutex->queue.next = mutex_LOCKED;
        sancus_debug1("PID[%" PRIkernel_pid "]: mutex_wait early out.\n",
              sched_active_pid);
        return 1;
    }
    else if (blocking) {
        thread_t *me = (thread_t*)sched_active_thread;
        sancus_debug2("PID[%" PRIkernel_pid "]: Adding node to mutex queue: prio: %"
              PRIu32 "\n", sched_active_pid, (uint32_t)me->priority);
        sched_set_status(me, STATUS_MUTEX_BLOCKED);
        if (mutex->queue.next == mutex_LOCKED) {
            mutex->queue.next = (list_node_t*)&me->rq_entry;
            mutex->queue.next->next = NULL;
        }
        else {
            thread_add_to_list(&mutex->queue, me);
        }
        thread_yield_higher();
        /* We were woken up by scheduler. Waker removed us from queue.
         * We have the mutex now. */
        return 1;
    }
    else {
        return 0;
    }
}

void SM_FUNC(sancus_sm_timer) mutex_unlock(mutex_t *mutex)
{

    sancus_debug1("mutex_unlock():pid: %" PRIkernel_pid "\n",
          sched_active_pid);

    if (mutex->queue.next == NULL) {
        /* the mutex was not locked */
        return;
    }

    if (mutex->queue.next == mutex_LOCKED) {
        mutex->queue.next = NULL;
        /* the mutex was locked and no thread was waiting for it */
        return;
    }

    list_node_t *next = list_remove_head(&mutex->queue);

    thread_t *process = container_of((clist_node_t*)next, thread_t, rq_entry);

    sancus_debug2("mutex_unlock: waking up waiting thread %" PRIkernel_pid " with prio %" PRIu16 " \n",
          process->pid, process->priority);

    sched_set_status(process, STATUS_PENDING);

    if (!mutex->queue.next) {
        mutex->queue.next = mutex_LOCKED;
    }

    uint16_t process_priority = process->priority;
    sancus_debug("mutex_unlock: done.\n");
    // sched_switch(process_priority);
    // We do not switch immediately anymore as this would have required an exitless entry.
    // We only create a switch request if the new prio is above ours
    sched_switch_internal_allow_yield(process_priority, false);
}

void SM_FUNC(sancus_sm_timer) mutex_unlock_and_sleep(mutex_t *mutex)
{
    sancus_debug1("PID[%" PRIkernel_pid "]: unlocking mutex. taking a nap\n", sched_active_pid);

    if (mutex->queue.next) {
        if (mutex->queue.next == mutex_LOCKED) {
            mutex->queue.next = NULL;
        }
        else {
            list_node_t *next = list_remove_head(&mutex->queue);
            thread_t *process = container_of((clist_node_t*)next, thread_t,
                                             rq_entry);
            sancus_debug1("PID[%" PRIkernel_pid "]: waking up waiter.\n", process->pid);
            sched_set_status(process, STATUS_PENDING);
            if (!mutex->queue.next) {
                mutex->queue.next = mutex_LOCKED;
            }
        }
    }

    sancus_debug1("PID[%" PRIkernel_pid "]: going to sleep.\n", sched_active_pid);
    sched_set_status((thread_t*)sched_active_thread, STATUS_SLEEPING);
    thread_yield_higher();
}
