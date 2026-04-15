#ifndef BUZZER_H_
#define BUZZER_H_

#include <stdbool.h>
#include <stdint.h>

extern "C" {
#include "FreeRTOS.h"
#include "queue.h"
}

typedef struct {
    uint32_t frequency;
    uint32_t duration;
} BuzzerCmd;

extern QueueHandle_t gBuzzerQ;

void Buzzer_Init(uint32_t systemClock);
void BuzzerTask(void *pvParameters);
bool Buzzer_Beep(uint32_t frequency, uint32_t duration);

#endif
