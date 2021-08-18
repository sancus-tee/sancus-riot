/**
 * Copyright (C) 2015 Kaspar Schleiser <kaspar@schleiser.de>
 *               2016 Eistec AB
 *               2018 Josua Arndt
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup sys_secure_mintimer
 *
 * @{
 * @file
 * @brief secure_mintimer core functionality
 * @author Kaspar Schleiser <kaspar@schleiser.de>
 * @author Joakim Nohlgård <joakim.nohlgard@eistec.se>
 * @author Josua Arndt <jarndt@ias.rwth-aachen.de>
 * @}
 */

#include "secure_mintimer.h"
#include <stdint.h>
#include <string.h>
#include "board.h"
#include "periph/timer.h"
#include "periph_conf.h"
// #include "uart.h"
#include "secure_mintimer.h"
// #include "irq.h"
#include "mutex.h"
#include "sancus_modules.h"
#include "sancus_helpers.h"
#include "sm_irq.h"
#include "time.h"

/* WARNING! enabling this will have side effects and can lead to timer underflows. */
#define ENABLE_DEBUG 0
// #include "debug.h"
#if ENABLE_DEBUG
#define SECMIN_DEBUG(a) if (ENABLE_DEBUG) a
#else
#define SECMIN_DEBUG(a)
#endif

static volatile SM_DATA(sancus_sm_timer) int _in_handler = 0;

// To be able to debug the timer, we have a debug flag on some protections
#ifndef DEBUG_TIMER
static SM_DATA(sancus_sm_timer) uint32_t _long_cnt = 0;
#if SECURE_MINTIMER_MASK
static SM_DATA(sancus_sm_timer) uint32_t _secure_mintimer_high_cnt = 0;
#endif
#else
uint32_t _long_cnt = 0;
uint32_t _secure_mintimer_high_cnt = 0;
#endif

static inline void SM_FUNC(sancus_sm_timer) secure_mintimer_spin_until(uint32_t value);

#define SECURE_MINTIMER_TIMER_LIST_LENGTH 15
static SM_DATA(sancus_sm_timer) secure_mintimer_t *overflow_list_head = NULL;

#ifndef DEBUG_TIMER
static SM_DATA(sancus_sm_timer) secure_mintimer_t secure_mintimer_timer_list [SECURE_MINTIMER_TIMER_LIST_LENGTH];
static SM_DATA(sancus_sm_timer) secure_mintimer_t *timer_list_head = NULL;
static SM_DATA(sancus_sm_timer) secure_mintimer_t *long_list_head = NULL;
#else
secure_mintimer_t secure_mintimer_timer_list [SECURE_MINTIMER_TIMER_LIST_LENGTH];
secure_mintimer_t *timer_list_head = NULL;
secure_mintimer_t *long_list_head = NULL;
#endif

static void SM_FUNC(sancus_sm_timer) _add_timer_to_list(secure_mintimer_t **list_head, secure_mintimer_t *timer);
static void SM_FUNC(sancus_sm_timer)_add_timer_to_long_list(secure_mintimer_t **list_head, secure_mintimer_t *timer);
static void SM_FUNC(sancus_sm_timer)_shoot_timer(secure_mintimer_t *timer);
static void SM_FUNC(sancus_sm_timer)_remove(secure_mintimer_t *timer);
static inline void SM_FUNC(sancus_sm_timer) _lltimer_set(uint32_t target);
static uint32_t SM_FUNC(sancus_sm_timer) _time_left(uint32_t target, uint32_t reference);

static void SM_FUNC(sancus_sm_timer)_timer_callback(void);
static void SM_FUNC(sancus_sm_timer) _periph_timer_callback(int chan);

static inline int SM_FUNC(sancus_sm_timer)_this_high_period(uint32_t target);

int SM_FUNC(sancus_sm_timer) _secure_mintimer_set_absolute(secure_mintimer_t *timer, uint32_t target);

