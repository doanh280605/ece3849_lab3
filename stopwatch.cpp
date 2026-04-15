#include "stopwatch.h"

#include "../libraries/buttonsDriver/button.h"
#include "buzzer.h"

extern "C" {
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "inc/hw_ints.h"
}

volatile bool gRunning = false;
volatile uint16_t g_ms = 0;
volatile uint8_t g_sec = 0;
volatile uint8_t g_min = 0;
volatile uint8_t g_hr = 0;

MyButton gStartButton = {12, 88, 48, 26, "PLAY", false};
MyButton gResetButton = {68, 88, 48, 26, "RESET", false};

static SemaphoreHandle_t xBtnSem = NULL;

#define BTN_PORT_BASE  GPIO_PORTL_BASE
#define BTN_PIN_MASK   (GPIO_PIN_1 | GPIO_PIN_2)
#define BTN_INT_NUM    INT_GPIOL

void ButtonISR(void)
{
    GPIOIntClear(BTN_PORT_BASE, BTN_PIN_MASK);

    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(xBtnSem, &woken);
    portYIELD_FROM_ISR(woken);
}

void Stopwatch_Init(void)
{
    gRunning = false;
    g_ms = 0;
    g_sec = 0;
    g_min = 0;
    g_hr = 0;

    gStartButton.label = "PLAY";
    gStartButton.pressed = false;
    gResetButton.label = "RESET";
    gResetButton.pressed = false;

    // Create semaphore BEFORE enabling the interrupt
    xBtnSem = xSemaphoreCreateBinary();

    // Enable PORTL clock so the GPIO registers are accessible
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOL)) {}

    // Register ISR, trigger on both edges (press + release)
    GPIOIntRegister(BTN_PORT_BASE, ButtonISR);
    GPIOIntTypeSet(BTN_PORT_BASE, BTN_PIN_MASK, GPIO_BOTH_EDGES);
    GPIOIntEnable(BTN_PORT_BASE, BTN_PIN_MASK);
    IntEnable(BTN_INT_NUM);
}

void TimeTask(void *pvParameters)
{
    (void)pvParameters;

    const TickType_t xPeriod = pdMS_TO_TICKS(10);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xPeriod);

        if (gRunning) {
            g_ms += 10;

            if (g_ms >= 1000) {
                g_ms -= 1000;
                g_sec++;

                if (g_sec >= 60) {
                    g_sec = 0;
                    g_min++;

                    if (g_min >= 60) {
                        g_min = 0;
                        g_hr++;

                        if (g_hr >= 24) {
                            g_hr = 0;
                        }
                    }
                }
            }
        }
    }
}

void ButtonTask(void *pvParameters)
{
    (void)pvParameters;

    Button btnPlayPause(S1);
    Button btnResetHw(S2);

    // begin() enables the peripheral clock and configures the pin as input
    btnPlayPause.begin();
    btnPlayPause.setTickIntervalMs(20);
    btnPlayPause.setDebounceMs(30);

    btnResetHw.begin();
    btnResetHw.setTickIntervalMs(20);
    btnResetHw.setDebounceMs(30);

    for (;;) {
        // Block here — wake only when the ISR fires
        xSemaphoreTake(xBtnSem, portMAX_DELAY);

        // Short delay so the GPIO level has settled after the edge
        vTaskDelay(pdMS_TO_TICKS(15));

        btnPlayPause.tick();
        btnResetHw.tick();

        if (btnPlayPause.wasPressed()) {
            gRunning = !gRunning;
            gStartButton.pressed = true;
            Buzzer_Beep(3000U, 30U);
        }

        if (btnPlayPause.wasReleased()) {
            gStartButton.pressed = false;
        }

        if (btnResetHw.wasPressed()) {
            g_ms = 0;
            g_sec = 0;
            g_min = 0;
            g_hr = 0;
            gResetButton.pressed = true;
            Buzzer_Beep(2200U, 40U);
        }

        if (btnResetHw.wasReleased()) {
            gResetButton.pressed = false;
        }
    }
}
