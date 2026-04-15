#include "screen_mic.h"
#include "mic.h"

#include <stdio.h>
#include "grlib/grlib.h"

// gContext is defined in main.cpp (non-static so other files can use it)
extern tContext gContext;

// DrawHeader is defined in main.cpp
extern void DrawHeader(const char *title);

// Bar layout (content area starts at y=21)
#define BAR_X    44     // left edge of bar
#define BAR_W    40     // bar width in pixels
#define BAR_TOP  28     // y when bar is completely full
#define BAR_BOT  105    // y bottom of bar (always drawn here)
#define BAR_H    (BAR_BOT - BAR_TOP)   // 77 pixels total height

void ScreenMic_Draw(void)
{
    DrawHeader("MIC");

    float level = gMicLevel;   // 0.0 to 1.0
    float db    = gMicDb;      // -60.0 to 0.0

    // Choose bar color based on loudness level
    uint32_t barColor;
    if      (level < 0.40f) barColor = ClrGreen;
    else if (level < 0.75f) barColor = ClrYellow;
    else                    barColor = ClrRed;

    // How many pixels to fill from the bottom
    int fillH   = (int)(level * (float)BAR_H);
    int fillTop = BAR_BOT - fillH;

    // Draw the colored filled portion (bottom section)
    if (fillH > 0) {
        tRectangle filled = {(int16_t)BAR_X,
                             (int16_t)fillTop,
                             (int16_t)(BAR_X + BAR_W),
                             (int16_t)BAR_BOT};
        GrContextForegroundSet(&gContext, barColor);
        GrRectFill(&gContext, &filled);
    }

    // Draw the empty (black) portion above the fill
    if (fillTop > BAR_TOP) {
        tRectangle empty = {(int16_t)BAR_X,
                            (int16_t)BAR_TOP,
                            (int16_t)(BAR_X + BAR_W),
                            (int16_t)(fillTop - 1)};
        GrContextForegroundSet(&gContext, ClrBlack);
        GrRectFill(&gContext, &empty);
    }

    // Draw the bar outline so it's always visible even when empty
    tRectangle outline = {(int16_t)BAR_X,
                          (int16_t)BAR_TOP,
                          (int16_t)(BAR_X + BAR_W),
                          (int16_t)BAR_BOT};
    GrContextForegroundSet(&gContext, ClrWhite);
    GrRectDraw(&gContext, &outline);

    // Draw dB readout below the bar
    char dbStr[16];
    snprintf(dbStr, sizeof(dbStr), "%.1f dB", db);
    GrContextForegroundSet(&gContext, ClrWhite);
    GrStringDrawCentered(&gContext, dbStr, -1, 64, 118, true);
}
