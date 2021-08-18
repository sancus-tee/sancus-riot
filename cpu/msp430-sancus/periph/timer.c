/*
 * Copyright (C) 2015 Freie Universität Berlin
 * Modified for Sancus by KU Leuven.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_msp430fxyz
 * @ingroup     drivers_periph_timer
 * @{
 *
 * @file
 * @brief       Low-level timer driver implementation
 *
 * This implementation does only support one fixed timer, as defined in the
 * boards periph_conf.h file.
 *
 * @todo        Generalize to handle more timers and make them configurable
 *              through the board's `periph_conf.h`
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * 
 * Sancus modifications:
 * @author      Fritz Alder <fritz.alder@cs.kuleuven.be>
 *
 * @}
 */

#include "cpu.h"
#include "periph_cpu.h"
#include "periph_conf.h"
#include "periph/timer.h"
#include "log.h"
#include "time.h"
#include "sancus_modules.h"
#include "sancus_helpers.h"
#include "msp430_regs.h"

/**
 * @brief   Save reference to the timer callback
 */
static SM_DATA(sancus_sm_timer) timer_cb_t isr_cb;

/**
 * @brief Isr stack for the scheduler sm
 */
SM_DATA(sancus_sm_timer) char __timer_isr_stack[ISR_STACKSIZE];

/**
 * @brief    Save argument for the ISR callback
 * For sancus we disable this arg feature and do not support it.
 */
// static void *isr_arg;

/**
 * Declare Sancus Modules: 
 * - sancus_sm_timer is its driver module that enables access to TimerA and calls into the mmio module
 * -- This is a pure ASM module without its own stack
 * -- Only allows access originating from sancus_sm_timer
 * */
DECLARE_SM(sancus_sm_timer, SANCUS_RIOT_ID);


int SM_FUNC(sancus_sm_timer) sm_timer_init(tim_t dev, unsigned long freq, timer_cb_t cb){
    /* using fixed TIMER_BASE for now */
    if (dev != 0) {
        return -1;
    }
    /* riot todo: configure time-base depending on freq value */
    if (freq != 1000000ul) {
        return -1;
    }
    /* save callback */
    isr_cb = cb;

    // init mmio timer
    // mmio_timer_init(TIMER_CTL_TASSEL_SMCLK | TIMERA_CLOCK_DIVIDER | TIMER_CTL_MC_CONT | TIMER_CTL_IE);
    // TIMER_BASE->CTL = TIMER_CTL_CLR;
    // for (int i = 0; i < TIMER_CHAN; i++)
    //         TIMER_BASE->CCTL[i] = 0;
    // TIMER_BASE->CTL |= TIMER_CTL_TASSEL_SMCLK | TIMERA_CLOCK_DIVIDER | TIMER_CTL_MC_CONT | TIMER_CTL_IE; 

    /* configure timer to use the SMCLK with prescaler of 8 (or 0 depending on simulator usage) */
    TIMER_BASE->CTL = (TIMER_CTL_TASSEL_SMCLK | TIMERA_CLOCK_DIVIDER);
    /* configure CC channels */
    for (int i = 0; i < TIMER_CHAN; i++) {
        TIMER_BASE->CCTL[i] = 0;
    }
    /* start the timer in continuous mode */
    TIMER_BASE->CTL |= TIMER_CTL_MC_CONT;

    return 0;
}

int timer_init( __attribute__((unused)) tim_t dev,  __attribute__((unused)) unsigned long freq, __attribute__((unused)) timer_cb_t cb)
{
    LOG_ERROR("[msp430-sancus] timer_init: This function can bot be called from outside the scheduler!!!\n");

    // return sm_timer_init(cb);
    return -1;

}

void inline SM_FUNC(sancus_sm_timer) sm_timer_set_absolute(int channel, unsigned int value){
    // mmio_timer_set_absolute(channel, value);
    TIMER_BASE->CCR[channel] = value;
    TIMER_BASE->CCTL[channel] &= ~(TIMER_CCTL_CCIFG);
    TIMER_BASE->CCTL[channel] |=  (TIMER_CCTL_CCIE);
}

