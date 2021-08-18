#include "sancus_helpers.h"

void riot_enable_sm(struct SancusModule* sm){
    int success_flag;
    do{success_flag = sancus_enable(sm);} while (success_flag == 0);
    
    // This is a custom pr_sm_info that allows colored LOG output
    LOG_INFO("SM %s with ID %d enabled\t: 0x%.4x 0x%.4x 0x%.4x 0x%.4x\n",
        sm->name, sm->id,
        (uintptr_t) sm->public_start, (uintptr_t) sm->public_end,
        (uintptr_t) sm->secret_start, (uintptr_t) sm->secret_end);
    
}

void print_thread_struct(kernel_pid_t pid){
    LOG_DEBUG("Thread with PID: %i :\n", pid);
    LOG_DEBUG("   - Is SM: %i\n", sched_threads[pid].is_sm);
    LOG_DEBUG("   - SP: %p\n", (void*) sched_threads[pid].sp);
    LOG_DEBUG("   - Status: %i\n", sched_threads[pid].status);
    LOG_DEBUG("   - Priority: %i\n", sched_threads[pid].priority);
    LOG_DEBUG("   - PID: %i\n", sched_threads[pid].pid);
    LOG_DEBUG("   - RQ entry: %p\n", (void*) &sched_threads[pid].rq_entry);
    LOG_DEBUG("   - In use: %i\n", sched_threads[pid].in_use);
    LOG_DEBUG("   - SM IDX: %i\n", sched_threads[pid].sm_idx);
    LOG_DEBUG("   - SM Entry: %p\n", (void *) sched_threads[pid].sm_entry);
}