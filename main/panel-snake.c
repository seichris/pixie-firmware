#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "firefly-scene.h"
#include "firefly-color.h"
#include "panel.h"
#include "panel-snake.h"
#include "utils.h"

#define GRID_SIZE 12
#define GRID_WIDTH 20
#define GRID_HEIGHT 20
#define MAX_SNAKE_LENGTH 50

typedef enum Direction {
    DIR_UP = 0,
    DIR_RIGHT = 1,
    DIR_DOWN = 2,
    DIR_LEFT = 3
} Direction;

typedef struct Point {
    int x, y;
} Point;

typedef struct SnakeState {
    FfxScene scene;
    FfxNode gameArea;
    FfxNode snakeBody[MAX_SNAKE_LENGTH];
    FfxNode food;
    FfxNode scoreLabel;
    
    Point snake[MAX_SNAKE_LENGTH];
    int snakeLength;
    Direction direction;
    Direction nextDirection;
    Point foodPos;
    int score;
    bool gameOver;
    bool paused;
    uint32_t lastMove;
    char scoreText[32];
    
    Keys currentKeys;
    uint32_t southHoldStart;
} SnakeState;

static void spawnFood(SnakeState *state) {
    do {
        state->foodPos.x = rand() % GRID_WIDTH;
        state->foodPos.y = rand() % GRID_HEIGHT;
        
        // Make sure food doesn't spawn on snake
        bool onSnake = false;
        for (int i = 0; i < state->snakeLength; i++) {
            if (state->snake[i].x == state->foodPos.x && state->snake[i].y == state->foodPos.y) {
                onSnake = true;
                break;
            }
        }
        if (!onSnake) break;
    } while (true);
    
    ffx_sceneNode_setPosition(state->food, (FfxPoint){
        .x = state->foodPos.x * GRID_SIZE,
        .y = state->foodPos.y * GRID_SIZE
    });
}

static bool checkCollision(SnakeState *state) {
    Point head = state->snake[0];
    
    // Wall collision
    if (head.x < 0 || head.x >= GRID_WIDTH || head.y < 0 || head.y >= GRID_HEIGHT) {
        return true;
    }
    
    // Self collision
    for (int i = 1; i < state->snakeLength; i++) {
        if (state->snake[i].x == head.x && state->snake[i].y == head.y) {
            return true;
        }
    }
    
    return false;
}

static void moveSnake(SnakeState *state) {
    if (state->gameOver || state->paused) return;
    
    // Update direction
    state->direction = state->nextDirection;
    
    // Move body
    for (int i = state->snakeLength - 1; i > 0; i--) {
        state->snake[i] = state->snake[i-1];
    }
    
    // Move head
    switch (state->direction) {
        case DIR_UP:    state->snake[0].y--; break;
        case DIR_DOWN:  state->snake[0].y++; break;
        case DIR_LEFT:  state->snake[0].x--; break;
        case DIR_RIGHT: state->snake[0].x++; break;
    }
    
    // Check collision
    if (checkCollision(state)) {
        state->gameOver = true;
        return;
    }
    
    // Check food
    if (state->snake[0].x == state->foodPos.x && state->snake[0].y == state->foodPos.y) {
        state->score += 10;
        state->snakeLength++;
        spawnFood(state);
        
        snprintf(state->scoreText, sizeof(state->scoreText), "Score: %d", state->score);
        ffx_sceneLabel_setText(state->scoreLabel, state->scoreText);
    }
    
    // Update visual positions
    for (int i = 0; i < state->snakeLength; i++) {
        ffx_sceneNode_setPosition(state->snakeBody[i], (FfxPoint){
            .x = state->snake[i].x * GRID_SIZE,
            .y = state->snake[i].y * GRID_SIZE
        });
    }
    
    // Hide unused body segments
    for (int i = state->snakeLength; i < MAX_SNAKE_LENGTH; i++) {
        ffx_sceneNode_setPosition(state->snakeBody[i], (FfxPoint){ .x = -100, .y = -100 });
    }
}

