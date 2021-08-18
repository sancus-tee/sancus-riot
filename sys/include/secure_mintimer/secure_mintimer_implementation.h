/*
 * Copyright (C) 2015 Kaspar Schleiser <kaspar@schleiser.de>
 *               2016 Eistec AB
 *               2018 Josua Arndt
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup   sys_secure_mintimer

 * @{
 * @file
 * @brief   secure_mintimer implementation
 *
 * @author  Kaspar Schleiser <kaspar@schleiser.de>
 * @author  Joakim Nohlgård <joakim.nohlgard@eistec.se>
 * @author  Josua Arndt <jarndt@ias.rwth-aachen.de>
 *
 */
#ifndef SECURE_MINTIMER_IMPLEMENTATION_H
#define SECURE_MINTIMER_IMPLEMENTATION_H

#ifndef SECURE_MINTIMER_H
#error "Do not include this file directly! Use secure_mintimer.h instead"
#endif

#include "periph/timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The number of microseconds per second
 */
#ifndef US_PER_SEC
#define US_PER_SEC (1000000U)
#endif
/**
 * @brief The number of nanoseconds per microsecond
 */
#ifndef NS_PER_US
#define NS_PER_US  (1000U)
#endif




/**
 * @{
 * @brief secure_mintimer internal stuff
 * @internal
 */

uint64_t _secure_mintimer_now64(void);

/**
 * @brief Sets the timer to the appropriate timer_list or list_head.
 *
 * @note    The target to set the timer to has to be at least bigger then the
 *          ticks needed to jump into the function and calculate '_secure_mintimer_now()'.
 *          So that 'now' did not pass the target.
 *          This is crucial when using low CPU frequencies and/or when the
 *          '_secure_mintimer_now()' call needs multiple secure_mintimer ticks to evaluate.
 *
 * @param[in] timer   pointer to secure_mintimer_t which is added to the list.
 * @param[in] target  Absolute target value in ticks.
 */
// int _secure_mintimer_set_absolute(secure_mintimer_t *timer, uint32_t target);
// void _secure_mintimer_set64(secure_mintimer_t *timer, uint32_t offset, uint32_t long_offset);
// void _secure_mintimer_set_wakeup(secure_mintimer_t *timer, uint32_t offset, kernel_pid_t pid);
// void _secure_mintimer_set_wakeup64(secure_mintimer_t *timer, uint64_t offset, kernel_pid_t pid);


/**
 * @brief  Sleep for the given number of ticks
 */
void _secure_mintimer_tsleep(uint32_t offset, uint32_t long_offset);
/** @} */

static inline secure_mintimer_ticks32_t secure_mintimer_now(void)
{
    secure_mintimer_ticks32_t ret;
    ret.ticks32 = _secure_mintimer_now();
    return ret;
}

static inline secure_mintimer_ticks64_t secure_mintimer_now64(void)
{
    secure_mintimer_ticks64_t ret;
    ret.ticks64 = _secure_mintimer_now64();
    return ret;
}

static inline uint32_t secure_mintimer_now_usec(void)
{
    return secure_mintimer_usec_from_ticks(secure_mintimer_now());
}

static inline uint64_t secure_mintimer_now_usec64(void)
{
    return secure_mintimer_usec_from_ticks64(secure_mintimer_now64());
}

static inline void _secure_mintimer_tsleep32(uint32_t ticks)
{
    _secure_mintimer_tsleep(ticks, 0);
}

static inline void _secure_mintimer_tsleep64(uint64_t ticks)
{
    _secure_mintimer_tsleep((uint32_t)ticks, (uint32_t)(ticks >> 32));
}

// Not allowed
// static inline void secure_mintimer_spin(secure_mintimer_ticks32_t ticks) {
//     _secure_mintimer_spin(ticks.ticks32);
// }

static inline void secure_mintimer_usleep(uint32_t microseconds)
{
    _secure_mintimer_tsleep32(_secure_mintimer_ticks_from_usec(microseconds));
}

static inline void secure_mintimer_usleep64(uint64_t microseconds)
{
    _secure_mintimer_tsleep64(_secure_mintimer_ticks_from_usec64(microseconds));
}

static inline void secure_mintimer_sleep(uint32_t seconds)
{
    _secure_mintimer_tsleep64(_secure_mintimer_ticks_from_usec64((uint64_t)seconds * US_PER_SEC));
}

static inline void secure_mintimer_nanosleep(uint32_t nanoseconds)
{
    _secure_mintimer_tsleep32(_secure_mintimer_ticks_from_usec(nanoseconds / NS_PER_US));
}

static inline void secure_mintimer_tsleep32(secure_mintimer_ticks32_t ticks)
{
    _secure_mintimer_tsleep32(ticks.ticks32);
}

static inline void secure_mintimer_tsleep64(secure_mintimer_ticks64_t ticks)
{
    _secure_mintimer_tsleep64(ticks.ticks64);
}


// Removed
// static inline void secure_mintimer_set64(secure_mintimer_t *timer, uint64_t period_us)
// {
//     uint64_t ticks = _secure_mintimer_ticks_from_usec64(period_us);
//     _secure_mintimer_set64(timer, ticks, ticks >> 32);
// }

static inline uint32_t secure_mintimer_usec_from_ticks(secure_mintimer_ticks32_t ticks)
{
    return _secure_mintimer_usec_from_ticks(ticks.ticks32);
}

static inline uint64_t secure_mintimer_usec_from_ticks64(secure_mintimer_ticks64_t ticks)
{
    return _secure_mintimer_usec_from_ticks64(ticks.ticks64);
}

#ifdef __cplusplus
}
#endif

#endif /* SECURE_MINTIMER_IMPLEMENTATION_H */
