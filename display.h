#pragma once

// Forward declaration for game state (defined in game.h).
struct SnakeGameState;

// Initialize LCD controller and the grlib drawing context.
void LCD_Init(void);

// Draw the full game frame: for now just background and snake.
void DrawGame(const SnakeGameState* state);
