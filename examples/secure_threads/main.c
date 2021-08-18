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
    pr_info2("Hi from FOO SM with ID %d, called by %d\n",
        sancus_get_self_id(), sancus_get_caller_id());
    
    while(!sm_2_done){
        /*
         * The sleep call from SMs is different since is needs to be a MACRO.
         * If we just used a simple library, this would result in an OCALL which we don't want.
         * Thus, the MACRO requires the 16 lower and 16 higher bits of the to sleeping ticks.
         * It also requires the SM name as the other SM macros do.
         * NOTE: The timer_sleep does NOT guarantee that the thread slept this amount of time upon returning!
         * Right now, sleep aborts if there is no available timer (SECURE_MINTIMER_TIMER_LIST_LENGTH == 10). 
         * An SM could avoid this by checking current system time before and after the sleep call to verify that sleeping worked..
         */
        pr_info("FOO: Other thread not done yet...I am sleeping.");
        ___MACRO_CALL_SLEEP_FROM_SM(0x10,0, foo)
    }
    pr_info("FOO: Hello again. Other thread is done, I am exiting too...\n");
    sm_1_done = true;
    ___MACRO_CALL_THREAD_EXIT_FROM_SM(foo)
}


void SM_ENTRY(bar) bar_greet(void)
{
    pr_info2("Hi from BAR SM with ID %d, called by %d\n",
        sancus_get_self_id(), sancus_get_caller_id());

    pr_info("BAR: Yielding...");
    ___MACRO_CALL_THREAD_YIELD_FROM_SM(bar);

    pr_info("BAR: Hello again. Sleeping for some time...\n");
    ___MACRO_CALL_SLEEP_FROM_SM(0x00,0, bar)

    pr_info("BAR: Hello again. Exiting...\n");
    sm_2_done = true;
    ___MACRO_CALL_THREAD_EXIT_FROM_SM(bar)
}

int main(void)
{
    LOG_INFO("######## Riot on Sancus\n");
    LOG_INFO("Simple secure threads application\n");
    printf("Testing colored logs..");
    LOG_DEBUG("Debug ");
    LOG_INFO("Info ");
    LOG_WARNING("Warning ");
    LOG_ERROR("and ERROR\n");

    LOG_INFO("Creating enclaves...\n");
    while(sancus_enable(&foo) == 0);
    while(sancus_enable(&bar) == 0);

    thread_create_protected(
        sm1_unprotected_stack,              // Unprotected stack for OCALLS (will be initialized by function)
        THREAD_EXTRA_STACKSIZE_PRINTF,      // size of the unprotected stack
        2,                                  // Priority to give. There are 16 levels, 1 is highest prio and 1-4 are special(but not really)
        THREAD_CREATE_WOUT_YIELD,           // Thread create flag. Here, do not yield immediately but just create thread and return
        SM_GET_ENTRY(foo),                  // SM Entry address 
        SM_GET_ENTRY_IDX(foo, foo_greet),   // SM IDX address
        sm1_name);                          // Name. Just for console logging
    thread_create_protected(
        sm2_unprotected_stack, 
        THREAD_EXTRA_STACKSIZE_PRINTF, 
        3, 
        THREAD_CREATE_WOUT_YIELD, 
        SM_GET_ENTRY(bar), 
        SM_GET_ENTRY_IDX(bar, bar_greet),
        sm2_name);

    LOG_INFO("Thread initialization done\n");
    LOG_INFO("Sleeping until both SMs are done.\n");
    while(!sm_1_done && !sm_2_done){
        LOG_INFO("MAIN: Sleeping 10000 usecs until both SMs are done.\n");
        // sleep 10ms which does not reflect too well in the simulator.
        secure_mintimer_usleep(10000); 
    }
    LOG_INFO("MAIN: Both threads have signaled that they are finished.\n");
    LOG_INFO("MAIN: Shutting down CPU\n");
    sched_shut_down();
    // we could also just exit the main thread if we don't want to shut down
    // cpu_switch_context_exit();

    UNREACHABLE();
    return 0;
}
