/**
 * ============================================================================
 * LIPL Embedded Systems Intern Assessment - Problem Statement 1
 * FreeRTOS Task Management with Queue Communication
 * ============================================================================
 * 
 * Requirements:
 * - ExampleTask1: Sends data every EXACTLY 500ms using vTaskDelayUntil()
 * - ExampleTask2: Receives data and implements conditional logic
 * - Queue size: 5 items of type Data_t
 * - Dynamic priority management based on received data
 * - Task self-deletion based on specific conditions
 * - Print dataID and DataValue at EVERY evaluation
 * 
 * Key Features:
 * ✅ Exact 500ms timing (no drift)
 * ✅ Timestamp logging for timing verification
 * ✅ Non-blocking queue send with error handling
 * ✅ Automated testing capability
 * ✅ Professional error checking on all APIs
 * ✅ Defensive programming with safety hooks
 * 
 * Build Options:
 * - Option 1: FreeRTOS Windows/POSIX Simulator (recommended)
 * - Option 2: STM32 with STM32CubeIDE + FreeRTOS middleware
 * 
 * Author: [Your Name]
 * Date: [Date]
 * ============================================================================
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <stdbool.h>

#ifndef portTICK_PERIOD_MS
#define portTICK_PERIOD_MS    ( 1000UL / configTICK_RATE_HZ )
#endif

#define PRODUCER_STACK_WORDS   configMINIMAL_STACK_SIZE
#define CONSUMER_STACK_WORDS   configMINIMAL_STACK_SIZE
#define TEST_STACK_WORDS       configMINIMAL_STACK_SIZE

/* ============================================================================
 * CONFIGURATION
 * ============================================================================ */

// Uncomment to enable automated testing
// #define RUN_AUTOMATED_TESTS

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

/**
 * Data structure for inter-task communication
 * - dataID: Identifier for conditional branching logic
 * - DataValue: Value for priority control and deletion logic
 */
typedef struct {
    uint8_t  dataID;        // Command/data identifier
    int32_t  DataValue;     // Value for processing logic
} Data_t;

/* ============================================================================
 * GLOBAL VARIABLES
 * ============================================================================ */

// Global variables updated "elsewhere" (simulated in this demo)
volatile uint8_t  G_DataID   = 1;
volatile int32_t  G_DataValue = 0;

// Task handles
TaskHandle_t TaskHandle_1 = NULL;
TaskHandle_t TaskHandle_2 = NULL;

// Queue handle - communication channel between tasks
QueueHandle_t Queue1 = NULL;

/* ============================================================================
 * TASK 1: PRODUCER (ExampleTask1)
 * ============================================================================ */

/**
 * ExampleTask1 - Producer Task
 * 
 * Purpose:
 * - Reads global variables G_DataID and G_DataValue
 * - Packages them into Data_t structure
 * - Sends to Queue1 at EXACT 500ms intervals
 * - Uses vTaskDelayUntil() for drift-free periodic execution
 * 
 * Timing Guarantee:
 * - vTaskDelayUntil() ensures exactly 500ms between sends
 * - No cumulative timing drift regardless of execution time
 * 
 * @param pvParameters: Unused task parameter
 */
