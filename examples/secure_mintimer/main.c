#include <msp430.h>
#include <stdio.h>
#include "kernel_defines.h"
#include "secure_mintimer.h"
#include "log.h"
#include "sancus_helpers.h"

int main(void)
{
    LOG_INFO("######## Riot on Sancus\n");
    LOG_INFO("Simple secure_mintimer application based on Riot Xtimer\n");
    printf("Testing colored logs..");
    LOG_DEBUG("Debug ");
    LOG_INFO("Info ");
    LOG_WARNING("Warning ");
    LOG_ERROR("and ERROR\n");

    uint64_t curr1, curr2;
    for(int i=0; i<2; i++){
        curr1 = secure_mintimer_now_usec64() ;
        curr2 = secure_mintimer_now_usec64() ;
        LOG_WARNING("Current system time 1 and 2 are %llu and %llu\n", curr1, curr2);
    }

    secure_mintimer_usleep(1000);
    LOG_WARNING("1000 microseconds passed\n");
    LOG_WARNING("Will now sleep for 1 second. In simulator, this will take 1,000,000 cycles.\n");
    secure_mintimer_sleep(1);
    LOG_WARNING("1 seconds passed\n");

    // Sleeping until here takes around 1,700,000 cycles (you can check current cycle count
    //  in simulator via the --print-progress-at=100000 flag)

    LOG_WARNING("done\n");
    __asm__("bis %0,r2" : : "i"(CPUOFF)); // With restrictions on G2 this won't work.
    LOG_WARNING("CPUOFF did not work, exiting this thread then via scheduler.\n");
    sched_shut_down();
    // cpu_switch_context_exit(); // we could also just exit the main thread and keep running the idle thread.

    UNREACHABLE();
    return 0;
}
