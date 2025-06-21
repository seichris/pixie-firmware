#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "firefly-scene.h"
#include "firefly-color.h"
#include "panel.h"
#include "panel-pong.h"
#include "utils.h"

#define PADDLE_WIDTH 4
#define PADDLE_HEIGHT 30
#define BALL_SIZE 6
#define GAME_WIDTH 240
#define GAME_HEIGHT 240
#define PADDLE_SPEED 3
#define BALL_SPEED 2

typedef struct PongState {
    FfxScene scene;
    FfxNode gameArea;
    FfxNode playerPaddle;  // Bottom paddle (player)
    FfxNode aiPaddle;      // Top paddle (AI)
    FfxNode ball;
    FfxNode scoreLabel;
    FfxNode centerLine;
    
    // Game state
    float playerPaddleX;   // Player paddle X position
    float aiPaddleX;       // AI paddle X position
    float ballX, ballY;
    float ballVelX, ballVelY;
    int playerScore, aiScore;
    bool gameOver;
    bool paused;
    Keys keys;
    uint32_t southHoldStart;
    char scoreText[32];
} PongState;

static void resetBall(PongState *state) {
    state->ballX = GAME_WIDTH / 2;
    state->ballY = GAME_HEIGHT / 2;
    
    // Random direction but not too steep for vertical play
    float angle = (rand() % 60 - 30) * M_PI / 180.0; // -30 to +30 degrees
    angle += M_PI / 2; // Make it primarily vertical
    if (rand() % 2) angle += M_PI; // Sometimes go up instead of down
    
    state->ballVelX = BALL_SPEED * cos(angle);
    state->ballVelY = BALL_SPEED * sin(angle);
}

static void updateGame(PongState *state) {
    if (state->paused || state->gameOver) return;
    
    // Move player paddle based on keys (East = Right, West = Left)
    if (state->keys & KeyEast) {
        state->playerPaddleX += PADDLE_SPEED;
        if (state->playerPaddleX > GAME_WIDTH - PADDLE_HEIGHT) {
            state->playerPaddleX = GAME_WIDTH - PADDLE_HEIGHT;
        }
    }
    if (state->keys & KeyWest) {
        state->playerPaddleX -= PADDLE_SPEED;
        if (state->playerPaddleX < 0) state->playerPaddleX = 0;
    }
    
    // Simple AI for top paddle
    float paddleCenter = state->aiPaddleX + PADDLE_HEIGHT / 2;
    float ballCenter = state->ballX + BALL_SIZE / 2;
    
    if (paddleCenter < ballCenter - 5) {
        state->aiPaddleX += PADDLE_SPEED * 0.8; // AI slightly slower
        if (state->aiPaddleX > GAME_WIDTH - PADDLE_HEIGHT) {
            state->aiPaddleX = GAME_WIDTH - PADDLE_HEIGHT;
        }
    } else if (paddleCenter > ballCenter + 5) {
        state->aiPaddleX -= PADDLE_SPEED * 0.8;
        if (state->aiPaddleX < 0) state->aiPaddleX = 0;
    }
    
    // Move ball
    state->ballX += state->ballVelX;
    state->ballY += state->ballVelY;
    
    // Ball collision with left/right walls
    if (state->ballX <= 0 || state->ballX >= GAME_WIDTH - BALL_SIZE) {
        state->ballVelX = -state->ballVelX;
        if (state->ballX <= 0) state->ballX = 0;
        if (state->ballX >= GAME_WIDTH - BALL_SIZE) state->ballX = GAME_WIDTH - BALL_SIZE;
    }
    
    // Ball collision with player paddle (bottom)
    if (state->ballY + BALL_SIZE >= GAME_HEIGHT - PADDLE_WIDTH && 
        state->ballX + BALL_SIZE >= state->playerPaddleX && 
        state->ballX <= state->playerPaddleX + PADDLE_HEIGHT) {
        
        state->ballVelY = -state->ballVelY;
        state->ballY = GAME_HEIGHT - PADDLE_WIDTH - BALL_SIZE;
        
        // Add some english based on where ball hits paddle
        float hitPos = (state->ballX + BALL_SIZE/2 - state->playerPaddleX - PADDLE_HEIGHT/2) / (PADDLE_HEIGHT/2);
        state->ballVelX += hitPos * 0.5;
        
        // Limit ball speed
        if (fabs(state->ballVelX) > BALL_SPEED * 1.5) {
            state->ballVelX = (state->ballVelX > 0) ? BALL_SPEED * 1.5 : -BALL_SPEED * 1.5;
        }
    }
    
    // Ball collision with AI paddle (top)
    if (state->ballY <= PADDLE_WIDTH && 
        state->ballX + BALL_SIZE >= state->aiPaddleX && 
        state->ballX <= state->aiPaddleX + PADDLE_HEIGHT) {
        
        state->ballVelY = -state->ballVelY;
        state->ballY = PADDLE_WIDTH;
        
        // Add some english
        float hitPos = (state->ballX + BALL_SIZE/2 - state->aiPaddleX - PADDLE_HEIGHT/2) / (PADDLE_HEIGHT/2);
        state->ballVelX += hitPos * 0.5;
        
        // Limit ball speed
        if (fabs(state->ballVelX) > BALL_SPEED * 1.5) {
            state->ballVelX = (state->ballVelX > 0) ? BALL_SPEED * 1.5 : -BALL_SPEED * 1.5;
        }
    }
    
    // Ball goes off top/bottom edges (scoring)
    if (state->ballY < -BALL_SIZE) {
        state->playerScore++;
        resetBall(state);
        snprintf(state->scoreText, sizeof(state->scoreText), "Player %d - AI %d", state->playerScore, state->aiScore);
        ffx_sceneLabel_setText(state->scoreLabel, state->scoreText);
        
        if (state->playerScore >= 7) {
            state->gameOver = true;
        }
    } else if (state->ballY > GAME_HEIGHT) {
        state->aiScore++;
        resetBall(state);
        snprintf(state->scoreText, sizeof(state->scoreText), "Player %d - AI %d", state->playerScore, state->aiScore);
        ffx_sceneLabel_setText(state->scoreLabel, state->scoreText);
        
        if (state->aiScore >= 7) {
            state->gameOver = true;
        }
    }
}

