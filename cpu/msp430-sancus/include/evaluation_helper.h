#ifndef EVAL_HELPER_H
#define EVAL_HELPER_H
#include <stdio.h>
#include <stdbool.h>
#include "msp430_regs.h"
#include "time.h"

/**
 * Helpers for performing timing evaluations.
 * */
// #define EVALUATION_ENABLED
#ifdef EVALUATION_ENABLED
/**
 * Evaluation helpers
 * */
typedef enum {
    TIMING_TYPE_UNUSED,
    TIMING_TYPE_THREAD_CREATE,
    TIMING_TYPE_SWITCH_PERIODIC,
    TIMING_TYPE_CONTEXT_EXIT,
    TIMING_TYPE_SLEEP,
    TIMING_TYPE_YIELD,
    TIMING_TYPE_GET_TIME
} timing_measurement_type_t;
struct _measurement
{
    uint32_t start_short;
    uint32_t start_long;
    uint32_t end_short;
    uint32_t end_long;
    timing_measurement_type_t type;
    char* desc;
};
typedef struct _measurement timing_measurement_t;

#define EVALUATION_TIMING_SIZE 100
extern timing_measurement_t timings[EVALUATION_TIMING_SIZE];
extern uint16_t timing_counter;
extern bool timing_running;
extern uint8_t clock_divider;

void init_eval_helper(void);

// Defines to start, stop a timing and do a full one. Only considers lower 32 bit counters.
// With the shared 
#define ___MACRO_START_TIMING(TYPE, DESC)                                   \
timing_running = true;                                                      \
timings[timing_counter].type = TYPE;                                        \
timings[timing_counter].desc = DESC;                                        \
timings[timing_counter].start_long = _secure_mintimer_high_cnt;             \
timings[timing_counter].start_short =TIMER_A->R;

#define ___MACRO_END_TIMING                                                 \
if(timing_running){                                                         \
timings[timing_counter].end_short = TIMER_A->R;                           \
timings[timing_counter].end_long = _secure_mintimer_high_cnt;             \
timing_counter++;                                                           \
timing_running = false;                                                     \
}

// Measures the time for function f.
#define ___MACRO_MEASURE_TIME(f, TYPE, DESC)                                \
___MACRO_START_TIMING(TYPE, DESC)                                           \
f;                                                                          \
___MACRO_END_TIMING

#endif

#endif /* EVAL_HELPERS_H */
/** @} */