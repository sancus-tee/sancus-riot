#include <msp430.h>
#include "uart.h"
#include "uart_hardware.h"
#include <stdio.h>
#include "kernel_defines.h"
#include "secure_mintimer.h"
#include "log.h"
#include "sancus_helpers.h"


#define ___MACRO_CLIX(clix_length)  \
    __asm__("push r15");    \
    __asm__("mov.w %0, r15" : : "i"(clix_length));    \
    __asm__(".word 0x1389");    \
    __asm__("pop r15");


#define _HAVE_APPA            1
#define _HAVE_APPB            1
#define _HAVE_APP_SLEEP       1
#define _HAVE_IO_THREAD       1


/* --- IO Enclave --------------------------------------------------------- */
DECLARE_SM(ioenclave,  0x1234);

#ifdef _HAVE_IO_THREAD
#define IO_BUFS        4
SM_DATA(ioenclave) unsigned char io_bufs[IO_BUFS] = {0, 0, 0, 0};
SM_DATA(ioenclave) bool io_ready[IO_BUFS] = {false, false, false, false};
#endif

// Output 
bool SM_ENTRY(ioenclave) io_uart_write_byte(unsigned char b)
{
#ifdef _HAVE_IO_THREAD
    // Async I/O
    ___MACRO_CLIX(50);
    int caller = (int) sancus_get_caller_id();
    if (!caller || caller >= IO_BUFS) { caller = 0; }
    if (io_ready[caller]) {
      return (false);
    } else {
      io_bufs[caller]  = b;
      io_ready[caller] = true;
      return (true);
    }
#else
    // Sync I/O
    ___MACRO_CLIX(30);
    while (UART_STAT & UART_TX_FULL) {}       // !!
    UART_TXD = b;
    return (true);
#endif
}

// Read sensor
uint64_t SM_ENTRY(ioenclave) io_get_reading(void)
{
    ___MACRO_CLIX(30);
    return (secure_mintimer_now_usec64());
}

#ifdef _HAVE_IO_THREAD
static char sm3_unprotected_stack[THREAD_EXTRA_STACKSIZE_PRINTF];
// Async I/O thread
void SM_ENTRY(ioenclave) io_thread(void)
{
    while (true) {
      // this could implement *any* policy.
      for (int i = 0; i < IO_BUFS; i++) {
        if (io_ready[i]) {
          ___MACRO_CLIX(30);
          while (UART_STAT & UART_TX_FULL) {} // !!
          UART_TXD = io_bufs[i];
          io_ready[i] = false;
        }
      }
#ifdef _HAVE_APP_SLEEP
      ___MACRO_CALL_SLEEP_FROM_SM(0x0100, 0x0001, ioenclave)
#endif
    }
    return;
}
#endif



/* --- APP A -------------------------------------------------------------- */
#ifdef _HAVE_APPA
static char sm1_unprotected_stack[THREAD_EXTRA_STACKSIZE_PRINTF];
DECLARE_SM(appa,       0x1234);

SM_DATA(appa) uint64_t reading_a = 0;

void SM_ENTRY(appa) a_entry(void)
{
    printf2("A: ID %d, called by %d\n",
        sancus_get_self_id(), sancus_get_caller_id());

    while (true) { 
      reading_a = io_get_reading();
      printf1("A: t is %lu\n", reading_a);
      if (reading_a >= 50000) { io_uart_write_byte('A'); }
#ifdef _HAVE_APP_SLEEP
      ___MACRO_CALL_SLEEP_FROM_SM(0x0100, 0x0001, appa)
#endif
    }
}
#endif


/* --- APP B -------------------------------------------------------------- */
#ifdef _HAVE_APPB
static char sm2_unprotected_stack[THREAD_EXTRA_STACKSIZE_PRINTF];
DECLARE_SM(appb,       0x1234);

SM_DATA(appb) uint64_t reading_b = 0;

void SM_ENTRY(appb) b_entry(void)
{
    printf2("B: ID %d, called by %d\n",
        sancus_get_self_id(), sancus_get_caller_id());

    while (true) {
      reading_b = io_get_reading();
      printf1("B: t is %lu\n", reading_b);
      if (reading_b >= 50000) { io_uart_write_byte('B'); }
#ifdef _HAVE_APP_SLEEP
      ___MACRO_CALL_SLEEP_FROM_SM(0x0100, 0x0001, appb)
#endif
    }
}
#endif


int main(void)
{
    LOG_INFO("######## Riot on Sancus\n");
    LOG_INFO("Case study with same prio levels\n");

    while(sancus_enable(&ioenclave) == 0);

#ifdef _HAVE_APPA
    while(sancus_enable(&appa) == 0);
#endif
#ifdef _HAVE_APPB
    while(sancus_enable(&appb) == 0);
#endif

#ifdef _HAVE_APPA
    thread_create_protected(
        sm1_unprotected_stack,              // Unprocted stack for OCALLS
        THREAD_EXTRA_STACKSIZE_PRINTF,      // size of the unprotected stack
        2,                                  // Priority to give
        THREAD_CREATE_WOUT_YIELD,           // Thread create flag
        SM_GET_ENTRY(appa),                 // SM Entry address 
        SM_GET_ENTRY_IDX(appa, a_entry),    // SM IDX address
        "A");                               // Name for console logging
#endif
#ifdef _HAVE_APPB
    thread_create_protected(
        sm2_unprotected_stack, 
        THREAD_EXTRA_STACKSIZE_PRINTF, 
        2, 
        THREAD_CREATE_WOUT_YIELD, 
        SM_GET_ENTRY(appb), 
        SM_GET_ENTRY_IDX(appb, b_entry),
        "B");
#endif
#ifdef _HAVE_IO_THREAD
    thread_create_protected(
        sm3_unprotected_stack, 
        THREAD_EXTRA_STACKSIZE_PRINTF, 
        2, 
        THREAD_CREATE_WOUT_YIELD, 
        SM_GET_ENTRY(ioenclave), 
        SM_GET_ENTRY_IDX(ioenclave, io_thread),
        "IO");
#endif

    LOG_INFO("Thread initialization done\n");
    while(true){
        secure_mintimer_usleep(300000); 
    }
    LOG_INFO("Exiting main thread by shutting down CPU\n");
    sched_shut_down();

    UNREACHABLE();
    return 0;
}

