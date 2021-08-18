#include <msp430.h>

#include "uart.h"
#include "uart_hardware.h"

#include <stdint.h>

#define RX_BUFFER_SIZE 128

// callback that is called for every byte received.
uart_receive_cb receive_cb;

// one element is wasted in the buffer to be able to make a distinction between
// a full and an empty buffer.
static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];

// points to the index where the next element should be written
static volatile size_t rx_head;

// points to the index where the next element should be read iff not equal to
// rx_head, in which case the buffer is empty
static volatile size_t rx_tail;

static void uart_append_byte(unsigned char);



void uart_init(void ) 
{
    receive_cb = uart_append_byte;
    rx_head = rx_tail = 0;
    UART_BAUD = BAUD;
    UART_CTL = UART_EN | UART_IEN_RX;
    UART2_BAUD = BAUD;
    UART2_CTL = UART_EN;
}

void uart_set_receive_cb(uart_receive_cb cb)
{
    receive_cb = cb;
}

size_t uart_available(void)
{
    size_t head = rx_head;
    size_t tail = rx_tail;

    if (head >= tail)
        return head - tail;
    else
        return RX_BUFFER_SIZE + head - tail;
}

void uart_write_byte(unsigned char b)
{
    // wait while TX buffer is full
    while (UART_STAT & UART_TX_FULL) {}

    // write byte to TX buffer
    UART_TXD = b;
}

void uart_write(const unsigned char* buf, size_t size)
{
    while (size--)
        uart_write_byte(*buf++);
}

unsigned char uart_read_byte(void)
{
    size_t i = rx_tail;

    while (i == rx_head) {}

    uint8_t ret = rx_buffer[i];
    rx_tail = (i + 1) % RX_BUFFER_SIZE;
    return ret;
}

void uart_read(unsigned char* buf, size_t size)
{
    while (size--)
        *buf++ = uart_read_byte();
}

void uart_flush(void)
{
    while (UART_STAT & UART_TX_BUSY) {}
}

static void __attribute__((interrupt(UART_RX_VECTOR))) uart_receive(void)
{
    // Read the received data
    uint8_t byte = UART_RXD;

    receive_cb(byte);

    // Clear the receive pending flag
    UART_STAT = UART_RX_PND;
}

static void uart_append_byte(unsigned char b)
{
    size_t i = rx_head;
    size_t next_head = (i + 1) % RX_BUFFER_SIZE;

    if (next_head != rx_tail) // drop byte if buffer is full
    {
        rx_buffer[i] = b;
        rx_head = next_head;
    }
    uart_write_byte(b);
}

void uart2_write_byte(unsigned char b)
{
    // wait while TX buffer is full
    while (UART2_STAT & UART_TX_FULL) {}

    // write byte to TX buffer
    UART2_TXD = b;
}

void uart_print_receive_buffer(void){
    for(int i = 0; i < RX_BUFFER_SIZE; i++){
        uart_write_byte(rx_buffer[i]);
    }
}