static void updateVisuals(PongState *state) {
    // Player paddle at bottom
    ffx_sceneNode_setPosition(state->playerPaddle, (FfxPoint){ 
        .x = (int)state->playerPaddleX, 
        .y = GAME_HEIGHT - PADDLE_WIDTH
    });
    
    // AI paddle at top
    ffx_sceneNode_setPosition(state->aiPaddle, (FfxPoint){ 
        .x = (int)state->aiPaddleX, 
        .y = 0
    });
    
    ffx_sceneNode_setPosition(state->ball, (FfxPoint){ 
        .x = (int)state->ballX, 
        .y = (int)state->ballY 
    });
}

static void keyChanged(EventPayload event, void *_state) {
    PongState *state = _state;
    
    // Handle South button hold-to-exit
    if (event.props.keys.down & KeySouth) {
        if (state->southHoldStart == 0) {
            state->southHoldStart = ticks();
        }
    } else {
        // South button released
        if (state->southHoldStart > 0) {
            uint32_t holdDuration = ticks() - state->southHoldStart;
            if (holdDuration > 1000) { // 1 second hold
                panel_pop();
                return;
            } else {
                // Short press - pause/unpause
                if (!state->gameOver) {
                    state->paused = !state->paused;
                }
            }
            state->southHoldStart = 0;
        }
    }
    
    if (state->gameOver) {
        if (event.props.keys.down & KeyNorth) {
            // Reset game with North button
            state->playerScore = 0;
            state->aiScore = 0;
            state->playerPaddleX = GAME_WIDTH / 2 - PADDLE_HEIGHT / 2;
            state->aiPaddleX = GAME_WIDTH / 2 - PADDLE_HEIGHT / 2;
            state->gameOver = false;
            state->paused = false;
            resetBall(state);
            snprintf(state->scoreText, sizeof(state->scoreText), "Player %d - AI %d", state->playerScore, state->aiScore);
            ffx_sceneLabel_setText(state->scoreLabel, state->scoreText);
        }
        return;
    }
    
    // Store key state for continuous movement (East=Right, West=Left)
    state->keys = event.props.keys.down;
}