int timer_set_absolute( __attribute__((unused)) tim_t dev,  __attribute__((unused)) int channel,  __attribute__((unused)) unsigned int value)
{
    // if (dev != 0 || channel >= TIMER_CHAN) {
    //     return -1;
    // }
    
    // sm_timer_set_absolute(channel, value);

    // return 0;
    LOG_ERROR("Timer set absolute: This function can not be called from outside the Scheduler!");
    return -1;
}

uint16_t SM_FUNC(sancus_sm_timer) sm_timer_get_taiv(){
    return TIMER_IVEC->TAIV;
}

/**
 * Clear IE bit from given channel.
 * */
void SM_FUNC(sancus_sm_timer) mmio_timer_clear(
            USED_IN_ASM int channel){    // Register 15, ignored due to only channel in Sancus
    asm(
    // TIMER_BASE->CCTL[channel] &= ~(TIMER_CCTL_CCIE);
        // get CCTL
        "mov.w %0, r15         \n\t"

        // clear CCIE
        "bic %1, r15           \n\t"
        
        // write back
        "mov.w r15, %0         \n\t"

    :   "=m"(TIMER_A->CCTL)   // Timer A channel controls
    :   
        "i"(TIMER_CCTL_CCIE)
    :   // no clobbers
    );
}

void SM_FUNC(sancus_sm_timer) sm_timer_clear(int channel){
    mmio_timer_clear(channel);
}

int timer_clear(__attribute__((unused)) tim_t dev, __attribute__((unused)) int channel)
{
    // if (dev != 0 || channel >= TIMER_CHAN) {
    //     return -1;
    // }

    // sm_timer_clear(channel);

    // return 0;

    LOG_ERROR("Timer clear: This function can not be called from outside the Scheduler!");
    return -1;
}

/*
    If this function does not define an input argument,
    it somehow gives the following llvm error:
        Called function is not the same type as the call!
        %call = call i16 @sm_timer_read_internal()
        LLVM ERROR: Broken function found, compilation aborted!
*/
unsigned inline int SM_FUNC(sancus_sm_timer) sm_timer_read_internal(tim_t dev){
    (void)dev;
    return TIMER_A->R;
}

unsigned int SM_ENTRY(sancus_sm_timer) sm_timer_read(tim_t dev){
    (void)dev;
    return TIMER_A->R;
}

unsigned int timer_read(tim_t dev)
{
    (void)dev;
    return sm_timer_read(0);
}

void SM_FUNC(sancus_sm_timer) mmio_timer_start(){
    asm(
    // TIMER_BASE->CTL |= TIMER_CTL_MC_CONT;
        "bis %0, %1      \n\t"  
    :   // No output
    :   
        "i"(TIMER_CTL_MC_CONT),
        "m"(TIMER_A->CTL)
    :   // no clobbers
    );
}

void SM_FUNC(sancus_sm_timer) sm_timer_start(){
    mmio_timer_start();
}

void timer_start(tim_t dev)
{
    // sm_timer_start();
    (void)dev;
    LOG_ERROR("Timer start: Not allowed from outside the scheduler!\n");
    return;
}

void SM_FUNC(sancus_sm_timer) mmio_timer_stop(){
    asm(
    // TIMER_BASE->CTL &= ~(TIMER_CTL_MC_MASK);
        "bic %0, %1      \n\t"  
    :   // No output
    :   
        "i"(TIMER_CTL_MC_MASK),
        "m"(TIMER_A->CTL)
    :   // no clobbers
    );
}

void SM_FUNC(sancus_sm_timer) sm_timer_stop(){
    mmio_timer_stop();
}

void timer_stop(tim_t dev)
{
    // sm_timer_stop();
    (void)dev;
    LOG_ERROR("Timer stop: Not allowed from outside the scheduler!\n");
    return;
}

