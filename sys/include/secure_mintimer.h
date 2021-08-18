/*
 * Copyright (C) 2015 Kaspar Schleiser <kaspar@schleiser.de>
 * Copyright (C) 2016 Eistec AB
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup  sys_secure_mintimer Timers
 * @ingroup   sys
 * @brief     Provides a high level timer module to register
 *            timers, get current system time, and let a thread sleep for
 *            a certain amount of time.
 *
 * The implementation takes one low-level timer and multiplexes it.
 *
 * Insertion and removal of timers has O(n) complexity with (n) being the
 * number of active timers.  The reason for this is that multiplexing is
 * realized by next-first singly linked lists.
 *
 * @{
 * @file
 * @brief   secure_mintimer interface definitions
 * @author  Kaspar Schleiser <kaspar@schleiser.de>
 * @author  Joakim Nohlgård <joakim.nohlgard@eistec.se>
 */
#ifndef SECURE_MINTIMER_H
#define SECURE_MINTIMER_H

#include <stdbool.h>
#include <stdint.h>
#include "mutex.h"
#include "kernel_types.h"

#include "board.h"
#include "periph_conf.h"

#include "sancus_helpers.h"
#include "sancus_modules.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief secure_mintimer timestamp (64 bit)
 *
 * @note This is a struct in order to make the secure_mintimer API type strict
 */
typedef struct {
    uint64_t ticks64;       /**< Tick count */
} secure_mintimer_ticks64_t;

/**
 * @brief secure_mintimer timestamp (32 bit)
 *
 * @note This is a struct in order to make the secure_mintimer API type strict
 */
typedef struct {
    uint32_t ticks32;       /**< Tick count */
} secure_mintimer_ticks32_t;

/**
 * @brief secure_mintimer callback type
 */
typedef void (*secure_mintimer_callback_t)(void*);

/**
 * @brief secure_mintimer timer structure
 */
typedef struct secure_mintimer {
    struct secure_mintimer *next;         /**< reference to next timer in timer lists */
    uint32_t target;             /**< lower 32bit absolute target time */
    uint32_t long_target;        /**< upper 32bit absolute target time */
    thread_t* thread;       /** Thread this timer is associated with **/
    // secure_mintimer_callback_t callback;  /**< callback function to call when timer
                                    //  expires */
    // void *arg;                   /**< argument to pass to callback function */
} secure_mintimer_t;

/**
 * @brief get the current system time as 32bit time stamp value
 *
 * @note    Overflows 2**32 ticks, thus returns secure_mintimer_now64() % 32,
 *          but is cheaper.
 *
 * @return  current time as 32bit time stamp
 */
uint32_t SM_ENTRY(sancus_sm_timer) _secure_mintimer_now(void);
void SM_FUNC(sancus_sm_timer) _secure_mintimer_now_internal(uint32_t *short_term, uint32_t *long_term);

/**
 * @brief get the current system time as 64bit time stamp
 *
 * @return  current time as 64bit time stamp
 */
uint64_t SM_ENTRY(sancus_sm_timer) _secure_mintimer_now64(void);

/**
 * @brief get the current system time in microseconds since start
 *
 * This is a convenience function for @c secure_mintimer_usec_from_ticks(secure_mintimer_now())
 */
static inline uint32_t secure_mintimer_now_usec(void);

/**
 * @brief get the current system time in microseconds since start
 *
 * This is a convenience function for @c secure_mintimer_usec_from_ticks64(secure_mintimer_now64())
 */
static inline uint64_t secure_mintimer_now_usec64(void);

/**
 * @brief secure_mintimer initialization function
 *
 * This sets up secure_mintimer. Has to be called once at system boot.
 * If @ref auto_init is enabled, it will call this for you.
 */
void SM_FUNC(sancus_sm_timer) secure_mintimer_init(void);

/**
 * During the timer tick, we add all expired timers to the waiting list 
 * that only gets emptied after alle high priority schedules have been called.
 * */
// static void SM_FUNC(sancus_sm_timer) shoot_all_waiting_timers();

/**
 * @brief Pause the execution of a thread for some seconds
 *
 * When called from an ISR, this function will spin and thus block the MCU in
 * interrupt context for the specified amount in *seconds*, so don't *ever* use
 * it there.
 *
 * @param[in] seconds   the amount of seconds the thread should sleep
 */
