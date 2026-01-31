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
    
    // 8% chance of simulating a brake fault scenario (roughly 1 in 12 times)
    if (rand() % 100 < 8) {
        brake_pressure = rand() % 20;  // Force low pressure fault (0-19 PSI)
    }
    
    printf("Brake Control: Checking brake pressure = %d PSI\n", brake_pressure);
    
    if (brake_pressure < BRAKE_PRESSURE_MIN) {
        printf("[BRAKE WARNING] Low brake pressure detected! Pressure: %d PSI\n", brake_pressure);
        send_can_message("Brake ECU", "BRAKE FAULT - LOW PRESSURE");
        safety_check(1);  
        if (!is_in_safe_mode()) {
            printf("[BRAKE CRITICAL] Brake system failure - initiating SAFE MODE!\n");
            enter_safe_mode();
        }
    } else if (brake_pressure > BRAKE_PRESSURE_MAX) {
        printf("[BRAKE WARNING] High brake pressure detected! Pressure: %d PSI\n", brake_pressure);
        send_can_message("Brake ECU", "BRAKE FAULT - HIGH PRESSURE");
        safety_check(1);  
        if (!is_in_safe_mode()) {
            printf("[BRAKE CRITICAL] Brake system failure - initiating SAFE MODE!\n");
            enter_safe_mode();
        }
    } else {
        send_can_message("Brake ECU", "Brake OK");
        safety_check(0);  
    }
}

void engine_task() {

    int engine_temp = 70 + (rand() % 40);  
    
    
    if (rand() % 100 < 12) {
        engine_temp = 110 + (rand() % 25);  
    }
    
    printf("Engine Control: Monitoring engine temperature = %d°C\n", engine_temp);
    
    if (engine_temp > ENGINE_TEMP_MAX) {
        printf("[ENGINE WARNING] Engine overheating! Temperature: %d°C\n", engine_temp);
        send_can_message("Engine ECU", "ENGINE FAULT - OVERHEATING");
        safety_check(1);  
        if (!is_in_safe_mode()) {
            printf("[ENGINE CRITICAL] Engine overheating - initiating SAFE MODE!\n");
            enter_safe_mode();
        }
    } else if (engine_temp < ENGINE_TEMP_MIN) {
        printf("[ENGINE WARNING] Engine too cold! Temperature: %d°C\n", engine_temp);
        send_can_message("Engine ECU", "ENGINE WARNING - COLD START");
        safety_check(0);
    } else {
        send_can_message("Engine ECU", "Engine Normal");
        safety_check(0);  // System normal
    }
}

void enter_safe_mode() {
    printf("\n========================================\n");
    printf("       SAFE MODE ACTIVATED\n");
    printf("========================================\n");
    printf("Actions taken:\n");
    printf("  - Reducing vehicle speed to safe limit\n");
    printf("  - Disabling non-critical systems\n");
    printf("  - Activating hazard lights\n");
    printf("  - Alerting driver to pull over safely\n");
    printf("========================================\n\n");
    send_can_message("Safety ECU", "SAFE MODE ENGAGED");
}

void sensor_fusion_task() {
    // Optional: make distance dynamic for testing
    float distance = ((rand() % 500) / 100.0);

    printf("Sensor Fusion: Distance = %.2fm\n", distance);

    if(distance < SAFE_DISTANCE) {
        printf("[COLLISION WARNING] Obstacle detected at %.2fm! Initiating emergency brake!\n", distance);
        unsafe_count++;
        safety_check(1); 
        if(unsafe_count >= 2 && !is_in_safe_mode()) { 
            printf("[SENSOR CRITICAL] Multiple collision warnings - initiating SAFE MODE!\n");
            enter_safe_mode();
        }
    } else {
        unsafe_count = 0; 
    }
}

void infotainment_task() {
    printf("Infotainment: Playing music\n");
}

int main() {

    srand(time(NULL));
    

    Task brake = {"Brake Task", 10, 1, 5, 0, brake_task};                   
    Task engine = {"Engine Task", 20, 2, 15, 0, engine_task};               
    Task sensor = {"Sensor Fusion Task", 30, 3, 25, 0, sensor_fusion_task}; 
    Task infotainment = {"Infotainment Task", 100, 4, 200, 0, infotainment_task}; 

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