void SM_FUNC(sancus_sm_timer) isr_error(){
    // On error, just print an error message and exit
    // This would of course be handled differently in deployment code.
    sancus_error("Error during ISR detected. Aborting execution.");
    // CPU off
    __asm__("bis %0,r2" : : "i"(CPUOFF));
}

void SM_FUNC(sancus_sm_timer) __attribute__((naked, used)) __sm_sancus_sm_timer_isr_func(unsigned __attribute__ ((unused)) num_name)
{
    // check whether this is a violation 
    __asm__("mov r15, &__sm_sancus_sm_timer_tmp");
    __asm__(".word 0x1387");
    __asm__("cmp #0xFFFD, r15");
    __asm__("mov &__sm_sancus_sm_timer_tmp, r15");
    __asm__("jne 1f"); // not a violation, jump to enter ISR
        // If we are a violation, kill the current thread -- treat this as a context_switch_context_exit call
        __asm__ volatile ("mov.w %0,r1" : : "i"(__isr_stack + ISR_STACKSIZE)); // set up sp
        __asm__("call %0" : : "i"(sched_task_exit_internal));
        
        __asm__("mov %0, r15" : : "m"(TIMER_BASE->CTL));
        __asm__("bit %0, r15" : : "i"(TIMER_CTL_IFG));
        __asm__("jz 2f");       
            // timer overflowed irrespective of the violation. Handle this now instead of returning
            // switch to IRQ mode
            ___MACRO_ENTER_ISR_NO_CONTEXT_STORE
            __asm__("jmp after_context_save");

        __asm__("2:");
        __asm__("mov #1, r15"); // mark yield as switch now
        __asm__("br %0": : "i"(thread_yield_higher_internal));


    __asm__("1:");
    __MACRO_ENTER_ISR
    __asm__("after_context_save:"); // possible jump label for IRQ handling inside violations above

    // TIMER_BASE->CCTL[0] &= ~(TIMER_CCTL_CCIE);
    __asm__ volatile ("mov %0, r12" : : "m"(TIMER_BASE->CCTL[0]));
    __asm__ volatile ("and %0, r12" : : "i"(~(TIMER_CCTL_CCIE)));
    __asm__ volatile ("mov r12, %0" : "=m"(TIMER_BASE->CCTL[0]));
    // TODO: For more sophisticated interrupt schemes, we should check whith IRQ called us.
    // for now we just assume CCR0 overflow here.
    // Check interrupt source and abort if it is not a timer A overflow (for now)
    // __asm__("call %0" : : "i"(sm_timer_get_taiv));
    // __asm__("cmp #10, r15");
    // __asm__("jeq 1f");
    // for now abort for any other interrupt but later on we can multiplex either here or in the ISR entry stub
    // __asm__("call %0" : : "i"(isr_error));

    __asm__("1:");
    // isr_cb(isr_arg, 0);
    // __asm__ volatile ("mov &%0, r15" : : "m"(isr_arg));
    __asm__ volatile ("clr r15");
    __asm__ volatile ("clr r14");
    // __asm__ volatile ("mov &%0, r12" : : "m"(isr_cb));
    __asm__("call &%0" : : "m"(isr_cb));

    // __exit_isr();
    __MACRO_EXIT_ISR
    // __asm__ volatile ("mov &%0, r12" : : "m"(sched_context_switch_request)); 
    // __asm__ volatile ("tst r12");                                           
    // __asm__ volatile ("jz $+6");                                            
    // __asm__ volatile ("call %0" : : "i"(sched_run_internal));               

    // __asm__ volatile ("mov #0, &%0"  : "=m"(__sm_irq_is_in));               

    // ___MACRO_RESTORE_CONTEXT
}

SM_HANDLE_IRQ(sancus_sm_timer, 9);
SM_HANDLE_IRQ(sancus_sm_timer, 8);
SM_HANDLE_IRQ(sancus_sm_timer, 13);