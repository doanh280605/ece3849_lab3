#include "joystick.h"
#include "../libraries/joystickDriver/joystick.h"

extern "C" {
#include "FreeRTOS.h"
#include "task.h"
}

void JoystickTask(void *pvParameters)
{
    (void)pvParameters;

    static Joystick js(JSX, JSY, JS1);
    js.begin();
    js.calibrateCenter(32);

    bool readyForNextMove = true;

    for (;;) {
        js.tick();

        JoystickDir dir = js.direction8();

        if (dir == JoystickDir::Center) {
            readyForNextMove = true;
        } else if (readyForNextMove &&
                   (dir == JoystickDir::E ||
                    dir == JoystickDir::NE ||
                    dir == JoystickDir::SE)) {
            gCurrentScreen = (gCurrentScreen + 1u < SCREEN_COUNT)
                             ? (gCurrentScreen + 1u) : 0u;
            readyForNextMove = false;
        } else if (readyForNextMove &&
                   (dir == JoystickDir::W ||
                    dir == JoystickDir::NW ||
                    dir == JoystickDir::SW)) {
            gCurrentScreen = (gCurrentScreen > 0u)
                             ? (gCurrentScreen - 1u) : (SCREEN_COUNT - 1u);
            readyForNextMove = false;
        }

        vTaskDelay(pdMS_TO_TICKS(30));
    }
}
