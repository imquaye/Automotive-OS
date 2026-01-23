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
                    │  DEGRADED MODE  │
                    │ (fault_count<3) │
                    └────────┬────────┘
                             │
          fault_count >= 3   │
                             ▼
                    ┌─────────────────┐
                    │   SAFE MODE     │
                    │ (in_safe_mode=1)│
                    └────────┬────────┘
                             │
            fault_count = 0  │ (recovery)
                             ▼
                    ┌─────────────────┐
                    │  NORMAL MODE    │
                    └─────────────────┘
```

#### Fault Counting Logic

```c
#define FAULT_THRESHOLD 3

void safety_check(int fault) {
    if (fault) {
        fault_count++;  // Accumulate faults
        
        if (fault_count >= FAULT_THRESHOLD && !in_safe_mode) {
            // Too many faults - enter safe mode
            in_safe_mode = 1;
        }
    } else {
        if (fault_count > 0) {
            fault_count--;  // Gradual recovery on successful cycles
        }
        // Check if we can exit safe mode
        if (in_safe_mode && fault_count == 0) {
            in_safe_mode = 0;  // Recovery complete
        }
    }
}
```

**Key Safety Concepts:**
- **Fault Tolerance**: Single faults don't crash the system
- **Gradual Degradation**: System degrades gracefully, not abruptly
- **Safe Mode**: When critically faulted, disable non-essential functions
- **Recovery**: System can return to normal after sustained fault-free operation

---

### 4. Main Application (`main.c`)

Brings everything together with concrete automotive tasks.

#### Task Definitions with Priority Assignment

```c
// Higher priority (lower numbers) = more critical = shorter period
Task brake         = {"Brake Task",          10,  1,  10, 0, brake_task};
Task engine        = {"Engine Task",         20,  2,  20, 0, engine_task};
Task sensor        = {"Sensor Fusion Task",  30,  3,  30, 0, sensor_fusion_task};
Task infotainment  = {"Infotainment Task",  100,  4, 100, 0, infotainment_task};
```

**Priority Rationale:**
| Task | Period | Why This Priority |
|------|--------|-------------------|
| Brake | 10ms | Life-critical: must respond immediately to driver input |
| Engine | 20ms | Safety-critical: engine parameters change rapidly |
| Sensor | 30ms | ADAS-critical: object detection needs quick response |
| Infotainment | 100ms | Non-critical: audio/display can tolerate delays |

#### Brake Task Implementation

```c
void brake_task() {
    printf("Brake Control: Checking brake pressure\n");
    send_can_message("Brake ECU", "Brake OK");
}
```

**Real-world equivalent:**
1. Read brake pedal position sensor
2. Read wheel speed sensors
3. Calculate required brake force
4. Send commands to brake actuators via CAN
5. Report status to main vehicle controller

#### Sensor Fusion Task - Obstacle Detection

```c
void sensor_fusion_task() {
    float distance = ((rand() % 500) / 100.0); // Simulate 0.0m to 5.0m

    if (distance < SAFE_DISTANCE) {  // SAFE_DISTANCE = 1.0m
        printf("Sensor Fusion: Obstacle detected!\n");
        unsafe_count++;
        
        if (unsafe_count >= 2) {  // 2 consecutive unsafe readings
            enter_safe_mode();
        }
    } else {
        unsafe_count = 0;  // Reset on safe reading
    }
}
```

**Why consecutive readings matter:**
- Single bad reading could be sensor noise
- Two consecutive readings suggest real obstacle
- This is a simple **filtering algorithm** to reduce false positives

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
```

### Safety State Transitions

```
Normal Operation:
   └─→ safety_check(0) called
         └─→ "System operating normally"

Fault Detected (deadline miss or obstacle):
   └─→ safety_check(1) called
         └─→ fault_count++
               ├─→ If < 3: Continue with warning
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

### Run

```bash
./automotive_os
```

**To stop**: Press `Ctrl+C` (sends SIGINT)

---

## Sample Output

```
[System] Automotive OS Starting...
[System] Safety monitoring enabled
[System] Deadline monitoring enabled

[Scheduler] Running tasks (Rate Monotonic Scheduling)
[Scheduler] Executing Brake Task (deadline: 10ms)
Brake Control: Checking brake pressure
[CAN BUS] Brake ECU sent: Brake OK
[Scheduler] Brake Task completed in 0ms (within deadline)
[Scheduler] Executing Engine Task (deadline: 20ms)
Engine Control: Monitoring engine temperature
[CAN BUS] Engine ECU sent: Engine Normal
[Scheduler] Engine Task completed in 0ms (within deadline)
[Scheduler] Executing Sensor Fusion Task (deadline: 30ms)
Sensor Fusion: Distance = 0.83m
Sensor Fusion: Obstacle detected! initiating instant brake!
[Scheduler] Sensor Fusion Task completed in 0ms (within deadline)
[Scheduler] Executing Infotainment Task (deadline: 100ms)
Infotainment: Playing music
[Scheduler] Infotainment Task completed in 0ms (within deadline)
[SAFETY] System operating normally
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