secure_mintimer_t* SM_FUNC(sancus_sm_timer) get_available_timer(kernel_pid_t pid){
    if(pid<=SECURE_MINTIMER_TIMER_LIST_LENGTH){
        secure_mintimer_t* current_timer = &secure_mintimer_timer_list[pid];

        // if(current_timer->target == 0 && current_timer->long_target == 0){
        //     return current_timer;
        // }

        // Delete any pending timer and return this timer
        secure_mintimer_remove(current_timer);
        return current_timer;
    }


    return NULL;
}

/**
 * @brief drop bits of a value that don't fit into the low-level timer.
 */
static inline uint32_t SM_FUNC(sancus_sm_timer) _secure_mintimer_lltimer_mask(uint32_t val)
{
    /* cppcheck-suppress shiftTooManyBits
     * (reason: cppcheck bug. `SECURE_MINTIMER_MASK` is zero when `SECURE_MINTIMER_WIDTH` is 32) */
    return val & ~SECURE_MINTIMER_MASK;
}

static inline int SM_FUNC(sancus_sm_timer) _is_set(secure_mintimer_t *timer)
{
    return (timer->target || timer->long_target);
}

/**
 * @brief returns the (masked) low-level timer counter value.
 */
static inline unsigned int SM_FUNC(sancus_sm_timer) _secure_mintimer_lltimer_now(void)
{
    return sm_timer_read_internal(0);
}

static inline void SM_FUNC(sancus_sm_timer) secure_mintimer_spin_until(uint32_t target)
{
#if SECURE_MINTIMER_MASK
    target = _secure_mintimer_lltimer_mask(target);
#endif
    while (_secure_mintimer_lltimer_now() > target) {}
    while (_secure_mintimer_lltimer_now() < target) {}
}

void SM_FUNC(sancus_sm_timer) secure_mintimer_init(void)
{
    /* initialize low-level timer */
    sm_timer_init(SECURE_MINTIMER_DEV, SECURE_MINTIMER_HZ, _periph_timer_callback);

    /* register initial overflow tick */
    _lltimer_set(0xFFFFFFFF);
}

uint32_t SM_ENTRY(sancus_sm_timer) _secure_mintimer_now(void)
{
#if SECURE_MINTIMER_MASK
    uint32_t latched_high_cnt, now;

    /* _high_cnt can change at any time, so check the value before
     * and after reading the low-level timer. If it hasn't changed,
     * then it can be safely applied to the timer count. */

    do {
        latched_high_cnt = _secure_mintimer_high_cnt;
        now = _secure_mintimer_lltimer_now();
    } while (_secure_mintimer_high_cnt != latched_high_cnt);

    return latched_high_cnt | now;
#else
    return _secure_mintimer_lltimer_now();
#endif
}

void SM_FUNC(sancus_sm_timer) _secure_mintimer_now_internal(uint32_t *short_term, uint32_t *long_term)
{
    // uint32_t before, after, long_value;

    /* loop to cope with possible overflow of _secure_mintimer_now() */
    // do {
    //     before = _secure_mintimer_now();
    //     long_value = _long_cnt;
    //     after = _secure_mintimer_now();

    // } while (before > after);

    //FIXME: This may ignore very fresh overflows (between start of scheduler and now).
    *short_term = _secure_mintimer_high_cnt | _secure_mintimer_lltimer_now(); 
    *long_term = _long_cnt;
}

uint64_t SM_ENTRY(sancus_sm_timer) _secure_mintimer_now64(void)
{
    uint32_t short_term, long_term;

    _secure_mintimer_now_internal(&short_term, &long_term);

    return ((uint64_t)long_term << 32) + short_term;
}

// static inline void SM_FUNC(sancus_sm_timer) _secure_mintimer_spin(uint32_t offset) {
//     uint32_t start = _secure_mintimer_lltimer_now();
// #if SECURE_MINTIMER_MASK
//     offset = _secure_mintimer_lltimer_mask(offset);
//     while (_secure_mintimer_lltimer_mask(_secure_mintimer_lltimer_now() - start) < offset);
// #else
//     while ((_secure_mintimer_lltimer_now() - start) < offset);
// #endif
// }