void SM_FUNC(sancus_sm_timer) _secure_mintimer_tsleep_internal();
static inline void secure_mintimer_sleep(uint32_t seconds);

/**
 * @brief Sets a specific pid to sleep. Only usable internally of scheduler.
 * 
 * See tsleep_internal, just with a specific pid.
 * */
void SM_FUNC(sancus_sm_timer) _secure_mintimer_tsleep_specific_pid(uint32_t offset, kernel_pid_t pid);

/**
 * @brief Pause the execution of a thread for some microseconds
 *
 * When called from an ISR, this function will spin and thus block the MCU for
 * the specified amount in microseconds, so only use it there for *very* short
 * periods, e.g., less than SECURE_MINTIMER_BACKOFF.
 *
 * @param[in] microseconds  the amount of microseconds the thread should sleep
 */
static inline void secure_mintimer_usleep(uint32_t microseconds);

/**
 * @brief Stop execution of a thread for some time
 *
 * Don't expect nanosecond accuracy. As of now, this function just calls
 * secure_mintimer_usleep(nanoseconds/1000).
 *
 * When called from an ISR, this function will spin-block, so only use it there
 * for *very* short periods.
 *
 * @param[in] nanoseconds   the amount of nanoseconds the thread should sleep
 */
static inline void secure_mintimer_nanosleep(uint32_t nanoseconds);

/**
 * @brief Stop execution of a thread for some time, 32bit version
 *
 * When called from an ISR, this function will spin and thus block the MCU for
 * the specified amount, so only use it there for *very* short periods,
 * e.g. less than SECURE_MINTIMER_BACKOFF.
 *
 * @param[in] ticks  number of ticks the thread should sleep
 */
static inline void secure_mintimer_tsleep32(secure_mintimer_ticks32_t ticks);

/**
 * @brief Stop execution of a thread for some time, 64bit version
 *
 * When called from an ISR, this function will spin and thus block the MCU for
 * the specified amount, so only use it there for *very* short periods,
 * e.g. less than SECURE_MINTIMER_BACKOFF.
 *
 * @param[in] ticks  number of ticks the thread should sleep
 */
static inline void secure_mintimer_tsleep64(secure_mintimer_ticks64_t ticks);

/**
 * @brief Stop execution of a thread for some time, blocking
 *
 * This function will spin-block, so only use it *very* short periods.
 *
 * @param[in] ticks  the number of secure_mintimer ticks the thread should spin for
 */
static inline void SM_FUNC(sancus_sm_timer) secure_mintimer_spin(secure_mintimer_ticks32_t ticks);

/**
 * @brief Set a timer to execute a callback at some time in the future, 64bit
 * version
 *
 * Expects timer->callback to be set.
 *
 * The callback specified in the timer struct will be executed @p offset_usec
 * microseconds in the future.
 *
 * @warning BEWARE! Callbacks from secure_mintimer_set() are being executed in interrupt
 * context (unless offset < SECURE_MINTIMER_BACKOFF). DON'T USE THIS FUNCTION unless you
 * know *exactly* what that means.
 *
 * @param[in] timer       the timer structure to use.
 *                        Its secure_mintimer_t::target and secure_mintimer_t::long_target
 *                        fields need to be initialized with 0 on first use
 * @param[in] offset_us   time in microseconds from now specifying that timer's
 *                        callback's execution time
 */
// static inline void secure_mintimer_set64(secure_mintimer_t *timer, uint64_t offset_us);
void SM_FUNC(sancus_sm_timer) _secure_mintimer_set(secure_mintimer_t *timer, uint32_t offset);

// Set explicit timer without checking time again.
secure_mintimer_t* SM_FUNC(sancus_sm_timer) get_available_timer(kernel_pid_t pid);
int SM_FUNC(sancus_sm_timer) _secure_mintimer_set_absolute(secure_mintimer_t *timer, uint32_t target);
int SM_FUNC(sancus_sm_timer) _secure_mintimer_set_absolute_explicit(secure_mintimer_t *timer, uint32_t now);

/**
 * @brief remove a timer
 *
 * @note this function runs in O(n) with n being the number of active timers
 *
 * @param[in] timer ptr to timer structure that will be removed
 */
void SM_FUNC(sancus_sm_timer) secure_mintimer_remove(secure_mintimer_t *timer);

