#include <stdio.h>
#include "safety.h"

static int fault_count = 0;
static int in_safe_mode = 0;

#define FAULT_THRESHOLD 3  // Number of faults before entering safe mode

void safety_check(int fault) {
    if (fault) {
        fault_count++;
        printf("[SAFETY] Fault detected! (fault count: %d/%d)\n", 
               fault_count, FAULT_THRESHOLD);
        
        if (fault_count >= FAULT_THRESHOLD && !in_safe_mode) {
            printf("[SAFETY] CRITICAL: Multiple faults detected!\n");
            printf("[SAFETY] Entering SAFE MODE - non-critical tasks suspended\n");
            in_safe_mode = 1;
        }
    } else {
        if (fault_count > 0) {
            fault_count--;  // Gradually reduce fault count on successful cycles
        }
        if (!in_safe_mode) {
            printf("[SAFETY] System operating normally\n");
        } else {
            printf("[SAFETY] System in SAFE MODE - monitoring for recovery\n");
            if (fault_count == 0) {
                printf("[SAFETY] System recovered - exiting SAFE MODE\n");
                in_safe_mode = 0;
            }
        }
    }
}

int is_in_safe_mode() {
    return in_safe_mode;
}

int get_fault_count() {
    return fault_count;
}
