// Shared application-level objects used across modules.
// Keep this header minimal to avoid tight coupling.

#pragma once

extern "C" {
#include "FreeRTOS.h"
#include "grlib/grlib.h"
}

#include <stdint.h>

// Global graphics context for the LCD driver
extern tContext gContext;

// System clock frequency (Hz) set during startup
extern uint32_t gSysClk;

// Add your synchronization primitives here as you build the lab
