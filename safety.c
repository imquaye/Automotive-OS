#include <stdio.h>
#include "safety.h"

static int fault_count = 0;
static int in_safe_mode = 0;
static int recovery_cycles = 0;

#define FAULT_THRESHOLD 3       // Number of faults before auto safe mode
#define RECOVERY_THRESHOLD 5    // Number of clean cycles needed to exit safe mode

void safety_check(int fault) {
    if (fault) {
        fault_count++;
        recovery_cycles = 0;  // Reset recovery counter on any fault
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
            recovery_cycles++;
            printf("[SAFETY] System in SAFE MODE - recovery progress: %d/%d clean cycles\n",
                   recovery_cycles, RECOVERY_THRESHOLD);
            if (fault_count == 0 && recovery_cycles >= RECOVERY_THRESHOLD) {
                printf("[SAFETY] System stable - exiting SAFE MODE\n");
                in_safe_mode = 0;
                recovery_cycles = 0;
            }
        }
    }
}

void activate_safe_mode() {
    if (!in_safe_mode) {
        in_safe_mode = 1;
        recovery_cycles = 0;
        printf("[SAFETY] SAFE MODE activated by critical system\n");
    }
}

int is_in_safe_mode() {
    return in_safe_mode;
}

int get_fault_count() {
    return fault_count;
}
