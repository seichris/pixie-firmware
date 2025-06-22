#include <stdio.h>
#include <string.h>
#include <esp_random.h>
#include <esp_task_wdt.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "firefly-scene.h"
#include "firefly-crypto.h"
#include "firefly-address.h"

#include "panel.h"
#include "panel-wallet.h"
#include "qr-generator.h"

typedef struct WalletState {
    FfxScene scene;
    FfxNode nodeAddress1;
    FfxNode nodeAddress2;
    FfxNode nodeBackground;
    FfxNode nodeInstructions;
    FfxNode qrModules[QR_SIZE * QR_SIZE];  // QR code visual modules
    uint8_t privateKey[FFX_PRIVKEY_LENGTH];
    uint8_t publicKey[FFX_PUBKEY_LENGTH];
    uint8_t address[FFX_ADDRESS_LENGTH];
    char addressStr[FFX_ADDRESS_STRING_LENGTH];
    char addressLine1[25];
    char addressLine2[25];
    QRCode qrCode;
    bool showingQR;
} WalletState;

static void updateAddressDisplay(WalletState *state) {
    // Split address into two lines for better readability
    // Line 1: 0x + first 20 chars
    // Line 2: remaining 22 chars
    strncpy(state->addressLine1, state->addressStr, 22);
    state->addressLine1[22] = '\0';
    strncpy(state->addressLine2, state->addressStr + 22, 20);
    state->addressLine2[20] = '\0';
    
    ffx_sceneLabel_setText(state->nodeAddress1, state->addressLine1);
    ffx_sceneLabel_setText(state->nodeAddress2, state->addressLine2);
}

static void showQRCode(WalletState *state) {
    // Generate QR code for the address
    printf("[wallet] Generating QR for: %s\n", state->addressStr);
    bool qrSuccess = qr_generate(&state->qrCode, state->addressStr);
    printf("[wallet] QR generation result: %s (size=%d)\n", qrSuccess ? "SUCCESS" : "FAILED", state->qrCode.size);
    
    // Hide address text
    ffx_sceneNode_setPosition(state->nodeAddress1, (FfxPoint){ .x = -300, .y = 0 });
    ffx_sceneNode_setPosition(state->nodeAddress2, (FfxPoint){ .x = -300, .y = 0 });
    
    // Show QR modules (smaller size to fit screen properly)
    int moduleSize = 8;   // Even smaller 8x8 pixels per module for better fit
    int startX = 30;      // Center horizontally 
    int startY = 30;      // Center vertically
    
    printf("[wallet] Displaying QR modules at startX=%d, startY=%d\n", startX, startY);
    
    for (int y = 0; y < QR_SIZE; y++) {
        for (int x = 0; x < QR_SIZE; x++) {
            int moduleIndex = y * QR_SIZE + x;
            bool isBlack = qr_getModule(&state->qrCode, x, y);
            
            if (isBlack) {
                ffx_sceneBox_setColor(state->qrModules[moduleIndex], ffx_color_rgb(0, 0, 0));  // Black
                ffx_sceneNode_setPosition(state->qrModules[moduleIndex], (FfxPoint){
                    .x = startX + x * moduleSize,
                    .y = startY + y * moduleSize
                });
            } else {
                // White modules - show as white
                ffx_sceneBox_setColor(state->qrModules[moduleIndex], ffx_color_rgb(255, 255, 255));
                ffx_sceneNode_setPosition(state->qrModules[moduleIndex], (FfxPoint){
                    .x = startX + x * moduleSize,
                    .y = startY + y * moduleSize
                });
            }
        }
    }
    
    printf("[wallet] QR modules positioned successfully\n");
}

static void hideQRCode(WalletState *state) {
    // Hide all QR modules
    for (int i = 0; i < QR_SIZE * QR_SIZE; i++) {
        ffx_sceneNode_setPosition(state->qrModules[i], (FfxPoint){ .x = -300, .y = 0 });
    }
    
    // Show address text
    ffx_sceneNode_setPosition(state->nodeAddress1, (FfxPoint){ .x = 30, .y = 65 });
    ffx_sceneNode_setPosition(state->nodeAddress2, (FfxPoint){ .x = 30, .y = 90 });
}

