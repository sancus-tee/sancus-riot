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

#ifndef TIME_H
#define TIME_H

#include <sys/types.h>
#include "msp430_types.h"

#include "sancus_modules.h"
#include "sancus_helpers.h"

#ifdef __cplusplus
extern "C" {
#endif

int SM_FUNC(sancus_sm_timer) sm_timer_init(tim_t dev, unsigned long freq, timer_cb_t cb);
void SM_FUNC(sancus_sm_timer) sm_timer_set_absolute(int channel, unsigned int value);
unsigned int SM_FUNC(sancus_sm_timer) sm_timer_read_internal(tim_t dev);
void SM_FUNC(sancus_sm_timer) sm_timer_start();
void SM_FUNC(sancus_sm_timer) sm_timer_stop();

// For hardware deployments, use a divider of 8 to clock down the timera
// #define TIMERA_CLOCK_DIVIDER TIMER_CTL_ID_DIV8
// For simulations, no clock divider is fine
#ifndef TIMERA_CLOCK_DIVIDER
#define TIMERA_CLOCK_DIVIDER TIMER_CTL_ID_DIV4
#endif

/**
 * @brief Datatype to represent time.
 */
struct tm {
    int tm_sec;     /**< Seconds after the minute [0, 60] */
    int tm_min;     /**< Minutes after the hour [0, 59] */
    int tm_hour;    /**< Hours since midnight [0, 23] */
    int tm_mday;    /**< Day of the month [1, 31] */
    int tm_mon;     /**< Months since January [0, 11] */
    int tm_year;    /**< Years since 1900 */
    int tm_wday;    /**< Days since Sunday [0, 6] */
    int tm_yday;    /**< Days since January 1st [0, 365] */
    int tm_isdst;   /**< Daylight saving time is in effect
                     *   (positive if true, 0 if not, negative if n/a) */
};

#ifdef __cplusplus
}
#endif

#endif /* TIME_H */
/** @} */
