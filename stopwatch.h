#ifndef STOPWATCH_H_
#define STOPWATCH_H_

#include <stdbool.h>
#include <stdint.h>

struct MyButton
{
    int x;
    int y;
    int w;
    int h;
    const char *label;
    bool pressed;
};

extern volatile bool gRunning;
extern volatile uint16_t g_ms;
extern volatile uint8_t g_sec;
extern volatile uint8_t g_min;
extern volatile uint8_t g_hr;

extern MyButton gStartButton;
extern MyButton gResetButton;

void Stopwatch_Init(void);
void TimeTask(void *pvParameters);
void ButtonTask(void *pvParameters);

#endif