static void keyChanged(EventPayload event, void *_state) {
    WalletState *state = _state;
    
    Keys keys = event.props.keys.down;
    printf("[wallet] keyChanged: keys=0x%04x, showingQR=%s\n", keys, state->showingQR ? "true" : "false");
    
    // Standardized controls:
    // Button 1 (KeyCancel) = Primary action (generate new address)
    // Button 2 (KeyOk) = Exit
    // Button 3 (KeyNorth) = Up/Right action (toggle QR)
    // Button 4 (KeySouth) = Down/Left action (unused)
    
    if (keys & KeyOk) {
        panel_pop();
        return;
    }
    
    if (keys & KeyCancel) {
        // Primary action - generate new wallet
        printf("[wallet] Starting key generation...\n");
        
        // Show loading message
        ffx_sceneLabel_setText(state->nodeAddress1, "Generating new");
        ffx_sceneLabel_setText(state->nodeAddress2, "address...");
        ffx_sceneLabel_setText(state->nodeInstructions, "Please wait...");
        
        // Disable watchdog for this task during crypto operations
        esp_task_wdt_delete(NULL);
        
        esp_fill_random(state->privateKey, FFX_PRIVKEY_LENGTH);
        
        // Yield frequently during crypto operations
        vTaskDelay(pdMS_TO_TICKS(50));
        
        printf("[wallet] Computing public key...\n");
        // Compute public key 
        if (!ffx_pk_computePubkeySecp256k1(state->privateKey, state->publicKey)) {
            printf("[wallet] Public key computation failed!\n");
            // Re-add to watchdog before returning
            esp_task_wdt_add(NULL);
            return;
        }
        
        // Yield after intensive operation
        vTaskDelay(pdMS_TO_TICKS(50));
        
        printf("[wallet] Computing address...\n");
        // Compute address
        ffx_eth_computeAddress(state->publicKey, state->address);
        
        // Yield after operation
        vTaskDelay(pdMS_TO_TICKS(50));
        
        printf("[wallet] Computing checksum...\n");
        // Get checksum address string
        ffx_eth_checksumAddress(state->address, state->addressStr);
        
        // Re-add to watchdog when done with crypto operations
        esp_task_wdt_add(NULL);
        
        printf("[wallet] Key generation complete!\n");
        
        // Update display - force back to address view
        state->showingQR = false;
        updateAddressDisplay(state);
        ffx_sceneLabel_setText(state->nodeInstructions, "Cancel=New  North=QR  Ok=Exit");
        return;
    }
    
    if (keys & KeyNorth) {
        // Up/Right action - toggle QR view
        if (!state->showingQR) {
            // Show QR code view
            state->showingQR = true;
            showQRCode(state);
            ffx_sceneLabel_setText(state->nodeInstructions, "Cancel=New  North=Back  Ok=Exit");
        } else {
            // Toggle back to address view
            state->showingQR = false;
            hideQRCode(state);
            ffx_sceneLabel_setText(state->nodeInstructions, "Cancel=New  North=QR  Ok=Exit");
        }
        return;
    }
}

static int init(FfxScene scene, FfxNode node, void* _state, void* arg) {
    WalletState *state = _state;
    state->scene = scene;
    
    // Create title
    FfxNode nodeTitle = ffx_scene_createLabel(scene, FfxFontLarge, "ETH Wallet");
    ffx_sceneGroup_appendChild(node, nodeTitle);
    ffx_sceneNode_setPosition(nodeTitle, (FfxPoint){ .x = 70, .y = 15 });
    
    // Create background for address text (larger and better positioned)
    state->nodeBackground = ffx_scene_createBox(scene, ffx_size(200, 120));
    ffx_sceneBox_setColor(state->nodeBackground, ffx_color_rgba(0, 0, 0, 200));
    ffx_sceneGroup_appendChild(node, state->nodeBackground);
    ffx_sceneNode_setPosition(state->nodeBackground, (FfxPoint){ .x = 20, .y = 50 });
    
    // Create address labels (split into two lines, properly centered)
    state->nodeAddress1 = ffx_scene_createLabel(scene, FfxFontMedium, "Press OK to");
    ffx_sceneGroup_appendChild(node, state->nodeAddress1);
    ffx_sceneNode_setPosition(state->nodeAddress1, (FfxPoint){ .x = 30, .y = 65 });
    
    state->nodeAddress2 = ffx_scene_createLabel(scene, FfxFontMedium, "generate wallet");
    ffx_sceneGroup_appendChild(node, state->nodeAddress2);
    ffx_sceneNode_setPosition(state->nodeAddress2, (FfxPoint){ .x = 30, .y = 90 });
    
    // Create instructions (positioned within background)
    state->nodeInstructions = ffx_scene_createLabel(scene, FfxFontSmall, "Cancel=New  North=QR  Ok=Exit");
    ffx_sceneGroup_appendChild(node, state->nodeInstructions);
    ffx_sceneNode_setPosition(state->nodeInstructions, (FfxPoint){ .x = 30, .y = 140 });
    
    // Create QR visual modules (smaller modules for better fit)
    for (int i = 0; i < QR_SIZE * QR_SIZE; i++) {
        state->qrModules[i] = ffx_scene_createBox(scene, ffx_size(8, 8));  // Smaller 8x8 modules
        ffx_sceneBox_setColor(state->qrModules[i], COLOR_BLACK);
        ffx_sceneGroup_appendChild(node, state->qrModules[i]);
        // Initially hide all modules off-screen
        ffx_sceneNode_setPosition(state->qrModules[i], (FfxPoint){ .x = -300, .y = 0 });
    }
    
    // Initialize state
    state->showingQR = false;
    
    // Generate initial wallet
    esp_fill_random(state->privateKey, FFX_PRIVKEY_LENGTH);
    
    if (ffx_pk_computePubkeySecp256k1(state->privateKey, state->publicKey)) {
        ffx_eth_computeAddress(state->publicKey, state->address);
        ffx_eth_checksumAddress(state->address, state->addressStr);
        updateAddressDisplay(state);
        printf("[wallet] Initial address: %s\n", state->addressStr);
    }
    
    // Register for key events (4 buttons: Cancel, Ok, North, South)
    panel_onEvent(EventNameKeysChanged | KeyCancel | KeyOk | KeyNorth | KeySouth, keyChanged, state);
    
    return 0;
}

void pushPanelWallet(void* arg) {
    panel_push(init, sizeof(WalletState), PanelStyleSlideLeft, arg);
}