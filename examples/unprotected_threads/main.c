#include <msp430.h>
#include <stdio.h>
#include "kernel_defines.h"
#include "secure_mintimer.h"
#include "log.h"
#include "sancus_helpers.h"


const char *t1_name = "Thread 1";
const char *t2_name = "Thread 2";

static char t1_unprotected_stack[THREAD_EXTRA_STACKSIZE_PRINTF];
static char t2_unprotected_stack[THREAD_EXTRA_STACKSIZE_PRINTF];

static void *foo_thread(void *arg)
{
    (void) arg;
    printf("%s: Time: %llu.\n",(const char*)arg, secure_mintimer_now_usec64());
    printf("%s: Yielding...\n", (const char*)arg);
    thread_yield_higher();
    printf("%s: Done!\n", (const char*)arg);

    cpu_switch_context_exit();

    UNREACHABLE();
    return NULL;
}

int main(void)
{
    LOG_INFO("######## Riot on Sancus\n");
    LOG_INFO("Creating two threads that will each print a time, yield, and then exit.\n");
    thread_create(t1_unprotected_stack, sizeof(t1_unprotected_stack),
            THREAD_PRIORITY_MAIN-1, // 15 is lowest, 1 highest prio
            THREAD_CREATE_WOUT_YIELD | THREAD_CREATE_STACKTEST,
            foo_thread, (void *)t1_name, t1_name);
    // t2 uses same function but different 🧵
    thread_create(t2_unprotected_stack, sizeof(t2_unprotected_stack),
            THREAD_PRIORITY_MAIN-1,
            THREAD_CREATE_WOUT_YIELD | THREAD_CREATE_STACKTEST,
            foo_thread, (void *)t2_name, t2_name);

    LOG_INFO("Thread initialization done\n");
    LOG_INFO("Yielding until both threads are done.\n");
    thread_yield_higher();
    LOG_INFO("Both threads are finished.\n");
    LOG_INFO("Exiting main thread by shutting down CPU\n");
    sched_shut_down();
    // we could also just exit the main thread if we don't want to shut down
    // cpu_switch_context_exit();

    UNREACHABLE();
    return 0;
}
