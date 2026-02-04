#include <stdio.h>
#include <unistd.h>
#include "scheduler.h"
#include "can.h"
#include "safety.h"
#include <stdlib.h>
#include <time.h>

#define SAFE_DISTANCE 1.0

// tracking Consecutive failure/success 
static int brake_consecutive_failures = 0;
static int engine_consecutive_failures = 0;
static int sensor_consecutive_failures = 0;
static int consecutive_successes = 0;

// Thresholds for brake and engine systems
#define BRAKE_PRESSURE_MIN 20    
#define BRAKE_PRESSURE_MAX 120   
#define ENGINE_TEMP_MAX 105      
#define ENGINE_TEMP_MIN 70      

// Forward declarations
void enter_safe_mode();
void exit_safe_mode();
void check_safe_mode_recovery();



// Tasks
void brake_task() {
    
    int brake_pressure = rand() % 151;  // 0 to 150 PSI
    
   
    if (rand() % 100 < 8) {
        brake_pressure = rand() % 20; 
    }
    
    printf("Brake Control: Checking brake pressure = %d PSI\n", brake_pressure);
    
    if (brake_pressure < BRAKE_PRESSURE_MIN) {
        printf("[BRAKE WARNING] Low brake pressure detected! Pressure: %d PSI\n", brake_pressure);
        brake_consecutive_failures++;
        consecutive_successes = 0;  
        safety_check(1);  
        
        if (is_in_safe_mode()) {
            printf("[SAFE MODE] Driver alert: Brake system requires attention!\n");
        } else {
            printf("[BRAKE] Consecutive failures: %d/2\n", brake_consecutive_failures);
            if (brake_consecutive_failures >= 2) {
                printf("[BRAKE CRITICAL] 2 consecutive brake failures - initiating SAFE MODE!\n");
                enter_safe_mode();
            }
        }
        send_can_message("Brake ECU", "BRAKE FAULT - LOW PRESSURE");
    } else if (brake_pressure > BRAKE_PRESSURE_MAX) {
        printf("[BRAKE WARNING] High brake pressure detected! Pressure: %d PSI\n", brake_pressure);
        brake_consecutive_failures++;
        consecutive_successes = 0;  
        safety_check(1);  
        
        if (is_in_safe_mode()) {
            printf("[SAFE MODE] Driver alert: Brake system requires attention!\n");
        } else {
            printf("[BRAKE] Consecutive failures: %d/2\n", brake_consecutive_failures);
            if (brake_consecutive_failures >= 2) {
                printf("[BRAKE CRITICAL] 2 consecutive brake failures - initiating SAFE MODE!\n");
                enter_safe_mode();
            }
        }
        send_can_message("Brake ECU", "BRAKE FAULT - HIGH PRESSURE");
    } else {
        send_can_message("Brake ECU", "Brake OK");
        brake_consecutive_failures = 0;  
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
        engine_consecutive_failures++;
        consecutive_successes = 0; 
        safety_check(1);  
        
        if (is_in_safe_mode()) {
            printf("[SAFE MODE] Driver alert: Engine system requires attention!\n");
        } else {
            printf("[ENGINE] Consecutive failures: %d/2\n", engine_consecutive_failures);
            if (engine_consecutive_failures >= 2) {
                printf("[ENGINE CRITICAL] 2 consecutive engine failures - initiating SAFE MODE!\n");
                enter_safe_mode();
            }
        }
        send_can_message("Engine ECU", "ENGINE FAULT - OVERHEATING");
    } else if (engine_temp < ENGINE_TEMP_MIN) {
        printf("[ENGINE WARNING] Engine too cold! Temperature: %d°C\n", engine_temp);
        send_can_message("Engine ECU", "ENGINE WARNING - COLD START");
        engine_consecutive_failures = 0; 
        safety_check(0);
    } else {
        send_can_message("Engine ECU", "Engine Normal");
        engine_consecutive_failures = 0; 
        safety_check(0); 
    }
}

void enter_safe_mode() {
    activate_safe_mode();
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

void exit_safe_mode() {
    printf("\n========================================\n");
    printf("       SAFE MODE DEACTIVATED\n");
    printf("========================================\n");
    printf("All systems stable and operational\n");
    printf("Resuming normal operation\n");
    printf("========================================\n\n");
    send_can_message("Safety ECU", "SAFE MODE DISENGAGED");
    deactivate_safe_mode();
}

void check_safe_mode_recovery() {
    if (is_in_safe_mode()) {
        // Check if all systems are currently healthy (no consecutive failures)
        if (brake_consecutive_failures == 0 && 
            engine_consecutive_failures == 0 && 
            sensor_consecutive_failures == 0) {
            consecutive_successes++;
            printf("[RECOVERY] All systems healthy - consecutive successes: %d/3\n", consecutive_successes);
            
            if (consecutive_successes >= 3) {
                exit_safe_mode();
                consecutive_successes = 0;
            }
        } else {
            consecutive_successes = 0;  // Reset if any system has failures
        }
    }
}

void sensor_fusion_task() {
    // Optional: make distance dynamic for testing
    float distance = ((rand() % 500) / 100.0);

    printf("Sensor Fusion: Distance = %.2fm\n", distance);

    if(distance < SAFE_DISTANCE) {
        printf("[COLLISION WARNING] Obstacle detected at %.2fm! Activating warning lights and horns!\n", distance);
        sensor_consecutive_failures++;
        consecutive_successes = 0;  // Reset ze success counta
        safety_check(1); 
        
        if (is_in_safe_mode()) {
            printf("[SAFE MODE] Driver alert: Collision avoidance system active!\n");
        } else {
            printf("[SENSOR] Consecutive failures: %d/2\n", sensor_consecutive_failures);
            if(sensor_consecutive_failures >= 2) { 
                printf("[SENSOR CRITICAL] 2 consecutive collision warnings - initiating SAFE MODE!\n");
                enter_safe_mode();
            }
        }
    } else {
        sensor_consecutive_failures = 0;  // Reset failure counter
    }
}

void infotainment_task() {
    printf("Infotainment: Playing music\n");
}

int main() {

    srand(time(NULL));
    
// taasks definition
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
        check_safe_mode_recovery();
        sleep(2);
    }

    return 0;
}
