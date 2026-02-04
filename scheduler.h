#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <time.h>

typedef struct {
    char name[30];
    int period;           
    int priority;
    int deadline;         
    int deadline_missed;  
    void (*task_function)();
} Task;

void add_task(Task task);
void run_scheduler();
int get_total_deadline_misses();

#endif
