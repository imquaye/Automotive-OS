#include <stdio.h>
#include "safety.h"

static int fault_count = 0;  // basically checks how many faults have happned so far
static int in_safe_mode = 0; // checks if the systnem is in safe mode or not

// helps track the number of faults for us.
void safety_check(int fault) {
    if (fault) {
        fault_count++;
    } else {
        if (fault_count > 0) {
            fault_count--;
        }
    }
}

void activate_safe_mode() {
    in_safe_mode = 1;
}

void deactivate_safe_mode() {
    in_safe_mode = 0;
    fault_count = 0;
}

int is_in_safe_mode() {
    return in_safe_mode;
}

int get_fault_count() {
    return fault_count;
}
