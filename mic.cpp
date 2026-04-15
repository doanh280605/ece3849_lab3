#include "mic.h"

#include <math.h>

extern "C" {
#include "FreeRTOS.h"
#include "timers.h"
#include "semphr.h"
#include "driverlib/adc.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "inc/hw_memmap.h"
}

#define WINDOW_SIZE       128
#define MIC_ADC_BASE      ADC0_BASE
#define MIC_ADC_SEQ       3
#define MIC_ADC_CHANNEL   ADC_CTL_CH8

volatile float gMicRMS   = 0.0f;
volatile float gMicDb    = -60.0f;
volatile float gMicLevel = 0.0f;

static TimerHandle_t     xMicTimer    = NULL;
static SemaphoreHandle_t xMicReadySem = NULL;
static uint16_t          gMicSamples[WINDOW_SIZE];
static uint16_t          gMicIndex    = 0;

// ---------------------------------------------------------------------------
// Hardware driver
// ---------------------------------------------------------------------------

void Mic_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0) ||
           !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)) {}

    // PE5 = AIN8 — configure as analog input
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_5);

    // Sequencer 3: software trigger, one step, read AIN8 then stop
    ADCSequenceConfigure(MIC_ADC_BASE, MIC_ADC_SEQ, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(MIC_ADC_BASE,
                             MIC_ADC_SEQ,
                             0,
                             MIC_ADC_CHANNEL | ADC_CTL_IE | ADC_CTL_END);
    ADCSequenceEnable(MIC_ADC_BASE, MIC_ADC_SEQ);
    ADCIntClear(MIC_ADC_BASE, MIC_ADC_SEQ);
}

uint16_t Mic_Read(void)
{
    uint32_t value;

    ADCProcessorTrigger(MIC_ADC_BASE, MIC_ADC_SEQ);
    while (!ADCIntStatus(MIC_ADC_BASE, MIC_ADC_SEQ, false)) {}
    ADCIntClear(MIC_ADC_BASE, MIC_ADC_SEQ);
    ADCSequenceDataGet(MIC_ADC_BASE, MIC_ADC_SEQ, &value);

    return (uint16_t)value;
}

// ---------------------------------------------------------------------------
// Timer callback — runs in the FreeRTOS timer daemon task, NOT an ISR.
// Collects one sample per 1 ms tick. When the window is full, signals MicTask.
// ---------------------------------------------------------------------------

static void MicSampleCb(TimerHandle_t xTimer)
{
    (void)xTimer;

    gMicSamples[gMicIndex++] = Mic_Read();

    if (gMicIndex >= WINDOW_SIZE) {
        gMicIndex = 0;
        // Not an ISR, so use the normal (non-FromISR) give
        xSemaphoreGive(xMicReadySem);
    }
}

// ---------------------------------------------------------------------------
// Init — call once from main() before the scheduler starts
// ---------------------------------------------------------------------------

void ScreenMic_Init(void)
{
    Mic_Init();

    xMicReadySem = xSemaphoreCreateBinary();

    xMicTimer = xTimerCreate("mic",
                             pdMS_TO_TICKS(1),  // fires every 1 ms
                             pdTRUE,            // auto-reload (periodic)
                             NULL,
                             MicSampleCb);

    xTimerStart(xMicTimer, 0);
}

// ---------------------------------------------------------------------------
// Task — sleeps until a full 128-sample window is ready, then computes RMS
// ---------------------------------------------------------------------------

void MicTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        // Block here — wakes up every 128 ms when the window is full
        xSemaphoreTake(xMicReadySem, portMAX_DELAY);

        // Step 1+2: normalize each raw sample to [0, 1]
        // Step 3:   subtract 0.5 to remove DC bias → centered around zero
        // Step 4+5: square and accumulate for mean square
        float sum = 0.0f;
        for (int i = 0; i < WINDOW_SIZE; i++) {
            float s = (float)gMicSamples[i] / 4095.0f;  // normalize to [0,1]
            float a = s - 0.5f;                          // remove DC bias
            sum += a * a;                                // accumulate squares
        }

        // Step 6: RMS = sqrt( mean of squares )
        gMicRMS = sqrtf(sum / (float)WINDOW_SIZE);

        // Step 7: convert to dB  (reference amplitude = 0.25)
        gMicDb = 20.0f * log10f(gMicRMS / 0.25f);

        // Step 8: clamp to [-60, 0] dB
        if (gMicDb < -60.0f) gMicDb = -60.0f;
        if (gMicDb >   0.0f) gMicDb =   0.0f;

        // Step 9: map to [0, 1] for the display bar
        gMicLevel = (gMicDb + 60.0f) / 60.0f;
    }
}