/**
 * @brief Convert microseconds to secure_mintimer ticks
 *
 * @param[in] usec  microseconds
 *
 * @return secure_mintimer time stamp
 */
static inline secure_mintimer_ticks32_t secure_mintimer_ticks_from_usec(uint32_t usec);

/**
 * @brief Convert microseconds to secure_mintimer ticks, 64 bit version
 *
 * @param[in] usec  microseconds
 *
 * @return secure_mintimer time stamp
 */
static inline secure_mintimer_ticks64_t secure_mintimer_ticks_from_usec64(uint64_t usec);

/**
 * @brief Convert secure_mintimer ticks to microseconds
 *
 * @param[in] ticks  secure_mintimer time stamp
 *
 * @return microseconds
 */
static inline uint32_t secure_mintimer_usec_from_ticks(secure_mintimer_ticks32_t ticks);

/**
 * @brief Convert secure_mintimer ticks to microseconds, 64 bit version
 *
 * @param[in] ticks  secure_mintimer time stamp
 *
 * @return microseconds
 */
static inline uint64_t secure_mintimer_usec_from_ticks64(secure_mintimer_ticks64_t ticks);

/**
 * @brief Create an secure_mintimer time stamp
 *
 * @param[in] ticks  number of secure_mintimer ticks
 *
 * @return secure_mintimer time stamp
 */
static inline secure_mintimer_ticks32_t secure_mintimer_ticks(uint32_t ticks);

/**
 * @brief Create an secure_mintimer time stamp, 64 bit version
 *
 * @param[in] ticks  number of secure_mintimer ticks
 *
 * @return secure_mintimer time stamp
 */
static inline secure_mintimer_ticks64_t secure_mintimer_ticks64(uint64_t ticks);

/**
 * @brief Compute difference between two secure_mintimer time stamps
 *
 * @param[in] a  left operand
 * @param[in] b  right operand
 *
 * @return @p a - @p b
 */
static inline secure_mintimer_ticks32_t secure_mintimer_diff(secure_mintimer_ticks32_t a, secure_mintimer_ticks32_t b);

/**
 * @brief Compute difference between two secure_mintimer time stamps, 64 bit version
 *
 * @param[in] a  left operand
 * @param[in] b  right operand
 *
 * @return @p a - @p b
 */
static inline secure_mintimer_ticks64_t secure_mintimer_diff64(secure_mintimer_ticks64_t a, secure_mintimer_ticks64_t b);

/**
 * @brief Compute 32 bit difference between two 64 bit secure_mintimer time stamps
 *
 * @param[in] a  left operand
 * @param[in] b  right operand
 *
 * @return @p a - @p b cast truncated to 32 bit
 */
static inline secure_mintimer_ticks32_t secure_mintimer_diff32_64(secure_mintimer_ticks64_t a, secure_mintimer_ticks64_t b);

/**
 * @brief Compare two secure_mintimer time stamps
 *
 * @param[in] a  left operand
 * @param[in] b  right operand
 *
 * @return @p a < @p b
 */
static inline bool secure_mintimer_less(secure_mintimer_ticks32_t a, secure_mintimer_ticks32_t b);

/**
 * @brief Compare two secure_mintimer time stamps, 64 bit version
 *
 * @param[in] a  left operand
 * @param[in] b  right operand
 *
 * @return @p a < @p b
 */
static inline bool secure_mintimer_less64(secure_mintimer_ticks64_t a, secure_mintimer_ticks64_t b);

/**
 * @brief lock a mutex but with timeout
 *
 * @param[in]    mutex  mutex to lock
 * @param[in]    us     timeout in microseconds relative
 *
 * @return       0, when returned after mutex was locked
 * @return       -1, when the timeout occcured
 */
int secure_mintimer_mutex_lock_timeout(mutex_t *mutex, uint64_t us);

/**
 * @brief    Set timeout thread flag after @p timeout
 *
 * This function will set THREAD_FLAG_TIMEOUT on the current thread after @p
 * timeout usec have passed.
 *
 * @param[in]   t       timer struct to use
 * @param[in]   timeout timeout in usec
 */
void secure_mintimer_set_timeout_flag(secure_mintimer_t *t, uint32_t timeout);

// Used to call the timer callback at arbitrary times unrelated to timer.c (e.g. after operations)
void SM_FUNC(sancus_sm_timer) secure_mintimer_timer_callback(void);

