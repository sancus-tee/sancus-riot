#include <msp430.h>
#include <stdio.h>
#include "kernel_defines.h"
#include "secure_mintimer.h"
#include "log.h"
#include "sancus_helpers.h"


const char *sm1_name = "EXTRA SM 1";
const char *sm2_name = "EXTRA SM 2";

static char sm1_unprotected_stack[THREAD_EXTRA_STACKSIZE_PRINTF];
static char sm2_unprotected_stack[THREAD_EXTRA_STACKSIZE_PRINTF];

DECLARE_SM(foo, 0x1234);
DECLARE_SM(bar, 0x1234);

bool sm_1_done = false;
bool sm_2_done = false;

void SM_ENTRY(foo) foo_greet(void)
{
    pr_info("Hello from first SM");
    // Do a clix for 3 cycles
    __asm__("mov #3, r15");
    __asm__(".word 0x1389");
    // Do 4 nops
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    pr_info("Normal clix works!");
    ___MACRO_CALL_THREAD_EXIT_FROM_SM(foo)
}


void SM_ENTRY(bar) bar_greet(void)
{
    pr_info("Second SM attempting to create violation..");
    // Do another clix for 3 cycles
    __asm__("mov #3, r15");
    __asm__(".word 0x1389");

    // Wait just 3 cycles to try following a clix with another one
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");

    // Attempt to create violation with a second clix right after the first one expired
    __asm__(".word 0x1389");

}

int main(void)
{
    LOG_INFO("######## Riot on Sancus\n");
    LOG_INFO("Testing atomic violation\n");

    while(sancus_enable(&foo) == 0);
    while(sancus_enable(&bar) == 0);

    thread_create_protected(
        sm1_unprotected_stack,              // Unprocted stack for OCALLS (will be initialized by function)
        THREAD_EXTRA_STACKSIZE_PRINTF,      // size of the unprotected stack
        1,                                  // Priority to give. There are 16 levels, 1 is highest prio and 1-4 are special(but not really)
        THREAD_CREATE_WOUT_YIELD,           // Thread create flag. Here, do not yield immediately but just create thread and return
        SM_GET_ENTRY(foo),                  // SM Entry address 
        SM_GET_ENTRY_IDX(foo, foo_greet),   // SM IDX address
        sm1_name);                          // Name. Just for console logging
    thread_create_protected(
        sm2_unprotected_stack, 
        THREAD_EXTRA_STACKSIZE_PRINTF, 
        2, 
        THREAD_CREATE_WOUT_YIELD, 
        SM_GET_ENTRY(bar), 
        SM_GET_ENTRY_IDX(bar, bar_greet),
        sm2_name);

    // To create unprotected threads, do this:
    // thread_create(main_stack, sizeof(main_stack), // Stack to use for this thread, will be initialized by function (just pass an array)
    //     THREAD_PRIORITY_MAIN,                        // Priority
    //     THREAD_CREATE_WOUT_YIELD ,                   // Flags
    //     main_trampoline, NULL, main_name);           //function pointer, argument pointer, name
    
    LOG_INFO("Thread initialization done\n");
    LOG_INFO("Yielding until both SMs are done.\n");
    thread_yield_higher();
    LOG_INFO("Both threads are finished.\n");
    LOG_INFO("Exiting main thread by shutting down CPU\n");
    sched_shut_down();
    // we could also just exit the main thread if we don't want to shut down
    // cpu_switch_context_exit();

    UNREACHABLE();
    return 0;
}
