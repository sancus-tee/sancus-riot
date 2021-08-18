#include "evaluation_helper.h"

#ifdef EVALUATION_ENABLED
timing_measurement_t timings[EVALUATION_TIMING_SIZE];
uint16_t timing_counter = 0;
bool timing_running = false;
uint8_t clock_divider;

void init_eval_helper(){
    switch(TIMERA_CLOCK_DIVIDER){
        case TIMER_CTL_ID_DIV2:
        clock_divider = 2;
        break;
        case TIMER_CTL_ID_DIV4:
        clock_divider = 4;
        break;
        case TIMER_CTL_ID_DIV8:
        clock_divider = 8;
        break;
        default:
        clock_divider = 1;
    }
}
#endif