/**
 * @brief secure_mintimer backoff value
 *
 * All timers that are less than SECURE_MINTIMER_BACKOFF microseconds in the future will
 * just spin.
 *
 * This is supposed to be defined per-device in e.g., periph_conf.h.
 */
#ifndef SECURE_MINTIMER_BACKOFF
#define SECURE_MINTIMER_BACKOFF 200
#endif

/**
 * @brief secure_mintimer overhead value, in hardware ticks
 *
 * This value specifies the time a timer will be late if uncorrected, e.g.,
 * the system-specific secure_mintimer execution time from timer ISR to executing
 * a timer's callback's first instruction.
 *
 * E.g., with SECURE_MINTIMER_OVERHEAD == 0
 * start=secure_mintimer_now();
 * secure_mintimer_set(&timer, X);
 * (in callback:)
 * overhead=secure_mintimer_now()-start-X;
 *
 * secure_mintimer automatically substracts SECURE_MINTIMER_OVERHEAD from a timer's target time,
 * but when the timer triggers, secure_mintimer will spin-lock until a timer's target
 * time is reached, so timers will never trigger early.
 *
 * This is supposed to be defined per-device in e.g., periph_conf.h.
 */
#ifndef SECURE_MINTIMER_OVERHEAD
#define SECURE_MINTIMER_OVERHEAD 300
#endif

#ifndef SECURE_MINTIMER_ISR_BACKOFF
/**
 * @brief   secure_mintimer IRQ backoff time, in hardware ticks
 *
 * When scheduling the next IRQ, if it is less than the backoff time
 * in the future, just spin.
 *
 * This is supposed to be defined per-device in e.g., periph_conf.h.
 */
#define SECURE_MINTIMER_ISR_BACKOFF 200
#endif

#ifndef SECURE_MINTIMER_PERIODIC_SPIN
/**
 * @brief   secure_mintimer_periodic_wakeup spin cutoff
 *
 * If the difference between target time and now is less than this value, then
 * secure_mintimer_periodic_wakeup will use secure_mintimer_spin instead of setting a timer.
 */
#define SECURE_MINTIMER_PERIODIC_SPIN (SECURE_MINTIMER_BACKOFF * 2)
#endif

#ifndef SECURE_MINTIMER_PERIODIC_RELATIVE
/**
 * @brief   secure_mintimer_periodic_wakeup relative target cutoff
 *
 * If the difference between target time and now is less than this value, then
 * secure_mintimer_periodic_wakeup will set a relative target time in the future instead
 * of the true target.
 *
 * This is done to prevent target time underflows.
 */
#define SECURE_MINTIMER_PERIODIC_RELATIVE (512)
#endif

/*
 * Default secure_mintimer configuration
 */
#ifndef SECURE_MINTIMER_DEV
/**
 * @brief Underlying hardware timer device to assign to secure_mintimer
 */
#define SECURE_MINTIMER_DEV TIMER_DEV(0)
/**
 * @brief Underlying hardware timer channel to assign to secure_mintimer
 */
#define SECURE_MINTIMER_CHAN (0)

#if (TIMER_0_MAX_VALUE) == 0xfffffful
#define SECURE_MINTIMER_WIDTH (24)
#elif (TIMER_0_MAX_VALUE) == 0xffff
#define SECURE_MINTIMER_WIDTH (16)
#endif

#endif

#ifndef SECURE_MINTIMER_WIDTH
/**
 * @brief secure_mintimer timer width
 *
 * This value specifies the width (in bits) of the hardware timer used by secure_mintimer.
 * Default for sancus is 16.
 */
#define SECURE_MINTIMER_WIDTH (16)
#endif

#if (SECURE_MINTIMER_WIDTH != 32) || DOXYGEN
/**
 * @brief secure_mintimer timer mask
 *
 * This value specifies the mask relative to 0xffffffff that the used timer
 * counts to, e.g., 0xffffffff & ~TIMER_MAXVALUE.
 *
 * For a 16bit timer, the mask would be 0xFFFF0000, for a 24bit timer, the mask
 * would be 0xFF000000.
 */
#define SECURE_MINTIMER_MASK ((0xffffffff >> SECURE_MINTIMER_WIDTH) << SECURE_MINTIMER_WIDTH)
#else
#define SECURE_MINTIMER_MASK (0)
#endif