void SM_FUNC(sancus_sm_timer) _secure_mintimer_set(secure_mintimer_t *timer, uint32_t offset)
{
    SECMIN_DEBUG(sancus_debug3("timer_set(): offset=%lu now=%lu (%lu)",
          offset, _secure_mintimer_now(), _secure_mintimer_lltimer_now()));
    // if (!timer->callback) {
    //     SECMIN_DEBUG(sancus_debug("timer_set(): timer has no callback."));
    //     return;
    // }

    secure_mintimer_remove(timer);

    // if (offset < SECURE_MINTIMER_BACKOFF) {
    //     _secure_mintimer_spin(offset);
    //     _shoot_timer(timer);
    // }
    // else {
        uint32_t target = _secure_mintimer_now() + offset;
        _secure_mintimer_set_absolute(timer, target);
    // }
}

// void SM_FUNC(sancus_sm_timer) _secure_mintimer_set64(secure_mintimer_t *timer, uint32_t offset, uint32_t long_offset)
// {
//     SECMIN_DEBUG(sancus_debug2(" _secure_mintimer_set64() offset=%" PRIu32 " long_offset=%" PRIu32 " ", offset, long_offset));
//     if (!long_offset) {
//         /* timer fits into the short timer */
//         _secure_mintimer_set(timer, (uint32_t)offset);
//     }
//     else {
//         // int state = sm_irq_disable();
//         if (_is_set(timer)) {
//             _remove(timer);
//         }

//         _secure_mintimer_now_internal(&timer->target, &timer->long_target);
//         timer->target += offset;
//         timer->long_target += long_offset;
//         if (timer->target < offset) {
//             timer->long_target++;
//         }

//         _add_timer_to_long_list(&long_list_head, timer);
//         // sm_irq_restore(state);
//         SECMIN_DEBUG(sancus_debug2("secure_mintimer_set64(): added longterm timer (long_target=%" PRIu32 " target=%" PRIu32 ")\n",
//               timer->long_target, timer->target));
//     }
// }

int SM_FUNC(sancus_sm_timer) _secure_mintimer_set_absolute_explicit(secure_mintimer_t *timer, uint32_t now){
    int res = 0;
    
    uint32_t target = timer->target;
    /* Ensure timer is fired in right timer period.
     * Backoff condition above ensures that 'target - SECURE_MINTIMER_OVERHEAD` is later
     * than 'now', also for values when now will overflow and the value of target
     * is smaller then now.
     * If `target < SECURE_MINTIMER_OVERHEAD` the new target will be at the end of this
     * 32bit period, as `target - SECURE_MINTIMER_OVERHEAD` is a big number instead of a
     * small at the beginning of the next period. */
    target = target - SECURE_MINTIMER_OVERHEAD;

    /* 32 bit target overflow, target is in next 32bit period */
    if (target < now) {
        timer->long_target++;
    }

    if ((timer->long_target > _long_cnt) || !_this_high_period(target)) {
        SECMIN_DEBUG(sancus_debug3("Timer set is long target %lu long count %lu and this high period %d", timer->long_target , _long_cnt,!_this_high_period(target) ));
        SECMIN_DEBUG(sancus_debug("secure_mintimer_set_absolute(): the timer doesn't fit into the low-level timer's mask."));
        _add_timer_to_long_list(&long_list_head, timer);
    }
    else {
        if (_secure_mintimer_lltimer_mask(now) >= target) {
            SECMIN_DEBUG(sancus_debug("secure_mintimer_set_absolute(): the timer will expire in the next timer period"));
            _add_timer_to_list(&overflow_list_head, timer);
        }
        else {
            SECMIN_DEBUG(sancus_debug("timer_set_absolute(): timer will expire in this timer period."));
            _add_timer_to_list(&timer_list_head, timer);

            if (timer_list_head == timer) {
                SECMIN_DEBUG(sancus_debug("timer_set_absolute(): timer is new list head. updating lltimer."));
                _lltimer_set(target);
            }
        }
    }

    // sm_irq_restore(state);

    return res;
}

