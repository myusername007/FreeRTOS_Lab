#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "message_buffer.h"
#include "stream_buffer.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>



#define STREAM_BUF_SIZE     256
#define MSG_BUF_SIZE        128
#define JOB_QUEUE_LIMIT     5


#define BIT_JOBS_PENDING     (1 << 0)
#define BIT_QUEUE_OVERLOAD   (1 << 1)
#define BIT_WORKER_IDLE      (1 << 2)
#define BIT_JOB_FAILED       (1 << 3)

typedef struct {
    uint32_t id;
    uint8_t priority;
    uint16_t payload;
} Job_t;



MessageBufferHandle_t xMsgBuffer = NULL;
StreamBufferHandle_t  xStreamBuffer = NULL;
EventGroupHandle_t    xEventGroup = NULL;

TaskHandle_t xWorkerHandle = NULL;

static int jobQueueCount = 0;


void JobGeneratorTask(void* pv)
{
    uint32_t id = 1;

    printf("GEN: Task started\n");

    while (1)
    {
        Job_t job = {
            .id = id,
            .priority = id % 3,
            .payload = 100 + id
        };

        /* send to message buffer */
        xMessageBufferSend(xMsgBuffer, &job, sizeof(job), portMAX_DELAY);

        /* raw log */
        char log[64];
        sprintf(log, "GEN: id=%lu pr=%d\n", id, job.priority);
        xStreamBufferSend(xStreamBuffer, log, strlen(log), 0);

        xEventGroupSetBits(xEventGroup, BIT_JOBS_PENDING);

        printf("GEN: job %lu sent\n", id);

        id++;
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}


void JobQueueTask(void* pv)
{
    Job_t job;

    printf("QUEUE: Task started\n");

    while (1)
    {
        if (xMessageBufferReceive(xMsgBuffer, &job, sizeof(job), portMAX_DELAY) > 0)
        {
            jobQueueCount++;

            if (jobQueueCount > JOB_QUEUE_LIMIT)
                xEventGroupSetBits(xEventGroup, BIT_QUEUE_OVERLOAD);

            char log[64];
            sprintf(log, "QUEUE: got id=%lu q=%d\n", job.id, jobQueueCount);
            xStreamBufferSend(xStreamBuffer, log, strlen(log), 0);

            printf("QUEUE: received job %lu\n", job.id);

            /* Notify worker */
            if (xWorkerHandle != NULL)
            {
                xTaskNotifyGive(xWorkerHandle);
            }
        }
    }
}


void WorkerTask(void* pv)
{
    printf("WORKER: Task started\n");

    while (1)
    {
        /* Wait for notify */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (jobQueueCount <= 0)
        {
            xEventGroupSetBits(xEventGroup, BIT_JOB_FAILED);
            printf("WORKER: ERROR (empty queue)\n");
            continue;
        }

        jobQueueCount--;

        printf("WORKER: processed job, left=%d\n", jobQueueCount);

        char log[64];
        sprintf(log, "WORK: done left=%d\n", jobQueueCount);
        xStreamBufferSend(xStreamBuffer, log, strlen(log), 0);

        if (jobQueueCount == 0)
            xEventGroupSetBits(xEventGroup, BIT_WORKER_IDLE);

        vTaskDelay(pdMS_TO_TICKS(150));
    }
}


void MonitorTask(void* pv)
{
    printf("MONITOR: Task started\n");

    while (1)
    {
        EventBits_t bits = xEventGroupWaitBits(
            xEventGroup,
            BIT_JOBS_PENDING | BIT_QUEUE_OVERLOAD | BIT_WORKER_IDLE | BIT_JOB_FAILED,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(300)
        );

        if (bits & BIT_QUEUE_OVERLOAD)
            printf("MONITOR: Queue overloaded!\n");

        if (bits & BIT_WORKER_IDLE)
            printf("MONITOR: Worker idle.\n");

        if (bits & BIT_JOB_FAILED)
            printf("MONITOR: Job failed!\n");

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}



void vStartLab6_Variant9(void)
{
    printf("Starting Lab 6 Variant 9...\n");

    xMsgBuffer = xMessageBufferCreate(MSG_BUF_SIZE);
    xStreamBuffer = xStreamBufferCreate(STREAM_BUF_SIZE, 1);
    xEventGroup = xEventGroupCreate();

    configASSERT(xMsgBuffer);
    configASSERT(xStreamBuffer);
    configASSERT(xEventGroup);

    /* Create tasks */
    xTaskCreate(JobGeneratorTask, "JobGen", 512, NULL, 2, NULL);
    xTaskCreate(JobQueueTask, "JobQueue", 512, NULL, 2, NULL);
    xTaskCreate(WorkerTask, "Worker", 512, NULL, 2, &xWorkerHandle);
    xTaskCreate(MonitorTask, "Monitor", 512, NULL, 1, NULL);
}
