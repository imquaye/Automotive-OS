#include <stdio.h>
#include <unistd.h>
#include "scheduler.h"
#include "can.h"
#include "safety.h"
#include <stdlib.h>
#include <time.h>

#define SAFE_DISTANCE 1.0
static int unsafe_count = 0;

// Thresholds for brake and engine systems
#define BRAKE_PRESSURE_MIN 20    // Minimum safe brake pressure (PSI)
#define BRAKE_PRESSURE_MAX 120   // Maximum safe brake pressure (PSI)
#define ENGINE_TEMP_MAX 105      // Maximum safe engine temperature (Celsius)
#define ENGINE_TEMP_MIN 70       // Minimum operating temperature (Celsius)

/* TASKS */

void brake_task() {
    // Simulate realistic brake pressure (0-150 PSI, with occasional faults)
    int brake_pressure = rand() % 151;  // 0 to 150 PSI
    
    // 15% chance of simulating a brake fault scenario
    if (rand() % 100 < 15) {
        brake_pressure = rand() % 20;  // Force low pressure fault (0-19 PSI)
    }
    
    printf("Brake Control: Checking brake pressure = %d PSI\n", brake_pressure);
    
    if (brake_pressure < BRAKE_PRESSURE_MIN) {
        printf("[BRAKE WARNING] Low brake pressure detected! Pressure: %d PSI\n", brake_pressure);
        send_can_message("Brake ECU", "BRAKE FAULT - LOW PRESSURE");
        safety_check(1);  // Report fault to safety system
    } else if (brake_pressure > BRAKE_PRESSURE_MAX) {
        printf("[BRAKE WARNING] High brake pressure detected! Pressure: %d PSI\n", brake_pressure);
        send_can_message("Brake ECU", "BRAKE FAULT - HIGH PRESSURE");
        safety_check(1);  // Report fault to safety system
    } else {
        send_can_message("Brake ECU", "Brake OK");
        safety_check(0);  // System normal
    }
}

void engine_task() {
    // Simulate realistic engine temperature (50-130 Celsius, with occasional faults)
    int engine_temp = 70 + (rand() % 40);  // Normal range: 70-110 Celsius
    
    // 12% chance of simulating an engine fault scenario
    if (rand() % 100 < 12) {
        engine_temp = 110 + (rand() % 25);  // Force overheating (110-134 Celsius)
    }
    
    printf("Engine Control: Monitoring engine temperature = %d°C\n", engine_temp);
    
    if (engine_temp > ENGINE_TEMP_MAX) {
        printf("[ENGINE WARNING] Engine overheating! Temperature: %d°C\n", engine_temp);
        send_can_message("Engine ECU", "ENGINE FAULT - OVERHEATING");
        safety_check(1);  // Report fault to safety system
    } else if (engine_temp < ENGINE_TEMP_MIN) {
        printf("[ENGINE WARNING] Engine too cold! Temperature: %d°C\n", engine_temp);
        send_can_message("Engine ECU", "ENGINE WARNING - COLD START");
        // Cold engine is a warning, not a critical fault
        safety_check(0);
    } else {
        send_can_message("Engine ECU", "Engine Normal");
        safety_check(0);  // System normal
    }
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
        printf("[COLLISION WARNING] Obstacle detected at %.2fm! Initiating emergency brake!\n", distance);
        unsafe_count++;
        safety_check(1);  // Report fault to safety system
        if(unsafe_count >= 2) {  // triggers after 2 consecutive unsafe readings
            printf("[SAFETY] Multiple collision warnings! Engaging SAFE MODE protocols\n");
            enter_safe_mode();
        }
    } else {
        unsafe_count = 0; // reset counter if safe
    }
}

void infotainment_task() {
    printf("Infotainment: Playing music\n");
}

int main() {
    // Seed random number generator for realistic simulation
    srand(time(NULL));
    
    // Task structure: {name, period(ms), priority, deadline(ms), deadline_missed, task_function}
    Task brake = {"Brake Task", 10, 1, 10, 0, brake_task};              // Critical: 10ms deadline
    Task engine = {"Engine Task", 20, 2, 20, 0, engine_task};           // High: 20ms deadline
    Task sensor = {"Sensor Fusion Task", 30, 3, 30, 0, sensor_fusion_task}; // Medium: 30ms deadline
    Task infotainment = {"Infotainment Task", 100, 4, 100, 0, infotainment_task}; // Low: 100ms deadline

    printf("[System] Automotive OS Starting...\n");
    printf("[System] Safety monitoring enabled\n");
    printf("[System] Deadline monitoring enabled\n");
    printf("[System] Fault simulation active - brake/engine faults may occur\n\n");

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