int SM_FUNC(sancus_sm_timer) _secure_mintimer_set_absolute(secure_mintimer_t *timer, uint32_t target)
{
    uint32_t now = _secure_mintimer_now();

    timer->next = NULL;

    /* Ensure that offset is bigger than 'SECURE_MINTIMER_BACKOFF',
     * 'target - now' will allways be the offset no matter if target < or > now.
     *
     * This expects that target was not set too close to now and overrun now, so
     * from setting target up until the call of '_secure_mintimer_now()' above now has not
     * become equal or bigger than target.
     * This is crucial when using low CPU frequencies so reaching the '_secure_mintimer_now()'
     * call needs multiple secure_mintimer ticks.
     *
     * '_secure_mintimer_set()' and `_secure_mintimer_periodic_wakeup()` ensure this by already
     * backing off for small values. */
    uint32_t offset = (target - now);

    SECMIN_DEBUG(sancus_debug3("timer_set_absolute(): now=%lu target=%lu offset=%lu ",
          now, target, offset));

    if (offset <= SECURE_MINTIMER_BACKOFF) {
        /* backoff */
        secure_mintimer_spin_until(target);
        _shoot_timer(timer);
        return 0;
    }

    // unsigned state = sm_irq_disable();
    if (_is_set(timer)) {
        _remove(timer);
    }

    timer->target = target;
    timer->long_target = _long_cnt;

    return _secure_mintimer_set_absolute_explicit(timer, now);
}


static void SM_FUNC(sancus_sm_timer)_periph_timer_callback(int chan)
{
    (void)chan;
    _timer_callback();
}

/**
 * 
 * */
static void SM_FUNC(sancus_sm_timer) _shoot_timer(secure_mintimer_t *timer)
{
    // To shoot a timer, we just allow the thread to be scheduled again, aka "wake" it up
    if(timer->thread != NULL) sched_set_status(timer->thread, STATUS_PENDING);
    
    // Since a timer triggered, we should run the scheduler.
    sched_context_switch_request = 1;
}

static inline void SM_FUNC(sancus_sm_timer) _lltimer_set(uint32_t target)
{
    if (_in_handler) {
        return;
    }
    // SECMIN_DEBUG(sancus_debug1("_lltimer_set(): setting %" PRIu32 "\n", _secure_mintimer_lltimer_mask(target)));
    sm_timer_set_absolute(SECURE_MINTIMER_CHAN, _secure_mintimer_lltimer_mask(target));
}


static void SM_FUNC(sancus_sm_timer) _add_timer_to_list(secure_mintimer_t **list_head, secure_mintimer_t *timer)
{
    while (*list_head && (*list_head)->target <= timer->target) {
        list_head = &((*list_head)->next);
    }

    timer->next = *list_head;
    *list_head = timer;
}

static void SM_FUNC(sancus_sm_timer) _add_timer_to_long_list(secure_mintimer_t **list_head, secure_mintimer_t *timer)
{
    while (*list_head
           && (((*list_head)->long_target < timer->long_target)
           || (((*list_head)->long_target == timer->long_target) && ((*list_head)->target <= timer->target)))) {
        list_head = &((*list_head)->next);
    }

    timer->next = *list_head;
    *list_head = timer;
}

static int SM_FUNC(sancus_sm_timer) _remove_timer_from_list(secure_mintimer_t **list_head, secure_mintimer_t *timer)
{
    while (*list_head) {
        if (*list_head == timer) {
            *list_head = timer->next;
            return 1;
        }
        list_head = &((*list_head)->next);
    }

    return 0;
}

