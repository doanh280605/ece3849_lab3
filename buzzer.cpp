#include "buzzer.h"

extern "C" {
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
}

static uint32_t gBuzzerSystemClock = 0;

QueueHandle_t gBuzzerQ = NULL;

static void Buzzer_Start(uint32_t frequency);
static void Buzzer_Stop(void);

void Buzzer_Init(uint32_t systemClock)
{
    gBuzzerSystemClock = systemClock;

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF) ||
           !SysCtlPeripheralReady(SYSCTL_PERIPH_PWM0)) {
    }

    GPIOPinConfigure(GPIO_PF1_M0PWM1);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);

    PWMClockSet(PWM0_BASE, PWM_SYSCLK_DIV_1);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenDisable(PWM0_BASE, PWM_GEN_0);
    PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, false);

    gBuzzerQ = xQueueCreate(4, sizeof(BuzzerCmd));
}

void BuzzerTask(void *pvParameters)
{
    (void)pvParameters;

    BuzzerCmd cmd;
    for (;;) {
        if (xQueueReceive(gBuzzerQ, &cmd, portMAX_DELAY) == pdPASS) {
            Buzzer_Start(cmd.frequency);
            vTaskDelay(pdMS_TO_TICKS(cmd.duration));
            Buzzer_Stop();
        }
    }
}

bool Buzzer_Beep(uint32_t frequency, uint32_t duration)
{
    BuzzerCmd cmd = {frequency, duration};
    return (gBuzzerQ != NULL) && (xQueueSend(gBuzzerQ, &cmd, 0) == pdPASS);
}

static void Buzzer_Start(uint32_t frequency)
{
    if (frequency == 0U) {
        Buzzer_Stop();
        return;
    }

    uint32_t load = gBuzzerSystemClock / frequency;
    if (load < 2U) {
        load = 2U;
    }

    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, load);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, load / 2U);
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
    PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, true);
}

static void Buzzer_Stop(void)
{
    PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, false);
    PWMGenDisable(PWM0_BASE, PWM_GEN_0);
}
