#include <msp430.h>
#include <stdio.h>
#include "kernel_defines.h"
#include "secure_mintimer.h"
#include "log.h"
#include "sancus_helpers.h"
#include "periph/timer.h"
#include "msp430_regs.h"
#include "evaluation_helper.h"
// #include "time.h"


uint8_t threads_done = 0;

/**
 * Thread creation helpers
 * Note, standard Sancus has a max of 4 SMs (including scheduler)
 * so we create 3 SMs and 12 unprotected threads that sleep
 * */

// Defines to create SMs
#define DEFINE_PERIODIC_SM(name)                            \
static char name##_unprotected_stack[THREAD_EXTRA_STACKSIZE_PRINTF];\
const char *name##_description = "SM " #name;                       \
DECLARE_SM(name, 0x1234);                                           \
void SM_ENTRY(name) name##_init(void){                              \
    kernel_pid_t curr_pid = thread_getpid();                        \
    pr_info1("Initializing SM " #name " with PID..\n", curr_pid);   \
};                                                                  \
void SM_ENTRY(name) name##_greet(void){                             \
    ___MACRO_END_TIMING;                                            \
    ___MACRO_START_TIMING(TIMING_TYPE_GET_TIME, "PER_SM" #name);    \
    uint64_t time = secure_mintimer_now_usec64();                   \
    ___MACRO_END_TIMING;                                            \
    pr_info1("Hi from periodic thread " #name " at %llu..\n", time); \
    ___MACRO_START_TIMING(TIMING_TYPE_YIELD, "PER_SM" #name);       \
    ___MACRO_CALL_THREAD_YIELD_FROM_SM(name)                        \
    ___MACRO_END_TIMING;                                            \
    ___MACRO_START_TIMING(TIMING_TYPE_CONTEXT_EXIT, "PER_SM" #name);        \
    ___MACRO_CALL_THREAD_EXIT_FROM_SM(name)                         \
};

#define DEFINE_SLEEPY_SM(name, lsb, msb)                            \
static char name##_unprotected_stack[THREAD_EXTRA_STACKSIZE_PRINTF];\
const char *name##_description = "SM " #name;                       \
DECLARE_SM(name, 0x1234);                                           \
void SM_ENTRY(name) name##_init(void){                              \
    kernel_pid_t curr_pid = thread_getpid();                        \
    pr_info1("Initializing SM " #name " with PID..\n", curr_pid);   \
};                                                                  \
void SM_ENTRY(name) name##_greet(void){                             \
    ___MACRO_END_TIMING;                                            \
    ___MACRO_START_TIMING(TIMING_TYPE_GET_TIME, "SM" #name);        \
    uint64_t time = secure_mintimer_now_usec64();                   \
    ___MACRO_END_TIMING;                                            \
    pr_info1("Sleeping " #name " at %llu..\n", time);                \
    ___MACRO_START_TIMING(TIMING_TYPE_YIELD, "SM" #name);           \
    ___MACRO_CALL_THREAD_YIELD_FROM_SM(name)                        \
    ___MACRO_END_TIMING;                                            \
    ___MACRO_START_TIMING(TIMING_TYPE_SLEEP, "SM" #name);           \
    ___MACRO_CALL_SLEEP_FROM_SM(lsb, msb, name)                     \
    ___MACRO_END_TIMING;                                            \
    threads_done++;                                                 \
    ___MACRO_START_TIMING(TIMING_TYPE_CONTEXT_EXIT, "SM" #name);            \
    ___MACRO_CALL_THREAD_EXIT_FROM_SM(name)                         \
};

#define CREATE_SM_THREAD(name, prio, pid)            \
    riot_enable_sm(&name);                      \
    pid = thread_create_protected(              \
        name##_unprotected_stack,               \
        THREAD_EXTRA_STACKSIZE_PRINTF,          \
        prio,                                   \
        THREAD_CREATE_WOUT_YIELD,               \
        SM_GET_ENTRY(name),                     \
        SM_GET_ENTRY_IDX(name, name##_greet),   \
        name##_description);                    \
    LOG_INFO("SM " #name " done.");


#define CREATE_AND_INIT_SM(name, prio, pid)     \
    CREATE_SM_THREAD(name, prio, pid)           \
    name##_init();


// Defines to create unprotected threads
// Create unprotected
#define DEFINE_SLEEPY_THREAD(name, time)                \
const char *name##_description = "Unprotected " #name;  \
static char name##_stack[THREAD_EXTRA_STACKSIZE_PRINTF];        \
static void *name##_trampoline(void *arg){              \
        (void) arg;                                     \
        ___MACRO_END_TIMING;                            \
        ___MACRO_START_TIMING(TIMING_TYPE_GET_TIME, "TIME " #name);\
        uint64_t curr = secure_mintimer_now_usec64();   \
        ___MACRO_END_TIMING;                            \
        printf(#name " is sleepy at %llu...\n", curr);  \
        ___MACRO_START_TIMING(TIMING_TYPE_SLEEP, "SLEEP " #name);\
        _secure_mintimer_tsleep32(time);                \
        ___MACRO_END_TIMING;                            \
        ___MACRO_START_TIMING(TIMING_TYPE_YIELD, "YIELD " #name);\
        thread_yield_higher();                          \
        ___MACRO_END_TIMING;                            \
        threads_done++;                                 \
        ___MACRO_START_TIMING(TIMING_TYPE_CONTEXT_EXIT, "EXIT " #name);\
        cpu_switch_context_exit();                      \
    }

#define CREATE_NORMAL_THREAD(name, prio)                \
___MACRO_MEASURE_TIME(                                  \
thread_create(name##_stack, sizeof(name##_stack),       \
    prio,                                               \
    THREAD_CREATE_WOUT_YIELD | THREAD_CREATE_STACKTEST, \
    name##_trampoline,                                  \
    NULL,                                               \
    name##_description),                            \
TIMING_TYPE_THREAD_CREATE, "CREATE " #name)

// Create SMs
DEFINE_PERIODIC_SM(foo)
DEFINE_PERIODIC_SM(bar)
DEFINE_SLEEPY_SM(fooC, 0x0000, 0xdddd)

// Create unprotected ones
DEFINE_SLEEPY_THREAD(fooD, 0x0000aaaa) // 4
DEFINE_SLEEPY_THREAD(fooE, 0x00000f00) //5
DEFINE_SLEEPY_THREAD(fooF, 0x0000A000)
DEFINE_SLEEPY_THREAD(fooG, 0x0000F000)
DEFINE_SLEEPY_THREAD(fooH, 0x0000F000)
DEFINE_SLEEPY_THREAD(fooI, 0x00000000)
DEFINE_SLEEPY_THREAD(fooJ, 0x00000000) //10
DEFINE_SLEEPY_THREAD(fooK, 0x00000000)
DEFINE_SLEEPY_THREAD(fooL, 0x00000000)
DEFINE_SLEEPY_THREAD(fooM, 0x00005000) // 13

// Function to print an eval table from the timing struct
void print_eval_table(timing_measurement_type_t type){
    LOG_WARNING("Printing measurements for type %u ", type);
    switch(type){
        case TIMING_TYPE_THREAD_CREATE:
        LOG_WARNING("(type Create Thread)\n");
        break;
        case TIMING_TYPE_SWITCH_PERIODIC:
        LOG_WARNING("(type Switch to periodic)\n");
        break;
        case TIMING_TYPE_CONTEXT_EXIT:
        LOG_WARNING("(type Context Exit)\n");
        break;
        case TIMING_TYPE_SLEEP:
        LOG_WARNING("(type Sleep)\n");
        break;
        case TIMING_TYPE_YIELD:
        LOG_WARNING("(type Yield)\n");
        break;
        case TIMING_TYPE_GET_TIME:
        LOG_WARNING("(type Get time)\n");
        break;
        default:
        LOG_ERROR("(Type unknown.!)\n");
        break;
    }
    LOG_DEBUG("Duration ticks | Duration cycles | Description \n");

    for(int i=0; i< EVALUATION_TIMING_SIZE; i++){
        if(timings[i].type == type){
            uint32_t start =  timings[i].start_long | timings[i].start_short;
            uint32_t end = timings[i].end_long | timings[i].end_short;
            uint32_t duration = end-start;
            LOG_INFO("%10lu     | %10lu      | %s\n", duration, duration * clock_divider, timings[i].desc);
        }
    }
}


// Lastly, define the final eval thread that will sum up all the data
const char *eval_description = "Eval thread"; 
static char eval_stack[THREAD_STACKSIZE_MAIN];
static void *eval_trampoline(void *arg){    
    (void) arg;                           
    while(threads_done < 10){
        ___MACRO_END_TIMING;
        LOG_WARNING("EVAL: Waiting for more threads done. Have %u\n", threads_done);
        ___MACRO_START_TIMING(TIMING_TYPE_SLEEP, "eval_thread");
        _secure_mintimer_tsleep32(0x00001000);      
    }       
    LOG_ERROR("Eval done. Dumping logs now..\n");
    // Once all threads are done, we can print eval tables
    print_eval_table(TIMING_TYPE_THREAD_CREATE);
    print_eval_table(TIMING_TYPE_SWITCH_PERIODIC);
    print_eval_table(TIMING_TYPE_CONTEXT_EXIT);
    print_eval_table(TIMING_TYPE_SLEEP);
    print_eval_table(TIMING_TYPE_YIELD);
    print_eval_table(TIMING_TYPE_GET_TIME);

    sched_shut_down();
    

    UNREACHABLE();
}

int main(void)
{
    init_eval_helper();

    LOG_INFO("######## Riot on Sancus\n");
    LOG_INFO("Cycle accurate riot evaluation\n");
    LOG_INFO("Remember to set clock divider to 1 or multiply cycles accordingly\n");
    LOG_INFO("Clock divider is currently set to %u.\n", clock_divider);

    kernel_pid_t pid1, pid2, pid3;
    CREATE_AND_INIT_SM(foo, 13, pid1)
    CREATE_AND_INIT_SM(bar, 13, pid2)
    CREATE_AND_INIT_SM(fooC, 13, pid3)

    // Then do a kernel init which returns here because main is higher than prio 13
    ___MACRO_EXITLESS_CALL_WITH_RESUME(EXITLESS_FUNCTION_TYPE_BOOT)
    
    // Now we have started the timer...do our thing

    // Switch two SMs to periodic SMs
    // ___MACRO_MEASURE_TIME(thread_change_to_periodical(pid1, 0xA000, 0x00020000), TIMING_TYPE_SWITCH_PERIODIC, "PERIODIC foo" )
    // ___MACRO_MEASURE_TIME(thread_change_to_periodical(pid2, 0xA000, 0x00020000), TIMING_TYPE_SWITCH_PERIODIC, "PERIODIC bar" )

    // create some normal threads
    CREATE_NORMAL_THREAD(fooD, 4)
    CREATE_NORMAL_THREAD(fooE, 5)
    CREATE_NORMAL_THREAD(fooF, 6)
    CREATE_NORMAL_THREAD(fooG, 7)
    CREATE_NORMAL_THREAD(fooH, 8)
    CREATE_NORMAL_THREAD(fooI, 9)
    CREATE_NORMAL_THREAD(fooJ, 10)
    CREATE_NORMAL_THREAD(fooK, 11)
    CREATE_NORMAL_THREAD(fooL, 12)
    CREATE_NORMAL_THREAD(fooM, 13)
    
    // Create an eval thread
    ___MACRO_MEASURE_TIME(thread_create(eval_stack, sizeof(eval_stack),      
    14,                                              
    THREAD_CREATE_WOUT_YIELD | THREAD_CREATE_STACKTEST,
    eval_trampoline,                                 
    NULL,                                              
    eval_description), TIMING_TYPE_THREAD_CREATE, "CREATE EVAL");


    LOG_INFO("All threads have been started.\n");
    LOG_INFO("Exiting main thread\n");
    ___MACRO_START_TIMING(TIMING_TYPE_CONTEXT_EXIT, "MAIN");
    cpu_switch_context_exit();

    UNREACHABLE();
    return 0;
}