static void SM_FUNC(sancus_sm_timer) _remove(secure_mintimer_t *timer)
{
    if (timer_list_head == timer) {
        uint32_t next;
        timer_list_head = timer->next;
        if (timer_list_head) {
            /* schedule callback on next timer target time */
            next = timer_list_head->target - SECURE_MINTIMER_OVERHEAD;
        }
        else {
            next = _secure_mintimer_lltimer_mask(0xFFFFFFFF);
        }
        _lltimer_set(next);
    }
    else {
        if (!_remove_timer_from_list(&timer_list_head, timer)) {
            if (!_remove_timer_from_list(&overflow_list_head, timer)) {
                _remove_timer_from_list(&long_list_head, timer);
            }
        }
    }
}

void SM_FUNC(sancus_sm_timer) secure_mintimer_remove(secure_mintimer_t *timer)
{
    // int state = sm_irq_disable();

    if (_is_set(timer)) {
        _remove(timer);
        timer->target = 0;
        timer->long_target = 0;
    }
    // sm_irq_restore(state);
}


static uint32_t SM_FUNC(sancus_sm_timer) _time_left(uint32_t target, uint32_t reference)
{
    uint32_t now = _secure_mintimer_lltimer_now();

    if (now < reference) {
        return 0;
    }

    if (target > now) {
        return target - now;
    }
    else {
        return 0;
    }
}

static inline int SM_FUNC(sancus_sm_timer) _this_high_period(uint32_t target)
{
#if SECURE_MINTIMER_MASK
    // LOG_ERROR("Target is %ul while mask is %ul. The combined is %ul and is compared to %ul\n", target, SECURE_MINTIMER_MASK, target & SECURE_MINTIMER_MASK, _secure_mintimer_high_cnt);

    return (target & SECURE_MINTIMER_MASK) == _secure_mintimer_high_cnt;
#else
    (void)target;
    return 1;
#endif
}

/**
 * @brief compare two timers' target values, return the one with lower value.
 *
 * if either is NULL, return the other.
 * if both are NULL, return NULL.
 */
static inline secure_mintimer_t* SM_FUNC(sancus_sm_timer) _compare(secure_mintimer_t *a, secure_mintimer_t *b)
{
    if (a && b) {
        return ((a->target <= b->target) ? a : b);
    }
    else {
        return (a ? a : b);
    }
}

/**
 * @brief merge two timer lists, return head of new list
 */
static secure_mintimer_t* SM_FUNC(sancus_sm_timer) _merge_lists(secure_mintimer_t *head_a, secure_mintimer_t *head_b)
{
    secure_mintimer_t *result_head = _compare(head_a, head_b);
    secure_mintimer_t *pos = result_head;

    while (1) {
        head_a = head_a->next;
        head_b = head_b->next;
        if (!head_a) {
            pos->next = head_b;
            break;
        }
        if (!head_b) {
            pos->next = head_a;
            break;
        }

        pos->next = _compare(head_a, head_b);
        pos = pos->next;
    }

    return result_head;
}

/**
 * @brief parse long timers list and copy those that will expire in the current
 *        short timer period
 */
static void SM_FUNC(sancus_sm_timer) _select_long_timers(void)
{
    secure_mintimer_t *select_list_start = long_list_head;
    secure_mintimer_t *select_list_last = NULL;

    /* advance long_list head so it points to the first timer of the next (not
     * just started) "long timer period" */
    // SECMIN_DEBUG(sancus_debug1("Current long_cnt: %lu\n", _long_cnt));
    while (long_list_head) {
        // SECMIN_DEBUG(sancus_debug2("Stuff in long list: target: %lu ;long target: %lu\n", long_list_head->target, long_list_head->long_target));
        if ((long_list_head->long_target <= _long_cnt) && _this_high_period(long_list_head->target)) {
            select_list_last = long_list_head;
            long_list_head = long_list_head->next;
        }
        else {
            /* remaining long_list timers belong to later long periods */
            break;
        }
    }

    /* cut the "selected long timer list" at the end */
    if (select_list_last) {
        select_list_last->next = NULL;
    }

    /* merge "current timer list" and "selected long timer list" */
    if (timer_list_head) {
        if (select_list_last) {
            /* both lists are non-empty. merge. */
            timer_list_head = _merge_lists(timer_list_head, select_list_start);
        }
        else {
            /* "selected long timer list" is empty, nothing to do */
        }
    }
    else { /* current timer list is empty */
        if (select_list_last) {
            /* there's no current timer list, but a non-empty "selected long
             * timer list".  So just use that list as the new current timer
             * list.*/
            timer_list_head = select_list_start;
        }
    }
}