/**
 * @brief  Base frequency of secure_mintimer is 1 MHz
 */
#define SECURE_MINTIMER_HZ_BASE (1000000ul)

#ifndef SECURE_MINTIMER_HZ
/**
 * @brief  Frequency of the underlying hardware timer
 */
#define SECURE_MINTIMER_HZ SECURE_MINTIMER_HZ_BASE
#endif

#ifndef SECURE_MINTIMER_SHIFT
#if (SECURE_MINTIMER_HZ == 32768ul)
/* No shift necessary, the conversion is not a power of two and is handled by
 * functions in tick_conversion.h */
#define SECURE_MINTIMER_SHIFT (0)
#elif (SECURE_MINTIMER_HZ == SECURE_MINTIMER_HZ_BASE)
/**
 * @brief   secure_mintimer prescaler value
 *
 * If the underlying hardware timer is running at a power of two multiple of
 * 15625, SECURE_MINTIMER_SHIFT can be used to adjust the difference.
 *
 * For a 1 MHz hardware timer, set SECURE_MINTIMER_SHIFT to 0.
 * For a 2 MHz or 500 kHz, set SECURE_MINTIMER_SHIFT to 1.
 * For a 4 MHz or 250 kHz, set SECURE_MINTIMER_SHIFT to 2.
 * For a 8 MHz or 125 kHz, set SECURE_MINTIMER_SHIFT to 3.
 * For a 16 MHz or 62.5 kHz, set SECURE_MINTIMER_SHIFT to 4.
 * and for 32 MHz, set SECURE_MINTIMER_SHIFT to 5.
 *
 * The direction of the shift is handled by the macros in tick_conversion.h
 */
#define SECURE_MINTIMER_SHIFT (0)
#elif (SECURE_MINTIMER_HZ >> 1 == SECURE_MINTIMER_HZ_BASE) || (SECURE_MINTIMER_HZ << 1 == SECURE_MINTIMER_HZ_BASE)
#define SECURE_MINTIMER_SHIFT (1)
#elif (SECURE_MINTIMER_HZ >> 2 == SECURE_MINTIMER_HZ_BASE) || (SECURE_MINTIMER_HZ << 2 == SECURE_MINTIMER_HZ_BASE)
#define SECURE_MINTIMER_SHIFT (2)
#elif (SECURE_MINTIMER_HZ >> 3 == SECURE_MINTIMER_HZ_BASE) || (SECURE_MINTIMER_HZ << 3 == SECURE_MINTIMER_HZ_BASE)
#define SECURE_MINTIMER_SHIFT (3)
#elif (SECURE_MINTIMER_HZ >> 4 == SECURE_MINTIMER_HZ_BASE) || (SECURE_MINTIMER_HZ << 4 == SECURE_MINTIMER_HZ_BASE)
#define SECURE_MINTIMER_SHIFT (4)
#elif (SECURE_MINTIMER_HZ >> 5 == SECURE_MINTIMER_HZ_BASE) || (SECURE_MINTIMER_HZ << 5 == SECURE_MINTIMER_HZ_BASE)
#define SECURE_MINTIMER_SHIFT (5)
#elif (SECURE_MINTIMER_HZ >> 6 == SECURE_MINTIMER_HZ_BASE) || (SECURE_MINTIMER_HZ << 6 == SECURE_MINTIMER_HZ_BASE)
#define SECURE_MINTIMER_SHIFT (6)
#else
#error "SECURE_MINTIMER_SHIFT cannot be derived for given SECURE_MINTIMER_HZ, verify settings!"
#endif
#else
#error "SECURE_MINTIMER_SHIFT is set relative to SECURE_MINTIMER_HZ, no manual define required!"
#endif

#ifdef DEBUG_TIMER
extern secure_mintimer_t secure_mintimer_timer_list [];
extern secure_mintimer_t *timer_list_head;
extern secure_mintimer_t *long_list_head;
extern uint32_t _long_cnt;
extern uint32_t _secure_mintimer_high_cnt;
#endif

#include "secure_mintimer/secure_mintimer_tick_conversion.h"

#include "secure_mintimer/secure_mintimer_implementation.h"

#ifdef __cplusplus
}
#endif

/** @} */
#endif /* SECURE_MINTIMER_H */
