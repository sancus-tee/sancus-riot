/*
 * Copyright (C) 2016 Eistec AB
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
 * @brief   secure_mintimer tick <-> seconds conversions for different values of SECURE_MINTIMER_HZ
 * @author  Joakim Nohlgård <joakim.nohlgard@eistec.se>
 */

#ifndef SECURE_MINTIMER_TICK_CONVERSION_H
#define SECURE_MINTIMER_TICK_CONVERSION_H

#ifndef SECURE_MINTIMER_H
#error "Do not include this file directly! Use secure_mintimer.h instead"
#endif

#include "div.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Some optimizations for common timer frequencies */
#if (SECURE_MINTIMER_SHIFT != 0)
#if (SECURE_MINTIMER_HZ % 15625 != 0)
#error SECURE_MINTIMER_HZ must be a multiple of 15625 (5^6) when using SECURE_MINTIMER_SHIFT
#endif
#if (SECURE_MINTIMER_HZ > 1000000ul)
#if (SECURE_MINTIMER_HZ != (1000000ul << SECURE_MINTIMER_SHIFT))
#error SECURE_MINTIMER_HZ != (1000000ul << SECURE_MINTIMER_SHIFT)
#endif
/* SECURE_MINTIMER_HZ is a power-of-two multiple of 1 MHz */
/* e.g. cc2538 uses a 16 MHz timer */
static inline uint32_t _secure_mintimer_ticks_from_usec(uint32_t usec) {
    return (usec << SECURE_MINTIMER_SHIFT); /* multiply by power of two */
}

static inline uint64_t _secure_mintimer_ticks_from_usec64(uint64_t usec) {
    return (usec << SECURE_MINTIMER_SHIFT); /* multiply by power of two */
}

static inline uint32_t _secure_mintimer_usec_from_ticks(uint32_t ticks) {
    return (ticks >> SECURE_MINTIMER_SHIFT); /* divide by power of two */
}

static inline uint64_t _secure_mintimer_usec_from_ticks64(uint64_t ticks) {
    return (ticks >> SECURE_MINTIMER_SHIFT); /* divide by power of two */
}

#else /* !(SECURE_MINTIMER_HZ > 1000000ul) */
#if ((SECURE_MINTIMER_HZ << SECURE_MINTIMER_SHIFT) != 1000000ul)
#error (SECURE_MINTIMER_HZ << SECURE_MINTIMER_SHIFT) != 1000000ul
#endif
/* 1 MHz is a power-of-two multiple of SECURE_MINTIMER_HZ */
/* e.g. ATmega2560 uses a 250 kHz timer */
static inline uint32_t _secure_mintimer_ticks_from_usec(uint32_t usec) {
    return (usec >> SECURE_MINTIMER_SHIFT); /* divide by power of two */
}

static inline uint64_t _secure_mintimer_ticks_from_usec64(uint64_t usec) {
    return (usec >> SECURE_MINTIMER_SHIFT); /* divide by power of two */
}

static inline uint32_t _secure_mintimer_usec_from_ticks(uint32_t ticks) {
    return (ticks << SECURE_MINTIMER_SHIFT); /* multiply by power of two */
}

static inline uint64_t _secure_mintimer_usec_from_ticks64(uint64_t ticks) {
    return (ticks << SECURE_MINTIMER_SHIFT); /* multiply by power of two */
}
#endif /* defined(SECURE_MINTIMER_SHIFT) && (SECURE_MINTIMER_SHIFT != 0) */
#elif SECURE_MINTIMER_HZ == (1000000ul)
/* This is the most straightforward as the secure_mintimer API is based around
 * microseconds for representing time values. */
static inline uint32_t _secure_mintimer_usec_from_ticks(uint32_t ticks) {
    return ticks; /* no-op */
}

static inline uint64_t _secure_mintimer_usec_from_ticks64(uint64_t ticks) {
    return ticks; /* no-op */
}

static inline uint32_t _secure_mintimer_ticks_from_usec(uint32_t usec) {
    return usec; /* no-op */
}

static inline uint64_t _secure_mintimer_ticks_from_usec64(uint64_t usec) {
    return usec; /* no-op */
}

#elif SECURE_MINTIMER_HZ == (32768ul)
/* This is a common frequency for RTC crystals. We use the fact that the
 * greatest common divisor between 32768 and 1000000 is 64, so instead of
 * multiplying by the fraction (32768 / 1000000), we will instead use
 * (512 / 15625), which reduces the truncation caused by the integer widths */
static inline uint32_t _secure_mintimer_ticks_from_usec(uint32_t usec) {
    return div_u32_by_15625div512(usec);
}

static inline uint64_t _secure_mintimer_ticks_from_usec64(uint64_t usec) {
    return div_u64_by_15625div512(usec);
}

static inline uint32_t _secure_mintimer_usec_from_ticks(uint32_t ticks) {
    /* return (usec * 15625) / 512; */
    /* Using 64 bit multiplication to avoid truncating the top 9 bits */
    uint64_t usec = (uint64_t)ticks * 15625ul;
    return (usec >> 9); /* equivalent to (usec / 512) */
}

static inline uint64_t _secure_mintimer_usec_from_ticks64(uint64_t ticks) {
    /* return (usec * 15625) / 512; */
    uint64_t usec = (uint64_t)ticks * 15625ul;
    return (usec >> 9); /* equivalent to (usec / 512) */
}

#else
/* No matching implementation found, try to give meaningful error messages */
#if ((SECURE_MINTIMER_HZ % 15625) == 0)
#error Unsupported hardware timer frequency (SECURE_MINTIMER_HZ), missing SECURE_MINTIMER_SHIFT in board.h? See secure_mintimer.h documentation for more info
#else
#error Unknown hardware timer frequency (SECURE_MINTIMER_HZ), check board.h and/or add an implementation in sys/include/secure_mintimer/tick_conversion.h
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* SECURE_MINTIMER_TICK_CONVERSION_H */