static void keyChanged(EventPayload event, void *_state) {
    printf("[snake] keyChanged called!\n");
    SnakeState *state = _state;
    
    // Update current keys for continuous movement
    state->currentKeys = event.props.keys.down;
    
    Keys keys = event.props.keys.down;
    
    // Standardized controls:
    // Button 1 (KeyCancel) = Primary action (rotate direction)
    // Button 2 (KeyOk) = Pause/Exit (hold 1s)
    // Button 3 (KeyNorth) = Up/Right movement (90° counter-clockwise like Le Space)  
    // Button 4 (KeySouth) = Down/Left movement
    
    static uint32_t okHoldStart = 0;
    
    // Handle Ok button hold-to-exit, short press for pause
    if (event.props.keys.down & KeyOk) {
        if (okHoldStart == 0) {
            okHoldStart = ticks();
        }
    } else {
        if (okHoldStart > 0) {
            uint32_t holdDuration = ticks() - okHoldStart;
            if (holdDuration > 1000) { // 1 second hold
                panel_pop();
                return;
            } else {
                // Short press - pause/unpause
                if (!state->gameOver) {
                    state->paused = !state->paused;
                }
            }
            okHoldStart = 0;
        }
    }
    
    if (state->gameOver) {
        if (event.props.keys.down & KeyCancel) {
            // Reset game with Cancel button
            state->snakeLength = 3;
            state->snake[0] = (Point){10, 10}; // Head in center
            state->snake[1] = (Point){9, 10};  // Body to the left  
            state->snake[2] = (Point){8, 10};  // Tail further left
            state->direction = DIR_RIGHT;      // Initially moving right
            state->nextDirection = DIR_RIGHT;
            state->score = 0;
            state->gameOver = false;
            state->paused = false;
            spawnFood(state);
            snprintf(state->scoreText, sizeof(state->scoreText), "Score: %d", state->score);
            ffx_sceneLabel_setText(state->scoreLabel, state->scoreText);
            
            // Reset visual positions for snake segments
            for (int i = 0; i < state->snakeLength; i++) {
                ffx_sceneNode_setPosition(state->snakeBody[i], (FfxPoint){
                    .x = state->snake[i].x * GRID_SIZE,
                    .y = state->snake[i].y * GRID_SIZE
                });
            }
        }
        return;
    }
    
    if (state->paused) return;
    
    // 90° counter-clockwise directional controls (like Le Space)
    // Button 3 (North) = Up/Right movement 
    if (event.props.keys.down & KeyNorth) {
        if (state->direction == DIR_UP || state->direction == DIR_DOWN) {
            // Currently moving vertically, change to right
            if (state->direction != DIR_LEFT) state->nextDirection = DIR_RIGHT;
        } else {
            // Currently moving horizontally, change to up
            if (state->direction != DIR_DOWN) state->nextDirection = DIR_UP;
        }
    }
    
    // Button 4 (South) = Down/Left movement
    if (event.props.keys.down & KeySouth) {
        if (state->direction == DIR_UP || state->direction == DIR_DOWN) {
            // Currently moving vertically, change to left
            if (state->direction != DIR_RIGHT) state->nextDirection = DIR_LEFT;
        } else {
            // Currently moving horizontally, change to down  
            if (state->direction != DIR_UP) state->nextDirection = DIR_DOWN;
        }
    }
    
    // Button 1 (Cancel) = Primary action (rotate direction clockwise)
    if (event.props.keys.down & KeyCancel) {
        switch (state->direction) {
            case DIR_UP:
                if (state->direction != DIR_DOWN) state->nextDirection = DIR_RIGHT;
                break;
            case DIR_RIGHT:
                if (state->direction != DIR_LEFT) state->nextDirection = DIR_DOWN;
                break;
            case DIR_DOWN:
                if (state->direction != DIR_UP) state->nextDirection = DIR_LEFT;
                break;
            case DIR_LEFT:
                if (state->direction != DIR_RIGHT) state->nextDirection = DIR_UP;
                break;
        }
    }
}