/**
 * @brief handle low-level timer overflow, advance to next short timer period
 */
// int temp_counter = 0;
static void SM_FUNC(sancus_sm_timer) _next_period(void)
{
    #if SECURE_MINTIMER_MASK
            /* advance <32bit mask register */
            _secure_mintimer_high_cnt += ~SECURE_MINTIMER_MASK + 1;
            // SECMIN_DEBUG(sancus_debug2("high count: %lu, long_count: %lu\n", _secure_mintimer_high_cnt, _long_cnt));
            if (_secure_mintimer_high_cnt == 0) {
                /* high_cnt overflowed, so advance >32bit counter */
                _long_cnt++;
                // SECMIN_DEBUG(sancus_debug1("Advancing long count %lu\n", _long_cnt));
            }
        
    #else
        /* advance >32bit counter */
        _long_cnt++;
    #endif

        /* swap overflow list to current timer list */
        timer_list_head = overflow_list_head;
        overflow_list_head = NULL;

        _select_long_timers();
}

void SM_FUNC(sancus_sm_timer) secure_mintimer_timer_callback(void){
    _timer_callback();
}


/**
 * @brief main secure_mintimer callback function
 */
static void SM_FUNC(sancus_sm_timer) _timer_callback(void)
{   
    uint32_t next_target;
    uint32_t reference;

    _in_handler = 1;

    // SECMIN_DEBUG(sancus_debug3("_timer_callback() now=%" PRIu32 " (%" PRIu32 ")pleft=%" PRIu32 "\n",
    //       _secure_mintimer_now(), _secure_mintimer_lltimer_mask(_secure_mintimer_now()),
    //       _secure_mintimer_lltimer_mask(0xffffffff - _secure_mintimer_now())));
    if (!timer_list_head) {
        // SECMIN_DEBUG(sancus_debug("_timer_callback(): tick\n"));
        /* there's no timer for this timer period,
         * so this was a timer overflow callback.
         *
         * In this case, we advance to the next timer period.
         */
        _next_period();

        reference = 0;

        /* make sure the timer counter also arrived
         * in the next timer period */
        while (_secure_mintimer_lltimer_now() == _secure_mintimer_lltimer_mask(0xFFFFFFFF)) {}
    }
    else {
        /* we ended up in _timer_callback and there is
         * a timer waiting.
         */
        /* set our period reference to the current time. */
        reference = _secure_mintimer_lltimer_now();
    }

overflow:
    /* check if next timers are close to expiring */
    while (timer_list_head && (_time_left(_secure_mintimer_lltimer_mask(timer_list_head->target), reference) < SECURE_MINTIMER_ISR_BACKOFF)) {
        /* make sure we don't fire too early. With Sancus we never fire too early */
        while (_time_left(_secure_mintimer_lltimer_mask(timer_list_head->target), reference)) {}

        /* pick first timer in list */
        secure_mintimer_t *timer = timer_list_head;

        /* advance list */
        timer_list_head = timer->next;

        /* make sure timer is recognized as being already fired */
        timer->target = 0;
        timer->long_target = 0;

        _shoot_timer(timer);
    }

    /* possibly executing all callbacks took enough
     * time to overflow.  In that case we advance to
     * next timer period and check again for expired
     * timers.*/
    /* check if the end of this period is very soon */
    uint32_t now = _secure_mintimer_lltimer_now() + SECURE_MINTIMER_ISR_BACKOFF;
    if (now < reference) {
        SECMIN_DEBUG(sancus_debug1("_timer_callback: overflowed while executing callbacks. %i\n",
              timer_list_head != NULL));
        _next_period();
        /* wait till overflow */
        while( reference < _secure_mintimer_lltimer_now()){}
        reference = 0;
        goto overflow;
    }

    if (timer_list_head) {
        /* schedule callback on next timer target time */
        next_target = timer_list_head->target - SECURE_MINTIMER_OVERHEAD;
    //     /* make sure we're not setting a time in the past */
    //     // LOG_ERROR("Marker 1. Target is %lu and now is %lu. Compared back-off %ul against next_target %u\n", timer_list_head->target, reference, _secure_mintimer_now() + SECURE_MINTIMER_ISR_BACKOFF, next_target);
        if (next_target < (_secure_mintimer_now() + SECURE_MINTIMER_ISR_BACKOFF)) {
            goto overflow;
        }
    }
    else {
        /* there's no timer planned for this timer period */
        /* schedule callback on next overflow */
        next_target = _secure_mintimer_lltimer_mask(0xFFFFFFFF);
        uint32_t now = _secure_mintimer_lltimer_now();
    
        /* check for overflow again */
        if (now < reference) {
            _next_period();
            reference = 0;
            goto overflow;
        }
        else {
            /* check if the end of this period is very soon */
            if (_secure_mintimer_lltimer_mask(now + SECURE_MINTIMER_ISR_BACKOFF) < now) {
                /* spin until next period, then advance */
                while (_secure_mintimer_lltimer_now() >= now) {}
                _next_period();
                reference = 0;
                goto overflow;
            }
        }
    }

    _in_handler = 0;

    /* set low level timer */
    _lltimer_set(next_target);

    // And reset the IFG of CCR0. Technically, this should be done in timer.c and included in timer.h
    TIMER_BASE->CTL &= ~(TIMER_CTL_IFG);
}

