// =============================================================
//  FreeRTOS Lab 4 Ч Mutex
//  Variant 9: Critical UART resource (3 tasks printing via Mutex)
//  Start function: vStartLab4_Variant9()
// =============================================================

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>


// Mutex handle
static SemaphoreHandle_t xUartMutex = NULL;


// Task prototypes
static void vUartTask(void* pvParameters);


//  START FUNCTION
void vStartLab4_Variant9(void)
{
    printf("==== Lab4 Variant 9 START ====\n");

    // MUTEX
    xUartMutex = xSemaphoreCreateMutex();
    configASSERT(xUartMutex != NULL);

    // 3 задач≥
    for (int i = 0; i < 3; i++)
    {
        char name[10];
        snprintf(name, sizeof(name), "User%d", i + 1);

        configASSERT(
            xTaskCreate(
                vUartTask,
                name,
                1024,
                (void*)(uintptr_t)(i + 1),
                1,
                NULL
            ) == pdPASS
        );
    }
}

//  UART Task Ч prints safely via Mutex
static void vUartTask(void* pvParameters)
{
    int id = (int)(uintptr_t)pvParameters;

    for (;;)
    {
       
        xSemaphoreTake(xUartMutex, portMAX_DELAY);

        // UART
        printf("[User %d] >>> Begin message\n", id);
        vTaskDelay(pdMS_TO_TICKS(50));
        printf("[User %d] Working...\n", id);
        vTaskDelay(pdMS_TO_TICKS(50));
        printf("[User %d] <<< End message\n", id);

        
        xSemaphoreGive(xUartMutex);

        vTaskDelay(pdMS_TO_TICKS(400 + 100 * id));
    }
}
