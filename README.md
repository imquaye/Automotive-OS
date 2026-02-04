# Automotive Operating System Simulation

A comprehensive simulation of a real-time operating system (RTOS) designed for automotive applications. This project demonstrates core concepts used in modern vehicle Electronic Control Units (ECUs) including task scheduling, inter-ECU communication, sensor processing, and functional safety mechanisms.

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Architecture](#architecture)
3. [File Structure](#file-structure)
4. [Detailed Component Breakdown](#detailed-component-breakdown)
   - [Scheduler Module](#1-scheduler-module-schedulerc-schedulerh)
   - [CAN Bus Module](#2-can-bus-module-canc-canh)
   - [Safety Module](#3-safety-module-safetyc-safetyh)
   - [Main Application](#4-main-application-mainc)
5. [How It Works - Step by Step](#how-it-works---step-by-step)
6. [Key Concepts Explained](#key-concepts-explained)
7. [Building and Running](#building-and-running)
8. [Sample Output](#sample-output)
9. [Future Enhancements](#future-enhancements)

---

## Project Overview

This simulation implements an **Automotive Operating System** that handles:

| Feature | Description |
|---------|-------------|
| **Real-Time Scheduling** | Rate Monotonic Scheduling (RMS) algorithm for deterministic task execution |
| **CAN Bus Communication** | Simulated Controller Area Network messaging between ECUs |
| **Safety Monitoring** | Fault detection, counting, and safe mode transitions |
| **Deadline Monitoring** | Tracks task execution times and detects deadline violations |
| **Sensor Fusion** | Processes distance sensor data for obstacle detection |
| **Realistic Fault Simulation** | Random brake pressure and engine temperature faults trigger safety protocols |
| **Infotainment** | Low-priority entertainment system simulation |

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        MAIN APPLICATION                         │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌─────────────────┐    │
│  │  Brake   │ │  Engine  │ │  Sensor  │ │  Infotainment   │    │
│  │   Task   │ │   Task   │ │  Fusion  │ │      Task       │    │
│  │ (10ms)   │ │ (20ms)   │ │  (30ms)  │ │    (100ms)      │    │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └───────┬─────────┘    │
│       │            │            │               │               │
│       └────────────┴─────┬──────┴───────────────┘               │
│                          │                                      │
│              ┌───────────▼───────────┐                          │
│              │      SCHEDULER        │                          │
│              │  (Rate Monotonic)     │                          │
│              │  + Deadline Monitor   │                          │
│              └───────────┬───────────┘                          │
│                          │                                      │
│       ┌──────────────────┼──────────────────┐                   │
│       │                  │                  │                   │
│       ▼                  ▼                  ▼                   │
│  ┌─────────┐      ┌───────────┐      ┌───────────┐             │
│  │ CAN BUS │      │  SAFETY   │      │   TIMER   │             │
│  │ Module  │      │  Module   │      │  (clock)  │             │
│  └─────────┘      └───────────┘      └───────────┘             │
└─────────────────────────────────────────────────────────────────┘
```

---

## File Structure

```
Automotive-OS/
├── main.c          # Application entry point and task definitions
├── scheduler.c     # Real-time scheduler implementation
├── scheduler.h     # Scheduler interface and Task structure
├── can.c           # CAN bus communication simulation
├── can.h           # CAN bus interface
├── safety.c        # Safety monitoring and fault management
├── safety.h        # Safety module interface
└── README.md       # This documentation
```

---

## Detailed Component Breakdown

### 1. Scheduler Module (`scheduler.c`, `scheduler.h`)

The scheduler is the **heart of the RTOS**, responsible for determining which task runs and when.

#### Task Structure Definition

```c
typedef struct {
    char name[30];           // Human-readable task name
    int period;              // How often the task should run (milliseconds)
    int priority;            // Task priority (lower number = higher priority)
    int deadline;            // Maximum allowed execution time (milliseconds)
    int deadline_missed;     // Flag: 1 if deadline was violated
    void (*task_function)(); // Pointer to the actual task code
} Task;
```

**Why these fields matter:**
- **period**: In RMS, shorter period = higher priority. Brake (10ms) runs before infotainment (100ms)
- **deadline**: Safety-critical systems must complete within predictable time bounds
- **task_function**: Function pointer allows flexible task registration

#### Rate Monotonic Scheduling (RMS) Algorithm

```c
void sort_tasks() {
    // Bubble sort tasks by period (ascending)
    // Shorter period = runs first = higher implicit priority
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
```

**RMS Principle**: Tasks with shorter periods (more frequent execution needs) receive higher priority. This is mathematically proven to be optimal for periodic task scheduling.

#### Deadline Monitoring

```c
// Before task execution
clock_gettime(CLOCK_MONOTONIC, &start_time);

// Execute task
tasks[i].task_function();

// After execution - check if deadline was met
clock_gettime(CLOCK_MONOTONIC, &end_time);
long execution_time = get_elapsed_ms(start_time, end_time);

if (execution_time > tasks[i].deadline) {
    // DEADLINE MISS! This is critical in automotive systems
    tasks[i].deadline_missed = 1;
    total_deadline_misses++;
    safety_check(1);  // Report fault to safety module
}
```

**Why deadline monitoring matters**: In a real car, if the brake task misses its deadline, the brakes might not respond in time. Deadline misses are treated as **safety faults**.

---

### 2. CAN Bus Module (`can.c`, `can.h`)

The **Controller Area Network (CAN)** is the standard communication protocol in vehicles, connecting all ECUs.

#### Message Transmission

```c
void send_can_message(char sender[], char message[]) {
    printf("[CAN BUS] %s sent: %s\n", sender, message);
}
```

**In a real system**, this would:
1. Package the message with a CAN ID (11 or 29 bits)
2. Add CRC for error detection
3. Transmit on the physical CAN bus (twisted pair wire)
4. Handle arbitration if multiple ECUs transmit simultaneously

**Example Usage**:
```c
send_can_message("Brake ECU", "Brake OK");
// Output: [CAN BUS] Brake ECU sent: Brake OK
```

---

### 3. Safety Module (`safety.c`, `safety.h`)

Implements **functional safety** concepts from ISO 26262 (automotive safety standard).

#### Fault Management State Machine

```
                    ┌─────────────────┐
                    │  NORMAL MODE    │
                    │ (fault_count=0) │
                    └────────┬────────┘
                             │
              fault detected │
                             ▼
                    ┌─────────────────┐
                    │  TRACKING FAULTS│
                    │ (fault_count>0) │
                    └────────┬────────┘
                             │
     2 consecutive failures  │
      in any subsystem       │
                             ▼
                    ┌─────────────────┐
                    │   SAFE MODE     │
                    │ (in_safe_mode=1)│
                    └────────┬────────┘
                             │
    3 consecutive successes  │ (recovery)
    across all subsystems    │
                             ▼
                    ┌─────────────────┐
                    │  NORMAL MODE    │
                    └─────────────────┘
```

#### Fault Counting Logic

```c
static int fault_count = 0;
static int in_safe_mode = 0;

void safety_check(int fault) {
    if (fault) {
        fault_count++;  // Accumulate faults
    } else {
        if (fault_count > 0) {
            fault_count--;  // Gradual recovery on successful cycles
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
```
 Consecutive Threshold |
|--------|-----------|-------------|-------------------|----------------------|
| **Brake** | Low Pressure | 8% | Pressure < 20 PSI | 2 consecutive |
| **Brake** | High Pressure | Random | Pressure > 120 PSI | 2 consecutive |
| **Engine** | Overheating | 12% | Temperature > 105°C | 2 consecutive |
| **Engine** | Cold Start | Normal | Temperature < 70°C | Warning only |
| **Sensor** | Collision Warning | Random | Distance < 1.0m | 2 consecutivecycles to exit safe mode
- **Fault Tracking**: Central fault counting provides additional system health monitoring

#### Realistic Fault Simulation

The system now simulates **realistic vehicle fault scenarios** to demonstrate safety protocols:

| System | Fault Type | Probability | Trigger Condition |
|--------|-----------|-------------|-------------------|
| **Brake** | Low Pressure | 8% | Pressure < 20 PSI |
| **Brake** | High Pressure | Random | Pressure > 120 PSI |
| **Engine** | Overheating | 8% | Temperature > 105°C |
| **Sensor** | Collision Warning | Random | Distance < 1.0m |

**Why Simulate Faults?**
- Real vehicles don't always operate perfectly
- Safety systemconsecutive fault detection logic
- Shows how subsystem failures trigger safe mode
- Recovery mechanism ensures system returns to normal only when stable

**Consecutive Failure Flow:**
```
Brake/Engine/Sensor Fault Detected (1st time)
         │
         ├─→ consecutive_failures = 1
         ├─→ consecutive_successes = 0 (reset)
         └─→ safety_check(1) called
                 │
                 └─→ fault_count++

Same Fault Detected Again (2nd consecutive)
         │
         ├─→ consecutive_failures = 2
         ├─→ TRIGGER SAFE MODE
         └─→ safety_check(1) called

All Systems Healthy (in safe mode)
         │
         ├─→ consecutive_successes++
         └─→ If consecutive_successes >= 3:
                 └─→ EXIT SAFE MODE
 SAFE MODE  Continue monitoring
```

---

### 4. Main Application (`main.c`)

Brings everything together with concrete automotive tasks.

#### Task Definitions with Priority Assignment

```c
// Higher priority (lower numbers) = more critical = shorter period
Task brake         = {"Brake Task",          10,  1,   5, 0, brake_task};
Task engine        = {"Engine Task",         20,  2,  15, 0, engine_task};
Task sensor        = {"Sensor Fusion Task",  30,  3,  25, 0, sensor_fusion_task};
Task infotainment  = {"Infotainment Task",  100,  4, 200, 0, infotainment_task};
```

**Priority Rationale:**
| Task | Period | Deadline | Why This Priority |
|------|--------|----------|-------------------|
| Brake | 10ms | 5ms | Life-critical: must respond immediately to driver input |
| Engine | 20ms | 15ms | Safety-critical: engine parameters change rapidly |
| Sensor | 30ms | 25ms | ADAS-critical: object detection needs quick response |
| Infotainment | 100ms | 200ms | Non-critical: audio/display can tolerate delays |

#### Brake Task Implementation

```c
#define BRAKE_PRESSURE_MIN 20    // Minimum safe brake pressure (PSI)
#define BRAKE_PRESSURE_MAX 120   // Maximum safe brake pressure (PSI)

static int brake_consecutive_failures = 0;
static int consecutive_successes = 0;

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
        brake_consecutive_failures++;
        consecutive_successes = 0;
        safety_check(1);  // Report fault to safety system
        
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
        safety_check(0);  // System normal
    }
}
```

**Consecutive Failure Logic:**
- Tracks consecutive brake failures using static counter
- 2 consecutive failures trigger immediate safe mode entry
- Reset on successful reading
- Separate tracking allows recovery when system stabilizes

**Real-world equivalent:**
1. Read brake pedal position sensor
2. Read wheel speed sensors
3. Calculate required brake force
4. Send commands to brake actuators via CAN
5. Report status to main vehicle controller

#### Engine Task Implementation

```c
#define ENGINE_TEMP_MAX 105      // Maximum safe engine temperature (Celsius)
#define ENGINE_TEMP_MIN 70       // Minimum operating temperature (Celsius)

static int engine_consecutive_failures = 0;

void engine_task() {
    // Simulate realistic engine temperature (70-110 Celsius, with occasional faults)
    int engine_temp = 70 + (rand() % 40);  // Normal range: 70-110 Celsius
    
    // 12% chance of simulating an engine fault scenario
    if (rand() % 100 < 12) {
        engine_temp = 110 + (rand() % 25);  // Force overheating (110-134 Celsius)
    }
    
    printf("Engine Control: Monitoring engine temperature = %d°C\n", engine_temp);
    
    if (engine_temp > ENGINE_TEMP_MAX) {
        printf("[ENGINE WARNING] Engine overheating! Temperature: %d°C\n", engine_temp);
        engine_consecutive_failures++;
        consecutive_successes = 0;
        safety_check(1);  // Report fault to safety system
        
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
        safety_check(0);  // System normal
    }
}
```

**Fault Simulation Logic:**
- Normal temperature range: 70-105°C
- 12% random chance of forced overheating fault
- Cold engine (<70°C) generates warning but not critical fault
- 2 consecutive overheating events trigger safe mode

---

#### Sensor Fusion Task - Obstacle Detection

```c
#define SAFE_DISTANCE 1.0

static int sensor_consecutive_failures = 0;

void sensor_fusion_task() {
    // Dynamic distance for testing
    float distance = ((rand() % 500) / 100.0); // Simulate 0.0m to 5.0m

    printf("Sensor Fusion: Distance = %.2fm\n", distance);

    if (distance < SAFE_DISTANCE) {  // SAFE_DISTANCE = 1.0m
        printf("[COLLISION WARNING] Obstacle detected at %.2fm! Initiating emergency brake!\n", distance);
        sensor_consecutive_failures++;
        consecutive_successes = 0;
        safety_check(1);  // Report fault to safety system
        
        if (is_in_safe_mode()) {
            printf("[SAFE MODE] Driver alert: Collision avoidance system active!\n");
        } else {
            printf("[SENSOR] Consecutive failures: %d/2\n", sensor_consecutive_failures);
            if (sensor_consecutive_failures >= 2) {
                printf("[SENSOR CRITICAL] 2 consecutive collision warnings - initiating SAFE MODE!\n");
                enter_safe_mode();
            }
        }
    } else {
        sensor_consecutive_failures = 0;  // Reset on safe reading
    }
}
```

**Why consecutive readings matter:**
- Single bad reading could be sensor noise
- Two consecutive readings suggest real obstacle
- This is a simple **filtering algorithm** to reduce false positives
- All three systems (brake, engine, sensor) use the same 2-failure threshold

#### Safe Mode Management

The system implements comprehensive safe mode entry and recovery:

```c
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

void check_safe_mode_recovery() {
    if (is_in_safe_mode()) {
        // All systems must be healthy (no consecutive failures)
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
```

**Recovery Requirements:**
- All three systems (brake, engine, sensor) must be fault-free
- 3 consecutive successful cycles required to exit safe mode
- Any failure during recovery resets the success counter
- Ensures system stability before returning to normal operation

---

## How It Works - Step by Step

### Program Startup

```
1. main() begins
   │
   ├─→ Create 4 Task structures with periods, priorities, deadlines
   │
   ├─→ Print startup messages
   │
   ├─→ add_task() × 4  →  Tasks stored in tasks[] array
   │
   └─→ Enter infinite loop
```

### Each Scheduler Cycle

```
1. run_scheduler() called
   │
   ├─→ sort_tasks()  →  Order by period (RMS)
   │       Result: [Brake, Engine, Sensor, Infotainment]
   │
   └─→ For each task:
           │
           ├─→ Record start time (clock_gettime)
           │
           ├─→ Execute task_function()
           │       ├─→ Task does its work
           │       └─→ May send CAN messages
           │
           ├─→ Record end time
           │
           ├─→ Calculate execution_time
           │
           ├─→ Compare with deadline
           │       ├─→ If exceeded: deadline_missed=1, call safety_check(1)
           │       └─→ If within: deadline_missed=0, print success
           │
           └─→ Move to next task

2. After all tasks:
   │
   ├─→ If no deadline misses: safety_check(0)
   │
   └─→ sleep(2) then repeat
```All tasks executing successfully
         ├─→ safety_check(0) called
         └─→ "System operating normally"

Fault Detected (single occurrence):
   └─→ Task detects fault (pressure/temperature/distance)
         ├─→ consecutive_failures = 1
         ├─→ safety_check(1) called
         ├─→ fault_count++
         └─→ Continue monitoring

Second Consecutive Fault (same subsystem):
   └─→ consecutive_failures = 2
         ├─→ enter_safe_mode() triggered
         ├─→ activate_safe_mode() called
         ├─→ CAN message broadcast
         └─→ in_safe_mode = 1

Recovery Process:
   └─→ All subsystems healthy (brake=0, engine=0, sensor=0)
         ├─→ consecutive_successes++
         ├─→ If consecutive_successes >= 3:
         │     ├─→ exit_safe_mode() triggered
         │     ├─→ deactivate_safe_mode() called
         │     └─→ in_safe_mode = 0
         └─→ Resume normal operationwarning
               └─→ If >= 3: Enter SAFE MODE

Recovery:
   └─→ Multiple safety_check(0) calls
         └─→ fault_count decrements
               └─→ When 0: Exit SAFE MODE
```

---

## Key Concepts Explained

### Rate Monotonic Scheduling (RMS)

**Definition**: A fixed-priority scheduling algorithm where tasks with shorter periods receive higher priorities.

**Mathematical Basis**: Liu & Layland (1973) proved that RMS is optimal for periodic tasks on single-processor systems when CPU utilization is below ~69%.

**Formula for schedulability**:
```
U = Σ(Ci/Ti) ≤ n(2^(1/n) - 1)

Where:
- U = CPU utilization
- Ci = Worst-case execution time of task i
- Ti = Period of task i
- n = Number of tasks
```

### Deadline Monitoring

**Purpose**: Ensure tasks complete within their specified time bounds.

**Implementation**: 
1. Record timestamp before task execution
2. Record timestamp after task execution
3. Compare elapsed time with deadline
4. Take corrective action if exceeded

### CAN Bus in Automotive

**Why CAN?**
- Robust: Differential signaling resists noise
- Multi-master: Any ECU can initiate communication
- Priority-based: Important messages have lower IDs (higher priority)
- Error detection: Built-in CRC and acknowledgment

### Functional Safety (ISO 26262)

**ASIL Levels** (Automotive Safety Integrity Level):
- **ASIL D**: Highest (steering, braking)
- **ASIL C**: High (airbags)
- **ASIL B**: Medium (headlights)
- **ASIL A**: Low (rear lights)
- **QM**: No safety requirements (infotainment)

Our simulation maps:
- Brake Task → ASIL D equivalent
- Engine Task → ASIL C equivalent
- Sensor Fusion → ASIL B equivalent
- Infotainment → QM equivalent

---

## Building and Running

### Prerequisites

- GCC compiler (any recent version)
- Linux/Unix environment (for `clock_gettime`)

### Compile

```bash
gcc *.c -o automotive_os
```

**What this does:**
1. Compiles `main.c`, `scheduler.c`, `can.c`, `safety.c`
2. Links them together
3. Creates executable `automotive_os`

### Run5ms)
Brake Control: Checking brake pressure = 85 PSI
[CAN BUS] Brake ECU sent: Brake OK
[Scheduler] Brake Task completed in 0ms (within deadline)
 
[Scheduler] Executing Engine Task (deadline: 15ms)
Engine Control: Monitoring engine temperature = 92°C
[CAN BUS] Engine ECU sent: Engine Normal
[Scheduler] Engine Task completed in 0ms (within deadline)

[Scheduler] Executing Sensor Fusion Task (deadline: 25ms)
Sensor Fusion: Distance = 3.45m
[Scheduler] Sensor Fusion Task completed in 0ms (within deadline)

[Scheduler] Executing Infotainment Task (deadline: 200ms)
Infotainment: Playing music
[Scheduler] Infotainment Task completed in 0ms (within deadline)

--- Later in execution (fault occurs) ---

[Scheduler] Executing Brake Task (deadline: 5ms)
Brake Control: Checking brake pressure = 12 PSI
[BRAKE WARNING] Low brake pressure detected! Pressure: 12 PSI
[BRAKE] Consecutive failures: 1/2
[CAN BUS] Brake ECU sent: BRAKE FAULT - LOW PRESSURE
[Scheduler] Brake Task completed in 0ms (within deadline)

--- Next cycle (second consecutive brake fault) ---

[Scheduler] Executing Brake Task (deadline: 5ms)
Brake Control: Checking brake pressure = 15 PSI
[BRAKE WARNING] Low brake pressure detected! Pressure: 15 PSI
[BRAKE] Consecutive failures: 2/2
[BRAKE CRITICAL] 2 consecutive brake failures - initiating SAFE MODE!

========================================
       SAFE MODE ACTIVATED
========================================
Actions taken:
  - Reducing vehicle speed to safe limit
  - Disabling non-critical systems
  - Activating hazard lights
  - Alerting driver to pull over safely
========================================

[CAN BUS] Safety ECU sent: SAFE MODE ENGAGED
[CAN BUS] Brake ECU sent: BRAKE FAULT - LOW PRESSURE
[Scheduler] Brake Task completed in 0ms (within deadline)

--- System continues in safe mode ---

[Scheduler] Executing Brake Task (deadline: 5ms)
Brake Control: Checking brake pressure = 78 PSI
[CAN BUS] Brake ECU sent: Brake OK
[SAFE MODE] Driver alert: Brake system requires attention!
[Scheduler] Brake Task completed in 0ms (within deadline)

--- Recovery begins (all systems healthy) ---

[RECOVERY] All systems healthy - consecutive successes: 1/3
[RECOVERY] All systems healthy - consecutive successes: 2/3
[RECOVERY] All systems healthy - consecutive successes: 3/3

========================================
       SAFE MODE DEACTIVATED
========================================
All systems stable and operational
Resuming normal operation
========================================

[CAN BUS] Safety ECU sent: SAFE MODE DISENGAGED
[Scheduler] Executing Brake Task (deadline: 10ms)
Brake Control: Checking brake pressure = 12 PSI
[BRAKE WARNING] Low brake pressure detected! Pressure: 12 PSI
[CAN BUS] Brake ECU sent: BRAKE FAULT - LOW PRESSURE
[SAFETY] Fault detected! Fault count: 1
[Scheduler] Brake Task completed in 0ms (within deadline)

[Scheduler] Executing Engine Task (deadline: 20ms)
Engine Control: Monitoring engine temperature = 118°C
[ENGINE WARNING] Engine overheating! Temperature: 118°C
[CAN BUS] Engine ECU sent: ENGINE FAULT - OVERHEATING
[SAFETY] Fault detected! Fault count: 2
[Scheduler] Engine Task completed in 0ms (within deadline)

--- Multiple faults trigger safe mode ---

[SAFETY] CRITICAL: Too many faults! Entering SAFE MODE!
[SAFETY] Non-critical systems disabled for driver protection
System is now in SAFE MODE.
```

---

## Future Enhancements

| Enhancement | Description |
|-------------|-------------|
| **Preemptive Scheduling** | Allow higher-priority tasks to interrupt lower-priority ones |
| **CAN Message Queue** | Implement realistic CAN message buffering and filtering |
| **Multiple Sensors** | Add radar, lidar, camera fusion for ADAS |
| **Watchdog Timer** | Hardware timer to detect system hangs |
| **Task Suspension** | Actually disable non-critical tasks in safe mode |
| **Logging System** | Persistent fault logging for diagnostics |
| **Real CAN Interface** | Use SocketCAN for actual CAN hardware |

---

## Authors

Group 47 - Automotive Operating System Project

---

## License

This project is for educational purposes demonstrating automotive OS concepts.
