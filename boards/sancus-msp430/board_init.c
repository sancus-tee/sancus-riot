/*
 * Copyright (C) 2014 INRIA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup boards
 * @{
 * @file
 * @brief       msb-430 common board initialization
 *
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 *
 * @}
 */

#include <msp430.h>
#include "cpu.h"
#include "irq.h"
#include "board.h"
#include "stdio_base.h"
#include "periph_conf.h"
#include "debug.h"
#include "uart.h"
#include "log.h"


void board_init(void)
{
    // Disable watchdog
    WDTCTL = WDTPW | WDTHOLD;
    // Init cpu and periphs
    msp430_cpu_init();
    uart_init();
    /* finally initialize STDIO */
    stdio_init();

    puts("\n\n");
    LOG_INFO("Riot on Sancus booting...\n");
    LOG_INFO("Board initialized correctly\n");
}
