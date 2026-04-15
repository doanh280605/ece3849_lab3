#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "buzzer.h"
#include "grlib/grlib.h"
#include "joystick.h"
#include "mic.h"
#include "screen_mic.h"
#include "stopwatch.h"

extern "C" {
#include "driverlib/fpu.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "../libraries/display/Crystalfontz128x128_ST7735.h"

#include "FreeRTOS.h"
#include "task.h"
}

volatile uint8_t gCurrentScreen = SCREEN_STOPWATCH;

tContext gContext;
static bool gDisplayInitialized = false;

static void initializeDisplay(tContext &context);
static void updateTimeText(tContext &context, const char *oldTime, const char *newTime, bool running);
static void updateStateText(tContext &context, bool oldRunning, bool newRunning);
static void drawButton(tContext &context, const MyButton &btn);
void DrawHeader(const char *title);
static void Stopwatch_Draw(void);
static void Stopwatch_DrawReset(void);

static void initializeDisplay(tContext &context)
{
    Crystalfontz128x128_Init();
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);
    GrContextInit(&context, &g_sCrystalfontz128x128);
    GrContextFontSet(&context, &g_sFontFixed6x8);

    tRectangle full = {0, 0, 127, 127};
    GrContextForegroundSet(&context, ClrBlack);
    GrRectFill(&context, &full);
    gDisplayInitialized = true;
}

void DrawHeader(const char *title)
{
    tRectangle header = {0, 0, 127, 20};
    GrContextForegroundSet(&gContext, ClrDarkBlue);
    GrRectFill(&gContext, &header);

    GrContextForegroundSet(&gContext, ClrYellow);
    GrStringDraw(&gContext, "<", -1, 4, 6, false);
    GrStringDraw(&gContext, ">", -1, 118, 6, false);

    GrContextForegroundSet(&gContext, ClrWhite);
    GrStringDrawCentered(&gContext, title, -1, 64, 10, false);
}

static void updateTimeText(tContext &context, const char *oldTime, const char *newTime, bool running)
{
    GrContextForegroundSet(&context, ClrBlack);
    GrStringDrawCentered(&context, oldTime, -1, 64, 45, false);

    GrContextForegroundSet(&context, running ? ClrYellow : ClrOlive);
    GrStringDrawCentered(&context, newTime, -1, 64, 45, false);
}

static void updateStateText(tContext &context, bool oldRunning, bool newRunning)
{
    GrContextForegroundSet(&context, ClrBlack);
    GrStringDrawCentered(&context, oldRunning ? "Running" : "Stopped", -1, 64, 65, false);

    GrContextForegroundSet(&context, newRunning ? ClrGreen : ClrRed);
    GrStringDrawCentered(&context, newRunning ? "Running" : "Stopped", -1, 64, 65, false);
}

static void drawButton(tContext &context, const MyButton &btn)
{
    uint16_t bgColor = btn.pressed ? ClrBlack : ClrGray;
    uint16_t textColor = btn.pressed ? ClrWhite : ClrBlack;

    tRectangle rect = {btn.x, btn.y, btn.x + btn.w - 1, btn.y + btn.h - 1};
    GrContextForegroundSet(&context, bgColor);
    GrRectFill(&context, &rect);

    GrContextForegroundSet(&context, ClrBlack);
    GrRectDraw(&context, &rect);

    GrContextForegroundSet(&context, textColor);
    GrStringDrawCentered(&context, btn.label, -1, btn.x + btn.w / 2, btn.y + btn.h / 2, false);
}

// File-scope state for Stopwatch_Draw so it can be reset on screen transitions
static char        sw_lastTimeStr[20]   = "";
static bool        sw_lastRunning       = false;
static const char *sw_lastStartLabel    = "";
static bool        sw_lastStartPressed  = false;
static bool        sw_lastResetPressed  = false;

static void Stopwatch_DrawReset(void)
{
    sw_lastTimeStr[0]   = '\0';
    sw_lastRunning      = !gRunning;
    sw_lastStartLabel   = "";
    sw_lastStartPressed = !gStartButton.pressed;
    sw_lastResetPressed = !gResetButton.pressed;
}

static void Stopwatch_Draw(void)
{
    DrawHeader("STOPWATCH");

    char timeStr[20];
    snprintf(timeStr, sizeof(timeStr), "%02u:%02u:%02u:%03u",
             g_hr, g_min, g_sec, g_ms);

    gStartButton.label = gRunning ? "PAUSE" : "PLAY";

    if (strcmp(timeStr, sw_lastTimeStr) != 0) {
        updateTimeText(gContext, sw_lastTimeStr, timeStr, gRunning);
        strcpy(sw_lastTimeStr, timeStr);
    }

    if (gRunning != sw_lastRunning) {
        updateStateText(gContext, sw_lastRunning, gRunning);
        if (strcmp(timeStr, sw_lastTimeStr) == 0) {
            updateTimeText(gContext, sw_lastTimeStr, timeStr, gRunning);
        }
        sw_lastRunning = gRunning;
    }

    if (gStartButton.label != sw_lastStartLabel || gStartButton.pressed != sw_lastStartPressed) {
        drawButton(gContext, gStartButton);
        sw_lastStartLabel   = gStartButton.label;
        sw_lastStartPressed = gStartButton.pressed;
    }

    if (gResetButton.pressed != sw_lastResetPressed) {
        drawButton(gContext, gResetButton);
        sw_lastResetPressed = gResetButton.pressed;
    }
}


void DisplayTask(void *pvParameters)
{
    (void)pvParameters;

    uint8_t lastScreen = gCurrentScreen;

    for (;;) {
        if (gDisplayInitialized) {
            // On screen transition: clear content area and reset draw state
            if (gCurrentScreen != lastScreen) {
                tRectangle content = {0, 21, 127, 127};
                GrContextForegroundSet(&gContext, ClrBlack);
                GrRectFill(&gContext, &content);

                if (gCurrentScreen == SCREEN_STOPWATCH) {
                    Stopwatch_DrawReset();
                }

                lastScreen = gCurrentScreen;
            }

            switch (gCurrentScreen) {
                case SCREEN_STOPWATCH: Stopwatch_Draw(); break;
                case SCREEN_MIC:       ScreenMic_Draw(); break;
                default:               break;
            }

#ifdef GrFlush
            GrFlush(&gContext);
#endif
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main(void)
{
    uint32_t systemClock = 0;

    IntMasterDisable();

    FPUEnable();
    FPULazyStackingEnable();

    systemClock = SysCtlClockFreqSet(
        (SYSCTL_XTAL_25MHZ |
         SYSCTL_OSC_MAIN |
         SYSCTL_USE_PLL |
         SYSCTL_CFG_VCO_480),
        120000000);

    Stopwatch_Init();
    ScreenMic_Init();
    initializeDisplay(gContext);
    updateTimeText(gContext, "", "00:00:00:000", gRunning);
    updateStateText(gContext, true, gRunning);
    drawButton(gContext, gStartButton);
    drawButton(gContext, gResetButton);

#ifdef GrFlush
    GrFlush(&gContext);
#endif

    Buzzer_Init(systemClock);

    xTaskCreate(TimeTask,     "Time",     128, NULL, tskIDLE_PRIORITY + 3, NULL);
    xTaskCreate(DisplayTask,  "Display",  512, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(ButtonTask,   "Button",   128, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(BuzzerTask,   "Buzzer",   128, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(JoystickTask, "Joystick", 256, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(MicTask,      "Mic",      256, NULL, tskIDLE_PRIORITY + 1, NULL);

    IntMasterEnable();
    vTaskStartScheduler();

    while (1) {
    }
}