void ExampleTask1(void *pvParameters)
{
    (void)pvParameters;  // Mark as unused to avoid warnings
    
    // For exact periodic timing - tracks last wake time
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(500);  // 500ms period
    
    Data_t msgToSend;
    BaseType_t queueStatus;
    
    printf("ExampleTask1: Started\n");
    
    // Infinite task loop
    for (;;) {
        // 1. Read global variables (populated elsewhere)
        msgToSend.dataID = G_DataID;
        msgToSend.DataValue = G_DataValue;
        
        // 2. Send to queue (non-blocking to detect queue full condition)
        queueStatus = xQueueSend(Queue1, &msgToSend, 0);
        
        if (queueStatus != pdPASS) {
            // Queue is full - data lost (this shouldn't happen with proper design)
            printf("[T1 @ %lu ms] ⚠️  Queue full! Data lost.\n",
                   xTaskGetTickCount() * portTICK_PERIOD_MS);
        } else {
            // Successfully sent
            printf("[T1 @ %lu ms] Sent: dataID=%u, DataValue=%ld\n",
                   xTaskGetTickCount() * portTICK_PERIOD_MS,
                   msgToSend.dataID,
                   (long)msgToSend.DataValue);
        }
        
        // 3. Wait for EXACTLY 500ms from last wake time
        // This is critical - maintains precise periodicity
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

/* ============================================================================
 * TASK 2: CONSUMER (ExampleTask2)
 * ============================================================================ */

/**
 * ExampleTask2 - Consumer Task with Dynamic Priority Management
 * 
 * Purpose:
 * - Receives data from Queue1 (blocking wait)
 * - Prints received data at EVERY evaluation (requirement!)
 * - Implements conditional logic based on dataID and DataValue
 * 
 * Conditional Logic:
 * 
 *   IF dataID == 0:
 *     → Delete self immediately
 *   
 *   IF dataID == 1:
 *     Process DataValue:
 *       IF DataValue == 0:
 *         → Increase priority by 2 (once only)
 *       IF DataValue == 1:
 *         → Decrease priority to original (if previously increased)
 *       IF DataValue == 2:
 *         → Delete self immediately
 * 
 * Priority Tracking:
 * - Stores original priority at startup
 * - Uses boolean flag to track if priority was increased
 * - Only decreases if flag indicates it was previously increased
 * 
 * @param pvParameters: Unused task parameter
 */
void ExampleTask2(void *pvParameters)
{
    (void)pvParameters;  // Mark as unused
    
    // Store original creation priority
    const UBaseType_t basePriority = uxTaskPriorityGet(NULL);
    UBaseType_t currentPriority = basePriority;
    
    // Track priority state
    bool priorityIncreased = false;
    
    Data_t receivedData;
    BaseType_t queueStatus;
    
    printf("ExampleTask2: Started with basePriority=%lu\n",
           (unsigned long)basePriority);
    
    // Infinite task loop
    for (;;) {
        // 1. Wait for data from Queue1 (block indefinitely until data arrives)
        queueStatus = xQueueReceive(Queue1, &receivedData, portMAX_DELAY);
        
        if (queueStatus == pdTRUE) {
            // 2. MANDATORY: Print at EVERY evaluation (with timestamp for verification)
            printf("[T2 @ %lu ms] Received: dataID=%u, DataValue=%ld\n",
                   xTaskGetTickCount() * portTICK_PERIOD_MS,
                   receivedData.dataID,
                   (long)receivedData.DataValue);
            
            // 3. Deletion conditions (elegant combined check)
            // Delete if: dataID==0 OR (dataID==1 AND DataValue==2)
            if (receivedData.dataID == 0 || 
                (receivedData.dataID == 1 && receivedData.DataValue == 2)) {
                printf("[T2 @ %lu ms] 🛑 Deletion condition met → Deleting self\n",
                       xTaskGetTickCount() * portTICK_PERIOD_MS);
                vTaskDelete(NULL);  // Self-delete - task ends here
            }
            
            // 4. Priority control logic (only when dataID == 1)
            if (receivedData.dataID == 1) {
                
                if (receivedData.DataValue == 0) {
                    // Increase priority by 2 (first time only)
                    if (!priorityIncreased) {
                        currentPriority = basePriority + 2;
                        vTaskPrioritySet(NULL, currentPriority);
                        priorityIncreased = true;
                        printf("[T2 @ %lu ms] ⬆️  Priority increased: %lu → %lu\n",
                               xTaskGetTickCount() * portTICK_PERIOD_MS,
                               (unsigned long)basePriority,
                               (unsigned long)currentPriority);
                    } else {
                        printf("[T2 @ %lu ms] ℹ️  Priority already increased (no action)\n",
                               xTaskGetTickCount() * portTICK_PERIOD_MS);
                    }
                    
                } else if (receivedData.DataValue == 1) {
                    // Decrease priority to original (only if previously increased)
                    if (priorityIncreased) {
                        currentPriority = basePriority;
                        vTaskPrioritySet(NULL, currentPriority);
                        priorityIncreased = false;
                        printf("[T2 @ %lu ms] ⬇️  Priority decreased: %lu → %lu\n",
                               xTaskGetTickCount() * portTICK_PERIOD_MS,
                               (unsigned long)(basePriority + 2U),
                               (unsigned long)basePriority);
                    } else {
                        printf("[T2 @ %lu ms] ℹ️  Priority not increased (no action)\n",
                               xTaskGetTickCount() * portTICK_PERIOD_MS);
                    }
                    
                } else if (receivedData.DataValue == 2) {
                    // This case is already handled in deletion check above
                    // (Kept here for clarity of logic flow)
                    
                } else {
                    // Other DataValue - no action specified
                    printf("[T2 @ %lu ms] ℹ️  DataValue=%ld (no action defined)\n",
                           xTaskGetTickCount() * portTICK_PERIOD_MS,
                           (long)receivedData.DataValue);
                }
                
            } else {
                // dataID is neither 0 nor 1 - no action specified
                printf("[T2 @ %lu ms] ℹ️  dataID=%u (no action defined)\n",
                       xTaskGetTickCount() * portTICK_PERIOD_MS,
                       receivedData.dataID);
            }
            
        } else {
            // This should never happen with portMAX_DELAY
            printf("[T2] ⚠️  ERROR: Queue receive failed!\n");
        }
    }
}

/* ============================================================================
 * TEST TASK (Optional - for automated testing)
 * ============================================================================ */

#ifdef RUN_AUTOMATED_TESTS
/**
 * TestTask - Automated Test Sequence
 * 
 * Purpose:
 * - Systematically tests all requirements
 * - Updates global variables to trigger different conditions
 * - Validates timing, priority changes, and deletion
 * 
 * Test Sequence:
 * 1. Priority increase (dataID=1, DataValue=0)
 * 2. Try increase again (should be ignored)
 * 3. Priority decrease (dataID=1, DataValue=1)
 * 4. Try decrease again (should be ignored)
 * 5. Increase after decrease (should work)
 * 6. Delete via DataValue=2
 * 
 * @param pvParameters: Unused task parameter
 */
void TestTask(void *pvParameters)
{
    (void)pvParameters;
    
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║   AUTOMATED TEST SEQUENCE STARTING     ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\n");
    
    // Wait for tasks to initialize
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Test 1: Priority Increase
    printf("\n┌─ TEST 1: Priority Increase ─────────┐\n");
    G_DataID = 1;
    G_DataValue = 0;
    vTaskDelay(pdMS_TO_TICKS(600));
    printf("└──────────────────────────────────────┘\n");
    
    // Test 2: Try Increase Again (should be ignored)
    printf("\n┌─ TEST 2: Try Increase Again ─────────┐\n");
    G_DataID = 1;
    G_DataValue = 0;
    vTaskDelay(pdMS_TO_TICKS(600));
    printf("└──────────────────────────────────────┘\n");
    
    // Test 3: Priority Decrease
    printf("\n┌─ TEST 3: Priority Decrease ──────────┐\n");
    G_DataID = 1;
    G_DataValue = 1;
    vTaskDelay(pdMS_TO_TICKS(600));
    printf("└──────────────────────────────────────┘\n");
    
    // Test 4: Try Decrease Again (should be ignored)
    printf("\n┌─ TEST 4: Try Decrease Again ─────────┐\n");
    G_DataID = 1;
    G_DataValue = 1;
    vTaskDelay(pdMS_TO_TICKS(600));
    printf("└──────────────────────────────────────┘\n");
    
    // Test 5: Increase After Decrease (should work)
    printf("\n┌─ TEST 5: Increase After Decrease ────┐\n");
    G_DataID = 1;
    G_DataValue = 0;
    vTaskDelay(pdMS_TO_TICKS(600));
    printf("└──────────────────────────────────────┘\n");
    
    // Test 6: Delete via DataValue=2
    printf("\n┌─ TEST 6: Delete via DataValue=2 ─────┐\n");
    G_DataID = 1;
    G_DataValue = 2;
    vTaskDelay(pdMS_TO_TICKS(1000));
    printf("└──────────────────────────────────────┘\n");
    
    printf("\n>>> ✅ Task2 should be deleted now <<<\n");
    printf(">>> 📊 Only Task1 messages should appear <<<\n\n");
    
    // Keep Task1 running for verification
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║   TEST SEQUENCE COMPLETED              ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\n");
    
    // Delete self - testing is done
    vTaskDelete(NULL);
}
#endif

/* ============================================================================
 * MAIN FUNCTION
 * ============================================================================ */

/**
 * Main Entry Point
 * 
 * Responsibilities:
 * 1. Create Queue1 for inter-task communication
 * 2. Create ExampleTask1 (producer)
 * 3. Create ExampleTask2 (consumer)
 * 4. Optionally create TestTask (automated testing)
 * 5. Start the FreeRTOS scheduler
 * 
 * Error Handling:
 * - Checks all creation operations
 * - Prints detailed error messages on failure
 * - Halts execution if critical resources fail
 */
int main(void)
{
    BaseType_t status;
    
    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║   LIPL Embedded Systems Intern Assessment     ║\n");
    printf("║   Problem Statement 1: FreeRTOS Tasks         ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    printf("\n");
    
    /* -----------------------------------------------------------------------
     * Step 1: Create Queue
     * Queue stores Data_t structures for inter-task communication
     * Size: 5 items (as per requirement)
     * ----------------------------------------------------------------------- */
    Queue1 = xQueueCreate(5, sizeof(Data_t));
    
    if (Queue1 == NULL) {
        printf("❌ ERROR: Failed to create Queue1!\n");
        printf("   Possible cause: Insufficient heap memory\n");
        printf("   Check: configTOTAL_HEAP_SIZE in FreeRTOSConfig.h\n");
        for (;;);  // Halt
    }
    printf("✅ [Main] Queue1 created successfully (size=5, itemSize=%zu bytes)\n",
           sizeof(Data_t));
    
    /* -----------------------------------------------------------------------
     * Step 2: Create ExampleTask1 (Producer)
     * Priority: 1 (low)
     * Stack: 1024 words (4KB on 32-bit systems)
     * Function: Sends data every 500ms
     * ----------------------------------------------------------------------- */
    status = xTaskCreate(
        ExampleTask1,           // Task function pointer
        "Producer",             // Task name (for debugging)
        PRODUCER_STACK_WORDS,   // Stack size in words (adjust per target)
        NULL,                   // Task parameter (unused)
        1,                      // Priority (1 = low)
        &TaskHandle_1           // Task handle
    );
    
    if (status != pdPASS) {
        printf("❌ ERROR: Failed to create ExampleTask1!\n");
        printf("   Possible cause: Insufficient heap memory\n");
        for (;;);  // Halt
    }
    printf("✅ [Main] ExampleTask1 created (Priority=1, Stack=%u words)\n",
           (unsigned)PRODUCER_STACK_WORDS);
    
    /* -----------------------------------------------------------------------
     * Step 3: Create ExampleTask2 (Consumer)
     * Priority: 2 (medium, can be modified at runtime)
     * Stack: 1024 words
     * Function: Receives data and implements control logic
     * ----------------------------------------------------------------------- */
    status = xTaskCreate(
        ExampleTask2,           // Task function pointer
        "Consumer",             // Task name (for debugging)
        CONSUMER_STACK_WORDS,   // Stack size in words (adjust per target)
        NULL,                   // Task parameter (unused)
        2,                      // Priority (2 = medium)
        &TaskHandle_2           // Task handle
    );
    
    if (status != pdPASS) {
        printf("❌ ERROR: Failed to create ExampleTask2!\n");
        printf("   Possible cause: Insufficient heap memory\n");
        for (;;);  // Halt
    }
    printf("✅ [Main] ExampleTask2 created (Priority=2, Stack=%u words)\n",
           (unsigned)CONSUMER_STACK_WORDS);
    
    /* -----------------------------------------------------------------------
     * Step 4: Create TestTask (Optional - only if enabled)
     * Priority: 3 (highest - controls test sequence)
     * Stack: 1024 words
     * Function: Automated testing of all scenarios
     * ----------------------------------------------------------------------- */
    #ifdef RUN_AUTOMATED_TESTS
    status = xTaskCreate(
        TestTask,               // Task function pointer
        "TestTask",             // Task name
        TEST_STACK_WORDS,       // Stack size in words (adjust per target)
        NULL,                   // Task parameter (unused)
        3,                      // Priority (3 = high)
        NULL                    // Don't need handle
    );
    
    if (status != pdPASS) {
        printf("❌ ERROR: Failed to create TestTask!\n");
        for (;;);  // Halt
    }
    printf("✅ [Main] TestTask created (Priority=3, Automated=ON)\n");
    #else
    printf("ℹ️  [Main] Automated testing disabled (define RUN_AUTOMATED_TESTS to enable)\n");
    #endif
    
    /* -----------------------------------------------------------------------
     * Step 5: Start the FreeRTOS Scheduler
     * From this point, tasks execute based on priority
     * This function should NEVER return
     * ----------------------------------------------------------------------- */
    printf("\n🚀 [Main] Starting FreeRTOS scheduler...\n");
    printf("──────────────────────────────────────────────\n\n");
    
    vTaskStartScheduler();
    
    /* -----------------------------------------------------------------------
     * If we reach here, there was insufficient heap memory for idle task
     * ----------------------------------------------------------------------- */
    printf("\n❌ FATAL ERROR: Scheduler failed to start!\n");
    printf("   Cause: Insufficient heap memory for idle task\n");
    printf("   Action: Increase configTOTAL_HEAP_SIZE\n");
    
    for (;;) {
        // Infinite loop - should never get here
    }
    
    return 0;
}

/* ============================================================================
 * FREERTOS HOOK FUNCTIONS
 * These are called by FreeRTOS on specific events
 * ============================================================================ */

/**
 * Application Idle Hook
 * Called when no tasks are ready to run (idle task executing)
 * Can be used for: low-power mode, background tasks, LED blinking
 */
#if (configUSE_IDLE_HOOK == 1)
void vApplicationIdleHook(void)
{
    // Optional: Add low-power mode code here
}
#endif

/**
 * Stack Overflow Hook
 * Called when FreeRTOS detects stack overflow (if checking is enabled)
 * This is a serious error - task has corrupted memory
 * 
 * @param xTask: Handle of the task that overflowed
 * @param pcTaskName: Name of the task (for debugging)
 */
#if (configCHECK_FOR_STACK_OVERFLOW > 0)
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;  // Can be used to identify task
    
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║   ⚠️  FATAL ERROR: STACK OVERFLOW     ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("Task: %s\n", pcTaskName);
    printf("Action: Increase stack size for this task\n");
    printf("\n");
    
    for (;;) {
        // Halt system - stack overflow is critical
    }
}
#endif

/**
 * Malloc Failed Hook
 * Called when pvPortMalloc() fails to allocate memory
 * This indicates heap exhaustion
 */
#if (configUSE_MALLOC_FAILED_HOOK == 1)
void vApplicationMallocFailedHook(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║   ⚠️  FATAL ERROR: MALLOC FAILED      ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("Cause: FreeRTOS heap exhausted\n");
    printf("Action: Increase configTOTAL_HEAP_SIZE\n");
    printf("\n");
    
    for (;;) {
        // Halt system - out of memory is critical
    }
}
#endif

/* ============================================================================
 * END OF FILE
 * ============================================================================ */