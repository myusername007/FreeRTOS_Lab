// FreeRTOS Lab 2 — Thread Local Storage (TLS)
// Variant 9: (Ntasks=3, mode=BUSY, Burst(B)=9 lines, TLS indices V1/V2 = 1/0, Base=18400 cycles)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

// ===== Variant 9 parameters =====
#define IDX                 9
#define NTASKS              3
#define MODE_BUSY           1
#define TLS_INDEX_V1        1   // TaskContext
#define TLS_INDEX_V2        0   // Profile
#define BASE_CYCLES         18400UL
#define STEP_CYCLES         800UL

// ===== Structures from the lab spec =====
typedef enum { MODE_DELAY = 0, MODE_BUSY_ENUM = 1 } run_mode_t;

#define BURST_MAX 10

typedef struct {
    char     name[12];
    uint32_t iter;                // current iteration
    uint32_t checksum;            // rolling checksum (CRC8 over iter & seed)
    uint32_t delayTicksOrCycles;  // used per-run based on mode
    uint32_t seed;                // initial seed = IDX
    char     line1[BURST_MAX][64];// burst buffer (not used separately here)
    char     line2[BURST_MAX][64];// optional second buffer if needed
    uint8_t  bcnt;                // how many lines accumulated
} task_context_t;

typedef struct {
    run_mode_t mode;        // delay or busy
    uint8_t    burstCount;  // how many lines to accumulate before print (<=10)
    uint32_t   baseDelayMs; // for MODE_DELAY
    uint32_t   baseCycles;  // for MODE_BUSY
    uint8_t    step;        // step between tasks (7 ms or 800 cycles)
} profile_t;

// ===== Helpers =====
static inline void busy_loop(uint32_t cycles)
{
    volatile uint32_t x = 0;
    for (uint32_t i = 0; i < cycles; ++i) { x += i; }
    (void)x; // keep compiler from optimizing away
}

// CRC-8 SAE J1850 (poly 0x1D), init 0xFF, final XOR 0xFF
static uint8_t crc8_sae_j1850(const uint8_t* data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; ++b) {
            if (crc & 0x80) crc = (uint8_t)((crc << 1) ^ 0x1D);
            else            crc <<= 1;
        }
    }
    return (uint8_t)(crc ^ 0xFF);
}

static uint32_t checksum_update(uint32_t iter, uint32_t seed)
{
    uint8_t buf[8];
    memcpy(buf, &iter, 4);
    memcpy(buf + 4, &seed, 4);
    return (uint32_t)crc8_sae_j1850(buf, sizeof(buf));
}

static void burst_flush(task_context_t* ctx)
{
    for (uint8_t k = 0; k < ctx->bcnt; ++k) {
        printf("%s\n", ctx->line1[k]);
    }
    ctx->bcnt = 0;
}

// ===== Worker task =====
static void vWorker(void* arg)
{
    const int i = (int)(uintptr_t)arg; // task index [0..NTASKS-1]

    // Allocate and initialize TLS objects
    task_context_t* ctx = (task_context_t*)pvPortMalloc(sizeof(task_context_t));
    profile_t* prof = (profile_t*)pvPortMalloc(sizeof(profile_t));

    configASSERT(ctx && prof);

    memset(ctx, 0, sizeof(*ctx));
    snprintf(ctx->name, sizeof(ctx->name), "T%02d_%d", IDX, i);
    ctx->iter = 0;
    ctx->seed = IDX;
    ctx->bcnt = 0;

    prof->mode = MODE_BUSY_ENUM;
    prof->burstCount = 9;              // from Variant 9 (B=9)
    prof->baseDelayMs = 0;              // not used for busy
    prof->baseCycles = BASE_CYCLES;
    prof->step = 1;              // step unit; cycles step is 800*i below

    // Put into TLS
    vTaskSetThreadLocalStoragePointer(NULL, TLS_INDEX_V1, (void*)ctx);
    vTaskSetThreadLocalStoragePointer(NULL, TLS_INDEX_V2, (void*)prof);

    const uint32_t Ni = 10 * NTASKS + IDX; // iterations per task

    for (uint32_t n = 0; n < Ni; ++n) {
        // Update iteration & checksum
        ctx->iter++;
        ctx->checksum = checksum_update(ctx->iter, ctx->seed);

        // Prepare log line
        TickType_t tick = xTaskGetTickCount();
        char line[64];
        snprintf(line, sizeof(line), "[Task %s] Tick:%lu Iter:%lu Sum:0x%08lX",
            ctx->name, (unsigned long)tick, (unsigned long)ctx->iter,
            (unsigned long)ctx->checksum);

        // Burst or immediate print
        if (prof->burstCount == 0) {
            // immediate printing mode
            printf("%s\n", line);
        }
        else {
            strncpy(ctx->line1[ctx->bcnt], line, sizeof(ctx->line1[0]) - 1);
            ctx->line1[ctx->bcnt][sizeof(ctx->line1[0]) - 1] = '\0';
            ctx->bcnt++;
            if (ctx->bcnt >= prof->burstCount || ctx->bcnt >= BURST_MAX) {
                burst_flush(ctx);
            }
        }

        // Do the work (busy or delay)
        if (prof->mode == MODE_DELAY) {
            TickType_t d = pdMS_TO_TICKS(prof->baseDelayMs + 7 * i);
            vTaskDelay(d);
        }
        else {
            uint32_t cyc = prof->baseCycles + STEP_CYCLES * i;
            ctx->delayTicksOrCycles = cyc;
            busy_loop(cyc);
            // yield so other tasks run fairly
            taskYIELD();
        }
    }

    // Flush remaining logs if any
    if (ctx->bcnt) burst_flush(ctx);

    printf("Task finished %s\n", ctx->name);

    // Free TLS objects
    vPortFree(ctx);
    vPortFree(prof);

    // Clear TLS slots (optional but neat)
    vTaskSetThreadLocalStoragePointer(NULL, TLS_INDEX_V1, NULL);
    vTaskSetThreadLocalStoragePointer(NULL, TLS_INDEX_V2, NULL);

    vTaskDelete(NULL);
}

// ===== Startup =====
void vStartVariant9(void)
{
    for (int i = 0; i < NTASKS; ++i) {
        BaseType_t ok = xTaskCreate(
            vWorker,
            "Task",
            1024,
            (void*)(uintptr_t)i,
            tskIDLE_PRIORITY + 1,
            NULL
        );
        configASSERT(ok == pdPASS);
    }
}