static void render(EventPayload event, void *_state) {
    PongState *state = _state;
    
    uint32_t now = ticks();
    
    // Check for hold-to-exit during gameplay
    if (state->southHoldStart > 0 && (now - state->southHoldStart) > 1000) {
        panel_pop();
        return;
    }
    
    updateGame(state);
    updateVisuals(state);
}

static int init(FfxScene scene, FfxNode node, void* _state, void* arg) {
    PongState *state = _state;
    state->scene = scene;
    
    // Create game area background
    state->gameArea = ffx_scene_createBox(scene, ffx_size(GAME_WIDTH, GAME_HEIGHT));
    ffx_sceneBox_setColor(state->gameArea, COLOR_BLACK);
    ffx_sceneGroup_appendChild(node, state->gameArea);
    ffx_sceneNode_setPosition(state->gameArea, (FfxPoint){ .x = 0, .y = 0 });
    
    // Create center line (horizontal for vertical play)
    state->centerLine = ffx_scene_createBox(scene, ffx_size(GAME_WIDTH, 2));
    ffx_sceneBox_setColor(state->centerLine, ffx_color_rgb(128, 128, 128));
    ffx_sceneGroup_appendChild(node, state->centerLine);
    ffx_sceneNode_setPosition(state->centerLine, (FfxPoint){ .x = 0, .y = GAME_HEIGHT/2 - 1 });
    
    // Create score label
    state->scoreLabel = ffx_scene_createLabel(scene, FfxFontMedium, "Player 0 - AI 0");
    ffx_sceneGroup_appendChild(node, state->scoreLabel);
    ffx_sceneNode_setPosition(state->scoreLabel, (FfxPoint){ .x = 60, .y = GAME_HEIGHT/2 - 10 });
    
    // Create paddles (horizontal for vertical play)
    // Player paddle (bottom)
    state->playerPaddle = ffx_scene_createBox(scene, ffx_size(PADDLE_HEIGHT, PADDLE_WIDTH));
    ffx_sceneBox_setColor(state->playerPaddle, ffx_color_rgb(255, 255, 255));
    ffx_sceneGroup_appendChild(node, state->playerPaddle);
    
    // AI paddle (top)
    state->aiPaddle = ffx_scene_createBox(scene, ffx_size(PADDLE_HEIGHT, PADDLE_WIDTH));
    ffx_sceneBox_setColor(state->aiPaddle, ffx_color_rgb(255, 255, 255));
    ffx_sceneGroup_appendChild(node, state->aiPaddle);
    
    // Create ball
    state->ball = ffx_scene_createBox(scene, ffx_size(BALL_SIZE, BALL_SIZE));
    ffx_sceneBox_setColor(state->ball, ffx_color_rgb(255, 255, 255));
    ffx_sceneGroup_appendChild(node, state->ball);
    
    // Initialize game state
    state->playerScore = 0;
    state->aiScore = 0;
    state->playerPaddleX = GAME_WIDTH / 2 - PADDLE_HEIGHT / 2;
    state->aiPaddleX = GAME_WIDTH / 2 - PADDLE_HEIGHT / 2;
    state->gameOver = false;
    state->paused = false;
    state->keys = 0;
    state->southHoldStart = 0;
    
    resetBall(state);
    snprintf(state->scoreText, sizeof(state->scoreText), "Player %d - AI %d", state->playerScore, state->aiScore);
    ffx_sceneLabel_setText(state->scoreLabel, state->scoreText);
    
    // Register events
    panel_onEvent(EventNameKeysChanged | KeyNorth | KeySouth | KeyEast | KeyWest, keyChanged, state);
    panel_onEvent(EventNameRenderScene, render, state);
    
    return 0;
}

void pushPanelPong(void* arg) {
    panel_push(init, sizeof(PongState), PanelStyleSlideLeft, arg);
}