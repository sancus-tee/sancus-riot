#ifndef SANCUS_SUPPORT_UART_H
#define SANCUS_SUPPORT_UART_H

#include <stddef.h>

typedef void (*uart_receive_cb)(unsigned char);



void uart_init(void);
void uart_set_receive_cb(uart_receive_cb cb);
size_t uart_available(void);
void uart_write_byte(unsigned char b);
void uart_write(const unsigned char* buf, size_t size);
unsigned char uart_read_byte(void);
void uart_read(unsigned char* buf, size_t size);
void uart_flush(void);
void uart_print_receive_buffer(void);
void uart2_write_byte(unsigned char b);

#endif