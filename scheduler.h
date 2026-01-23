#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <time.h>

typedef struct {
    char name[30];
    int period;           // Task period in ms
    int priority;
    int deadline;         // Deadline in ms (typically equal to period)
    int deadline_missed;  // Flag to track if deadline was missed
    void (*task_function)();
} Task;

void add_task(Task task);
void run_scheduler();
int get_total_deadline_misses();

#endif
