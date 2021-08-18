/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    core_sched Scheduler
 * @ingroup     core
 * @brief       The RIOT scheduler
 * @details
 *
 * RIOT features a tickless, preemptive, priority based scheduler.
 * Context switches can occur either preemptively (i.e. on interrupts),
 * voluntarily, or when a blocking operation (like `msg_receive()`) is
 * executed.
 * Being tickless means it does not have a timer that fires
 * periodically in order to emulate concurrent execution by switching
 * threads continuously.
 *
 * ## Priorities:
 *
 * Every thread is given a priority on creation. The priority values
 * are "order" or "nice" values, i.e. a higher value means a lower
 * priority.
 *
 * ### Example:
 *
 * Given threads with priorities A=6, B=1, and C=3, B has the highest
 * priority.
 *
 * A higher priority means that the scheduler will run this thread
 * whenever it becomes runnable instead of a thread with a lower
 * priority.
 * In case of equal priorities, the threads are scheduled in a
 * semi-cooperative fashion. That means that unless an interrupt
 * happens, threads with the same priority will only switch due to
 * voluntary or implicit context switches.
 *
 * ## Interrupts:
 *
 * When an interrupt occurs, e.g. because a timer fired or a network
 * packet was received, the active context is saved and an interrupt
 * service routine (ISR) that handles the interrupt is executed in
 * another context.
 * When the ISR is finished, the `::sched_context_switch_request` flag
 * can be checked. In case it is set, the `sched_run()` function is
 * called to determine the next active thread. (In the special case
 * that the ISR knows that it can not enable a thread switch, this
 * check can of course be omitted.)
 * If the flag is not set, the original context is being restored and
 * the thread resumes immediately.
 *
 * ## Voluntary Context Switches:
 *
 * There are two function calls that can lead to a voluntary context
 * switch: `thread_yield()` and `thread_sleep()`.
 * While the latter disables (think blocks) the thread until it is
 * woken (think unblocked) again via `thread_wakeup()`, the former only
 * leads to a context switch in case there is another runnable thread
 * with at least the same priority.
 *
 * ## Implicit Context Switches:
 *
 * Some functions that unblock another thread, e.g. `msg_send()` or
 * `mutex_unlock()`, can cause a thread switch, if the target had a
 * higher priority.
 *
 *
 * @{
 *
 * @file
 * @brief       Scheduler API definition
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 */

#ifndef SCHED_H
#define SCHED_H

#include <stddef.h>
#include "kernel_defines.h"
#include "bitarithm.h"
#include "kernel_types.h"
#include "native_sched.h"
#include "clist.h"
#include "sancus_modules.h"
#include "stdbool.h"

#ifdef __cplusplus
 extern "C" {
#endif


/**
 * @name Thread states supported by RIOT
 * @{
 */
typedef enum {
    STATUS_STOPPED,                 /**< has terminated                       */
    STATUS_SLEEPING,                /**< sleeping                             */
    STATUS_MUTEX_BLOCKED,           /**< waiting for a locked mutex           */
    STATUS_RECEIVE_BLOCKED,         /**< waiting for a message                */
    STATUS_SEND_BLOCKED,            /**< waiting for message to be delivered  */
    STATUS_REPLY_BLOCKED,           /**< waiting for a message response       */
    STATUS_FLAG_BLOCKED_ANY,        /**< waiting for any flag from flag_mask  */
    STATUS_FLAG_BLOCKED_ALL,        /**< waiting for all flags in flag_mask   */
    STATUS_MBOX_BLOCKED,            /**< waiting for get/put on mbox          */
    STATUS_COND_BLOCKED,            /**< waiting for a condition variable     */
    STATUS_RUNNING,                 /**< currently running                    */
    STATUS_PENDING,                 /**< waiting to be scheduled to run       */
    STATUS_NUMOF                    /**< number of supported thread states    */
} thread_status_t;
/** @} */

/**
 * @brief @c thread_t holds thread's context data.
    IMORTANT: When changing the order of variables here, 
    the manual offsets used in cpu.h MACRO_SAVE_CONTEXT need to be adjusted too!
 */
struct _thread {
    /** !! The first three elements in the struct AND sm_idx are indexed manually in cpu.c **/
    /** For easy assembly access, is_sm, sp, and sm_idx are to be the first item in the struct */
    bool is_sm; /** Marks a thread struct as to be returned into an sm **/
    char *sp;                       /**< thread's stack pointer         */
    entry_idx sm_idx;
    /** !! The first three elements in the struct are indexed manually in cpu.c **/
    thread_status_t status;         /**< thread's status                */
    uint8_t priority;               /**< thread's priority              */

    kernel_pid_t pid;               /**< thread's process id            */

// #if defined(MODULE_CORE_THREAD_FLAGS) || defined(DOXYGEN)
//     thread_flags_t flags;           /**< currently set flags            */
// #endif

    clist_node_t rq_entry;          /**< run queue entry                */
    bool in_use; /** Marks a thread struct as used. Only used by sm scheduler **/
    char *sm_entry; /* SM entry address used solely for sms. */

    // For periodic jobs, we define a period and the runtime within a period
    // To keep track, we also store the last reference point if we get interrupted anyway and how much of the current period has already been run
    uint32_t period;
    uint32_t runtime;
    uint32_t last_reference;
    uint32_t last_runtime;
    entry_idx original_idx;

// #if defined(MODULE_CORE_MSG) || defined(MODULE_CORE_THREAD_FLAGS) \
//     || defined(MODULE_CORE_MBOX) || defined(DOXYGEN)
//     void *wait_data;                /**< used by msg, mbox and thread
//                                          flags                          */
// #endif
// #if defined(MODULE_CORE_MSG) || defined(DOXYGEN)
//     list_node_t msg_waiters;        /**< threads waiting for their message
//                                          to be delivered to this thread
//                                          (i.e. all blocked sends)       */
//     cib_t msg_queue;                /**< index of this [thread's message queue]
//                                          (thread_t::msg_array), if any  */
//     msg_t *msg_array;               /**< memory holding messages sent
//                                          to this thread's message queue */
// #endif
// #if defined(DEVELHELP) || defined(SCHED_TEST_STACK) \
//     || defined(MODULE_MPU_STACK_GUARD) || defined(DOXYGEN)
//     char *stack_start;              /**< thread's stack start address   */
// #endif
// #if defined(DEVELHELP) || defined(DOXYGEN)
//     const char *name;               /**< thread's name                  */
//     int stack_size;                 /**< thread's stack size            */
// #endif
// #ifdef HAVE_THREAD_ARCH_T
//     thread_arch_t arch;             /**< architecture dependent part    */
// #endif
};


/**
 * Defines the scheduler overhead until the sched_run function. Used to calculate 
 * overheads and remaining runtimes of the periodic functions.
*/
#ifndef SCHEDULER_OVERHEAD_RUN
#define SCHEDULER_OVERHEAD_RUN 300
#endif

/**
 * @brief forward declaration for thread_t, defined in thread.h
 */
typedef struct _thread thread_t;

/**
 * @name Helpers to work with thread states
 * @{
 */
#define STATUS_ON_RUNQUEUE      STATUS_RUNNING  /**< to check if on run queue:
                                                 `st >= STATUS_ON_RUNQUEUE`   */
#define STATUS_NOT_FOUND ((thread_status_t)-1)  /**< Describes an illegal thread status */
/** @} */
/**
 * @def SCHED_PRIO_LEVELS
 * @brief The number of thread priority levels
 */
#ifndef SCHED_PRIO_LEVELS
#define SCHED_PRIO_LEVELS 16
#endif

/**
 * @def SCHED_PROTECTED_PRIO_LEVELS
 * @brief The number of thread priority levels that are reserved for SMs
 */
#ifndef SCHED_PROTECTED_PRIO_LEVELS
#define SCHED_PROTECTED_PRIO_LEVELS 1
#endif

/**
 * @def SCHED_PERIODIC_PRIO_LEVEL
 * @brief The prio level used for periodic jobs
 */
#ifndef SCHED_PERIODIC_PRIO_LEVEL
#define SCHED_PERIODIC_PRIO_LEVEL 1
#endif

/**
 * @def SCHED_MAX_PRIO_LEVEL_UNPROTECTED
 * @brief The max prio level that an unprotected thread can get
 * Since 0 has highest and MAX has lowest priority, the first unprotected 
 * priority is the one following the protected prio levels.
 */
#ifndef SCHED_MAX_PRIO_LEVEL_UNPROTECTED
#define SCHED_MAX_PRIO_LEVEL_UNPROTECTED SCHED_PROTECTED_PRIO_LEVELS + SCHED_PERIODIC_PRIO_LEVEL + 1
#endif

/**
 * @brief   Initializes the scheduler. Must be run once at boot time.
 */
void SM_FUNC(sancus_sm_timer) scheduler_init(void);

/**
 * @brief   Triggers the scheduler to schedule the next thread
 * @returns 1 if sched_active_thread/sched_active_pid was changed, 0 otherwise.
 */
int SM_ENTRY(sancus_sm_timer) sched_run(void);
int SM_FUNC(sancus_sm_timer) sched_run_internal(void); // Can be used from inside the SM

/**
 * @brief   Set the status of the specified process
 *
 * @param[in]   process     Pointer to the thread control block of the
 *                          targeted process
 * @param[in]   status      The new status of this thread
 */
void SM_FUNC(sancus_sm_timer) sched_set_status(thread_t *process, thread_status_t status);

/**
 * @brief   Set the status of the currently running process
 *
 * @param[in]   status      The new status of this thread
 */
void SM_ENTRY(sancus_sm_timer) sched_set_own_status(thread_status_t status);

/**
 * @brief       Yield if approriate.
 *
 * @details     Either yield if other_prio is higher than the current priority,
 *              or if the current thread is not on the runqueue.
 *
 *              Depending on whether the current execution is in an ISR (irq_is_in()),
 *              thread_yield_higher() is called or @ref sched_context_switch_request is set,
 *              respectively.
 *
 * @param[in]   other_prio      The priority of the target thread.
 */
void sched_switch(uint16_t other_prio);
void SM_FUNC(sancus_sm_timer) sched_switch_internal(uint16_t other_prio);
void SM_FUNC(sancus_sm_timer) sched_switch_internal_allow_yield(uint16_t other_prio, bool yield_allowed);

/**
 * This is a replacement for the old thread_yield that was in thread.c.
 * It simply places the thread back on the runqueue. After this, yield_higher should be called.
 * */
void SM_FUNC(sancus_sm_timer) sched_yield(void);

/**
 * @brief   Call context switching at thread exit
 */
NORETURN void cpu_switch_context_exit(void);

/**
 * @brief   Call scheduler kernel init. Done once at boot time. All subsequent calls are equivalent to a yield.
 */
NORETURN void scheduler_kernel_init(void);

/**
 * Optional function to shut down
 * */
void SM_ENTRY(sancus_sm_timer) sched_shut_down(void);

/**
 * Pointer to the sm_entry start of the scheduler. Necessary for scheduler to give this
 * address as a return point in r7 when scheduling SMs
 * */
SM_DATA(sancus_sm_timer) extern void* scheduler_entry;

/**
 * Flag indicating whether a context switch is necessary after handling an
 * interrupt. Supposed to be set in an ISR.
 */
SM_DATA(sancus_sm_timer) extern volatile unsigned int sched_context_switch_request;

/**
 *  Thread table
 */
SM_DATA(sancus_sm_timer) extern thread_t sched_threads[KERNEL_PID_LAST + 1];

/**
 *  Currently active thread
 */
SM_DATA(sancus_sm_timer) extern volatile thread_t *sched_active_thread;

/**
 *  Number of running (non-terminated) threads
 */
SM_DATA(sancus_sm_timer) extern volatile int sched_num_threads;

/**
 *  Process ID of active thread
 */
SM_DATA(sancus_sm_timer) extern volatile kernel_pid_t sched_active_pid;

/**
 * List of runqueues per priority level
 */
SM_DATA(sancus_sm_timer) extern clist_node_t sched_runqueues[SCHED_PRIO_LEVELS];

/**
 * @brief  Removes thread from scheduler and set status to #STATUS_STOPPED
 */
void SM_FUNC(sancus_sm_timer) sched_task_exit_internal(void);
NORETURN void sched_task_exit(void);

#ifdef MODULE_SCHED_CB
/**
 *  @brief  Register a callback that will be called on every scheduler run
 *
 *  @param[in] callback The callback functions the will be called
 */
void sched_register_cb(void (*callback)(kernel_pid_t, kernel_pid_t));
#endif /* MODULE_SCHED_CB */

#ifdef __cplusplus
}
#endif

#endif /* SCHED_H */
/** @} */
