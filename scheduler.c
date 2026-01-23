#include <stdio.h>
#include <time.h>
#include "scheduler.h"
#include "safety.h"

#define MAX_TASKS 10

Task tasks[MAX_TASKS];
int task_count = 0;
static int total_deadline_misses = 0;

void add_task(Task task) {
    task.deadline_missed = 0;  // Initialize deadline miss flag
    tasks[task_count++] = task;
}

void sort_tasks() {
    for (int i = 0; i < task_count - 1; i++) {
        for (int j = i + 1; j < task_count; j++) {
            if (tasks[i].period > tasks[j].period) {
                Task temp = tasks[i];
                tasks[i] = tasks[j];
                tasks[j] = temp;
            }
        }
    }
}

// Get elapsed time in milliseconds
long get_elapsed_ms(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000 + 
           (end.tv_nsec - start.tv_nsec) / 1000000;
}

int get_total_deadline_misses() {
    return total_deadline_misses;
}

void run_scheduler() {
    sort_tasks();
    printf("\n[Scheduler] Running tasks (Rate Monotonic Scheduling)\n");

    for (int i = 0; i < task_count; i++) {
        struct timespec start_time, end_time;
        
        // Record start time
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        
        printf("[Scheduler] Executing %s (deadline: %dms)\n", 
               tasks[i].name, tasks[i].deadline);
        
        // Execute the task
        tasks[i].task_function();
        
        // Record end time and check deadline
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        long execution_time = get_elapsed_ms(start_time, end_time);
        
        if (execution_time > tasks[i].deadline) {
            tasks[i].deadline_missed = 1;
            total_deadline_misses++;
            printf("[Scheduler] WARNING: %s missed deadline! (took %ldms, deadline was %dms)\n",
                   tasks[i].name, execution_time, tasks[i].deadline);
            
            // Trigger safety check on deadline miss
            safety_check(1);  // Report fault condition
        } else {
            tasks[i].deadline_missed = 0;
            printf("[Scheduler] %s completed in %ldms (within deadline)\n",
                   tasks[i].name, execution_time);
        }
    }
    
    // Periodic safety status check
    if (total_deadline_misses == 0) {
        safety_check(0);  // System operating normally
    } else {
        printf("[Scheduler] Total deadline misses this session: %d\n", total_deadline_misses);
    }
}


