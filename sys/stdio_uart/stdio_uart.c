/*
 * Copyright (C) 2013 INRIA
 *               2015 Kaspar Schleiser <kaspar@schleiser.de>
 *               2016 Eistec AB
 *               2018 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     sys
 * @{
 *
 * @file
 * @brief       STDIO over UART implementation
 *
 * This file implements a UART callback and the STDIO read/write functions
 *
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 * @author      Ludwig Knüpfer <ludwig.knuepfer@fu-berlin.de>
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Joakim Nohlgård <joakim.nohlgard@eistec.se>
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <errno.h>

#include "stdio_uart.h"

#include "board.h"
#include "periph/uart.h"

#define ENABLE_DEBUG 0
#include "debug.h"

void stdio_init(void)
{
    uart_rx_cb_t cb;
    void *arg;


    cb = NULL;
    arg = NULL;

    uart_init(STDIO_UART_DEV, STDIO_UART_BAUDRATE, cb, arg);

}

ssize_t stdio_read(void* buffer, size_t count)
{
    (void)buffer;
    (void)count;
    return -ENOTSUP;
}

ssize_t stdio_write(const void* buffer, size_t len)
{
    uart_write(STDIO_UART_DEV, (const uint8_t *)buffer, len);
    return len;
}
