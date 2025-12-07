#include <stdio.h>
#include <conio.h>     
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"


TaskHandle_t xControlTask = NULL;
TaskHandle_t xDownloadTask = NULL;
TaskHandle_t xUiTask = NULL;

TimerHandle_t xDownloadTickTimer = NULL;
TimerHandle_t xTimeoutTimer = NULL;

static int running = 0;        // 1 = downloading, 0 = paused/stopped
static int progress = 0;       // %
static int blockSize = 5;      // progress +5% each tick
static int restartCount = 0;   // number of starts


// periodic timer → every 100 ms generate "download tick"
void vDownloadTickCallback(TimerHandle_t xTimer)
{
    if (running)
    {
        xTaskNotifyGive(xDownloadTask); // семафор: "другий блок готовий"
    }
}

// one-shot timeout
void vTimeoutCallback(TimerHandle_t xTimer)
{
    uint32_t errCode = 999;  // error code for UI Task
    xTaskNotify(xUiTask, errCode, eSetValueWithOverwrite);
}


void vDownloadTask(void* pvParams)
{
    for (;;)
    {
        // wait for tick from periodic timer
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (!running)
            continue;

        // simulate download block
        progress += blockSize;
        if (progress > 100)
            progress = 100;

        // restart timeout timer
        xTimerReset(xTimeoutTimer, 0);

        // send progress to UI
        xTaskNotify(xUiTask, progress, eSetValueWithOverwrite);

        if (progress == 100)
        {
            running = 0;
            printf("[DownloadTask] Download complete.\n");
        }
    }
}



void vUiTask(void* pvParams)
{
    uint32_t value;

    for (;;)
    {
        if (xTaskNotifyWait(0, 0, &value, portMAX_DELAY))
        {
            if (value == 999)
            {
                printf("[UI] ERROR: Timeout — no progress.\n");
            }
            else
            {
                printf("[UI] Progress: %u%%\n", value);
            }
        }
    }
}


void vControlTask(void* pvParams)
{
    for (;;)
    {
        printf("\n--- Control ---\n");
        printf("1 = start   2 = stop   3 = pause\n");
        printf("Press key: ");

        // wait for any key
        while (!_kbhit())
        {
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        char key = _getch();
        printf("%c\n", key);

        if (key == '1')
        {
            restartCount++;
            running = 1;
            progress = 0;

            printf("[Control] START (attempt #%d)\n", restartCount);

            xTimerStart(xDownloadTickTimer, 0);
            xTimerReset(xTimeoutTimer, 0);
        }
        else if (key == '2')
        {
            running = 0;
            printf("[Control] STOP\n");
        }
        else if (key == '3')
        {
            running = 0;
            printf("[Control] PAUSE\n");
        }
    }
}



void vStartLab5_Variant9(void)
{

    // Periodic timer (download tick)
    xDownloadTickTimer = xTimerCreate(
        "TickTimer",
        pdMS_TO_TICKS(100),
        pdTRUE,
        NULL,
        vDownloadTickCallback
    );
    configASSERT(xDownloadTickTimer != NULL);

    // One-shot timeout timer
    xTimeoutTimer = xTimerCreate(
        "TimeoutTimer",
        pdMS_TO_TICKS(1500),
        pdFALSE,
        NULL,
        vTimeoutCallback
    );
    configASSERT(xTimeoutTimer != NULL);

    // UI Task
    configASSERT(
        xTaskCreate(
            vUiTask,
            "UiTask",
            1024,
            NULL,
            1,
            &xUiTask
        ) == pdPASS
    );

    // Download Task
    configASSERT(
        xTaskCreate(
            vDownloadTask,
            "DownloadTask",
            1024,
            NULL,
            2,
            &xDownloadTask
        ) == pdPASS
    );

    // Control Task
    configASSERT(
        xTaskCreate(
            vControlTask,
            "ControlTask",
            1024,
            NULL,
            3,
            &xControlTask
        ) == pdPASS
    );

    printf("Tasks and timers created.\n");
    printf("Press 1 / 2 / 3 to control download.\n\n");
}
