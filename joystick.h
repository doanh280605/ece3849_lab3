#ifndef JOYSTICK_TASK_H
#define JOYSTICK_TASK_H

#include <stdint.h>

enum ScreenID : uint8_t {
    SCREEN_STOPWATCH = 0,
    SCREEN_MIC,
    SCREEN_COUNT
};

extern volatile uint8_t gCurrentScreen;

void JoystickTask(void *pvParameters);

#endif
