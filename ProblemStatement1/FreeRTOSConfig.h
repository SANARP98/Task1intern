/**
 * ============================================================================
 * FreeRTOS Configuration File
 * Problem Statement 1 - LIPL Assessment
 * ============================================================================
 * 
 * This file configures FreeRTOS behavior and enables required features.
 * Adjust settings based on your target platform (simulator vs hardware).
 * ============================================================================
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ============================================================================
 * CRITICAL: Enable Required API Functions
 * These MUST be set to 1 for the assignment to work correctly
 * ============================================================================ */

#define INCLUDE_vTaskDelete                 1   // ✅ Required: Task self-deletion
#define INCLUDE_vTaskPrioritySet            1   // ✅ Required: Change task priority
#define INCLUDE_uxTaskPriorityGet           1   // ✅ Required: Get current priority
#define INCLUDE_vTaskDelayUntil             1   // ✅ Required: Exact 500ms timing
#define INCLUDE_vTaskDelay                  1   // ✅ Required: General delays

/* ============================================================================
 * Core FreeRTOS Configuration
 * ============================================================================ */

#define configUSE_PREEMPTION                1   // Enable preemptive scheduling
#define configUSE_IDLE_HOOK                 0   // Disable idle hook (not needed)
#define configUSE_TICK_HOOK                 0   // Disable tick hook (not needed)

// Platform-specific settings
#define configCPU_CLOCK_HZ                  ((unsigned long)100000000)  // 100 MHz
#define configTICK_RATE_HZ                  ((TickType_t)1000)          // 1ms tick
#define configMAX_PRIORITIES                (5)                         // 0-4 valid
#define configMINIMAL_STACK_SIZE            ((unsigned short)128)       // Min stack
#define configTOTAL_HEAP_SIZE               ((size_t)(15 * 1024))       // 15KB heap
#define configMAX_TASK_NAME_LEN             (16)                        // Task name length

/* ============================================================================
 * Memory Management & Safety
 * ============================================================================ */

#define configUSE_MALLOC_FAILED_HOOK        1   // ✅ Catch memory allocation failures
#define configCHECK_FOR_STACK_OVERFLOW      2   // ✅ Method 2: Thorough checking

/* ============================================================================
 * Queue Configuration
 * ============================================================================ */

#define configQUEUE_REGISTRY_SIZE           10  // For debugging queues

/* ============================================================================
 * Co-routine Configuration (Not Used)
 * ============================================================================ */

#define configUSE_CO_ROUTINES               0
#define configMAX_CO_ROUTINE_PRIORITIES     (2)

/* ============================================================================
 * Software Timer Configuration (Not Used)
 * ============================================================================ */

#define configUSE_TIMERS                    0

/* ============================================================================
 * Additional API Functions (Useful for Debugging)
 * ============================================================================ */

#define INCLUDE_xTaskGetSchedulerState      1   // Get scheduler state
#define INCLUDE_xTaskGetCurrentTaskHandle   1   // Get current task handle
#define INCLUDE_uxTaskGetStackHighWaterMark 1   // Check stack usage

/* ============================================================================
 * Platform-Specific Definitions (Adjust for your target)
 * ============================================================================ */

// For Windows/POSIX simulator
#ifdef _WIN32
    #define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#endif

// For STM32 or ARM Cortex-M
#ifdef __ARM_ARCH
    #define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
    // Add ARM-specific configurations here if needed
#endif

#endif /* FREERTOS_CONFIG_H */