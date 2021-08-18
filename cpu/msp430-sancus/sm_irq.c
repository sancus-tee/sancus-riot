/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu
* @{
 *
 * @file
 * @brief       ISR related functions
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 *
 * @}
 */

#include "irq.h"
#include "cpu.h"

char SM_DATA(sancus_sm_timer) __isr_stack[ISR_STACKSIZE];
volatile int SM_DATA(sancus_sm_timer) __sm_irq_is_in = 0;

unsigned int SM_FUNC(sancus_sm_timer) sm_irq_disable(void)
{
    // unsigned int state;
    // __asm__("mov.w r2,%0" : "=r"(state));
    // state &= GIE;

    // if (state) {
    //     ___MACRO_DISABLE_IRQ
    // }

    // return state;
    return 0;
}

unsigned int SM_FUNC(sancus_sm_timer) sm_irq_enable(void)
{
    // unsigned int state;
    // __asm__("mov.w r2,%0" : "=r"(state));
    // state &= GIE;

    // if (!state) {
    //     ___MACRO_ENABLE_IRQ
    // }

    // return state;
    return 0;
}

void SM_FUNC(sancus_sm_timer) sm_irq_restore(__attribute__((unused)) unsigned int state)
{
    // if (state) {
    //     ___MACRO_ENABLE_IRQ
    // }
}

int SM_FUNC(sancus_sm_timer) sm_irq_is_in(void)
{
    return __sm_irq_is_in;
}
