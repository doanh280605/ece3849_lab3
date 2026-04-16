#pragma once

#include <stdbool.h>
#include <stdint.h>

// Direction for snake movement
typedef enum { UP, DOWN, LEFT, RIGHT } Direction;

// High-level game state flags
typedef struct SnakeGameState {
    Direction currentDirection;
    bool isRunning;
    bool needsReset;
} SnakeGameState;

extern SnakeGameState gameState;

// Grid configuration (128x128 display, 8x8 cells)
#define GRID_SIZE 16
#define CELL_SIZE 8
#define MAX_LEN   256  // Maximum snake length (16x16 grid)

// Position on grid
typedef struct { uint8_t x, y; } Position;

// Snake representation
extern Position snake[MAX_LEN];
extern uint8_t snakeLength;

// Basic API
void ResetGame(void);
void moveSnake(void);