/**
 * From old secure_mintimer.c
 * */

// static void SM_FUNC(sancus_sm_timer) _callback_unlock_mutex(void* arg)
// {
//     mutex_t *mutex = (mutex_t *) arg;
//     mutex_unlock(mutex);
// }

void SM_FUNC(sancus_sm_timer) _secure_mintimer_tsleep_specific_pid(uint32_t offset, kernel_pid_t pid){
    SECMIN_DEBUG(sancus_debug1("timer sleep called with %lu offset", offset));


    // Look for an empty timer to use
    secure_mintimer_t* timer = get_available_timer(pid);
    
    if(timer == NULL){
        SECMIN_DEBUG(sancus_debug("timer sleep: Found no empty timer, not sleeping."));
    } else {
        // Set the active status to sleeping and set the timer 
        timer->target = timer->long_target = 0;
        timer->thread = &sched_threads[pid];
        sched_set_status(timer->thread, STATUS_SLEEPING);
        _secure_mintimer_set(timer, offset);
    }

}

void SM_FUNC(sancus_sm_timer) _secure_mintimer_tsleep_internal(uint32_t offset){
    _secure_mintimer_tsleep_specific_pid(offset, sched_active_thread->pid);
}

void _secure_mintimer_tsleep(USED_IN_ASM uint32_t offset, uint32_t long_offset)
{
    // We ignore long offset for now.
    // Consider adding a long_offset timer by setting it up in the scheduler first.
    if(long_offset != 0){
        LOG_WARNING("Secure Mintimer sleep: Ignoring long offset\n");
    }

    // move offset into r13 and r12
    __asm__("mov r15, r13");
    __asm__("mov r14, r12");

    // perform a full save context and exitless call
    ___MACRO_EXITLESS_CALL_WITH_RESUME(EXITLESS_FUNCTION_TYPE_SLEEP)

    return;
}
