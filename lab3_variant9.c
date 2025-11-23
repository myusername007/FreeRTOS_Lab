// =============================================================
//  FreeRTOS — Lab 3 (Queues)
//  Variant 9
//  2 tasks: Producer & Consumer
//  Queue element: struct { char ch; uint16_t data; }
//  Send timeout: 100 ms
//  Receive timeout: 300 ms
//  Consumer calculates avg of last 5 received values
//  START FUNCTION: vStartLab3_Variant9()
// =============================================================

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"


typedef struct
{
    char     ch;
    uint16_t data;
} QueueItem_t;

//settings
#define QUEUE_LEN        10
#define SEND_TIMEOUT_MS  100
#define RECV_TIMEOUT_MS  300

#define PRODUCER_PERIOD  200
#define CONSUMER_PERIOD  150

//global queue
static QueueHandle_t xQueue = NULL;



static void vProducer(void* pvParameters);
static void vConsumer(void* pvParameters);

//  start function
void vStartLab3_Variant9(void)
{
    printf("==== Lab3 Variant 9 START ====\n");

    //new queue
    xQueue = xQueueCreate(QUEUE_LEN, sizeof(QueueItem_t));
    configASSERT(xQueue != NULL);

    // Producer
    configASSERT(
        xTaskCreate(
            vProducer,
            "Producer",
            1024,
            NULL,
            2,
            NULL
        ) == pdPASS
    );

    // Consumer
    configASSERT(
        xTaskCreate(
            vConsumer,
            "Consumer",
            1024,
            NULL,
            1,
            NULL
        ) == pdPASS
    );
}

//     Producer Task
static void vProducer(void* pvParameters)
{
    QueueItem_t item;
    uint16_t value = 0;
    char c = 'A';

    (void)pvParameters;

    for (;;)
    {
        value += 10;
        item.ch = c;
        item.data = value;

        if (xQueueSend(xQueue, &item, pdMS_TO_TICKS(SEND_TIMEOUT_MS)) == pdPASS)
        {
            printf("[Producer] Sent {%c, %u}\n", item.ch, item.data);
        }
        else
        {
            printf("[Producer] QUEUE FULL\n");
        }

        c++;
        if (c > 'Z') c = 'A';

        vTaskDelay(pdMS_TO_TICKS(PRODUCER_PERIOD));
    }
}

//     Consumer Task
static void vConsumer(void* pvParameters)
{
    QueueItem_t item;

    (void)pvParameters;

    uint16_t last5[5] = { 0 };
    uint8_t count = 0;
    uint8_t index = 0;
    uint32_t sum = 0;

    for (;;)
    {
        if (xQueueReceive(xQueue, &item, pdMS_TO_TICKS(RECV_TIMEOUT_MS)) == pdPASS)
        {
            if (count < 5)
            {
                last5[count] = item.data;
                sum += item.data;
                count++;
            }
            else
            {
                sum -= last5[index];
                last5[index] = item.data;
                sum += item.data;
                index = (index + 1) % 5;
            }

            float avg = (float)sum / count;

            printf("[Consumer] Got {%c, %u} | avg(last %u) = %.2f\n",
                item.ch, item.data, count, avg);
        }
        else
        {
            printf("[Consumer] TIMEOUT\n");
        }

        vTaskDelay(pdMS_TO_TICKS(CONSUMER_PERIOD));
    }
}