static void render(EventPayload event, void *_state) {
    SnakeState *state = _state;
    
    uint32_t now = ticks();
    
    // Check for hold-to-exit during gameplay
    if (state->southHoldStart > 0 && (now - state->southHoldStart) > 1000) {
        panel_pop();
        return;
    }
    
    if (now - state->lastMove > 150) { // Move every 150ms
        moveSnake(state);
        state->lastMove = now;
    }
}

static int init(FfxScene scene, FfxNode node, void* _state, void* arg) {
    SnakeState *state = _state;
    state->scene = scene;
    
    // Create game area background
    state->gameArea = ffx_scene_createBox(scene, ffx_size(240, 240));
    ffx_sceneBox_setColor(state->gameArea, COLOR_BLACK);
    ffx_sceneGroup_appendChild(node, state->gameArea);
    ffx_sceneNode_setPosition(state->gameArea, (FfxPoint){ .x = 0, .y = 0 });
    
    // Create score label
    state->scoreLabel = ffx_scene_createLabel(scene, FfxFontMedium, "Score: 0");
    ffx_sceneGroup_appendChild(node, state->scoreLabel);
    ffx_sceneNode_setPosition(state->scoreLabel, (FfxPoint){ .x = 10, .y = 10 });
    
    // Create snake body segments
    for (int i = 0; i < MAX_SNAKE_LENGTH; i++) {
        state->snakeBody[i] = ffx_scene_createBox(scene, ffx_size(GRID_SIZE-1, GRID_SIZE-1));
        ffx_sceneBox_setColor(state->snakeBody[i], ffx_color_rgb(0, 255, 0));
        ffx_sceneGroup_appendChild(node, state->snakeBody[i]);
        ffx_sceneNode_setPosition(state->snakeBody[i], (FfxPoint){ .x = -100, .y = -100 });
    }
    
    // Create food
    state->food = ffx_scene_createBox(scene, ffx_size(GRID_SIZE-1, GRID_SIZE-1));
    ffx_sceneBox_setColor(state->food, ffx_color_rgb(255, 0, 0));
    ffx_sceneGroup_appendChild(node, state->food);
    
    // Initialize game state - start in center
    state->snakeLength = 3;
    state->snake[0] = (Point){10, 10}; // Head in center
    state->snake[1] = (Point){9, 10};  // Body to the left
    state->snake[2] = (Point){8, 10};  // Tail further left
    state->direction = DIR_RIGHT;      // Initially moving right
    state->nextDirection = DIR_RIGHT;
    state->score = 0;
    state->gameOver = false;
    state->paused = false;
    state->lastMove = ticks();
    state->currentKeys = 0;
    state->southHoldStart = 0;
    
    spawnFood(state);
    snprintf(state->scoreText, sizeof(state->scoreText), "Score: %d", state->score);
    ffx_sceneLabel_setText(state->scoreLabel, state->scoreText);
    
    // Set initial visual positions for snake segments
    for (int i = 0; i < state->snakeLength; i++) {
        ffx_sceneNode_setPosition(state->snakeBody[i], (FfxPoint){
            .x = state->snake[i].x * GRID_SIZE,
            .y = state->snake[i].y * GRID_SIZE
        });
    }
    
    // Register events (4 buttons: Cancel, Ok, North, South)
    panel_onEvent(EventNameKeysChanged | KeyCancel | KeyOk | KeyNorth | KeySouth, keyChanged, state);
    panel_onEvent(EventNameRenderScene, render, state);
    
    return 0;
}

void pushPanelSnake(void* arg) {
    panel_push(init, sizeof(SnakeState), PanelStyleSlideLeft, arg);
}