#ifndef MIC_H
#define MIC_H

#include <stdint.h>

extern volatile float gMicRMS;    // raw RMS amplitude [0, 0.5]
extern volatile float gMicDb;     // loudness in dB    [-60, 0]
extern volatile float gMicLevel;  // normalized level  [0, 1] for display

void     Mic_Init(void);
uint16_t Mic_Read(void);
void     ScreenMic_Init(void);
void     MicTask(void *pvParameters);

#endif
