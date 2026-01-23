#include <stdio.h>
#include <unistd.h>
#include "scheduler.h"
#include "can.h"
#include "safety.h"
#include <stdlib.h>

#define SAFE_DISTANCE 1.0
static int unsafe_count = 0;


/* TASKS */

void brake_task() {
    printf("Brake Control: Checking brake pressure\n");
    send_can_message("Brake ECU", "Brake OK");
}

void engine_task() {
    printf("Engine Control: Monitoring engine temperature\n");
    send_can_message("Engine ECU", "Engine Normal");
}

void enter_safe_mode() {
    // simulate safe mode actions
    printf("System is now in SAFE MODE.\n");
}

void sensor_fusion_task() {
    // Optional: make distance dynamic for testing
    float distance = ((rand() % 500) / 100.0); // 0.0m to 5.0m

    printf("Sensor Fusion: Distance = %.2fm\n", distance);

    if(distance < SAFE_DISTANCE) {
        printf("Sensor Fusion: Obstacle detected! initiating instant brake!\n");
        unsafe_count++;
        if(unsafe_count >= 2) {  // triggers after 2 consecutive unsafe readings
            printf("[SAFETY] Fault detected! Entering SAFE MODE\n");
            enter_safe_mode();  // your existing function
        }
    } else {
    unsafe_count = 0; // reset counter if safe
    }
}

void infotainment_task() {
    printf("Infotainment: Playing music\n");
}

int main() {
    // Task structure: {name, period(ms), priority, deadline(ms), deadline_missed, task_function}
    Task brake = {"Brake Task", 10, 1, 10, 0, brake_task};              // Critical: 10ms deadline
    Task engine = {"Engine Task", 20, 2, 20, 0, engine_task};           // High: 20ms deadline
    Task sensor = {"Sensor Fusion Task", 30, 3, 30, 0, sensor_fusion_task}; // Medium: 30ms deadline
    Task infotainment = {"Infotainment Task", 100, 4, 100, 0, infotainment_task}; // Low: 100ms deadline

    printf("[System] Automotive OS Starting...\n");
    printf("[System] Safety monitoring enabled\n");
    printf("[System] Deadline monitoring enabled\n\n");

    add_task(brake);
    add_task(engine);
    add_task(sensor);
    add_task(infotainment);

    while (1) {
        run_scheduler();
        sleep(2);
    }

    return 0;
}
