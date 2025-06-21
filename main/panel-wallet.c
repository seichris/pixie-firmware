#include <stdio.h>
#include <string.h>
#include <esp_random.h>

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
    qr_generate(&state->qrCode, state->addressStr);
    
    // Hide address text
    ffx_sceneNode_setPosition(state->nodeAddress1, (FfxPoint){ .x = -300, .y = 0 });
    ffx_sceneNode_setPosition(state->nodeAddress2, (FfxPoint){ .x = -300, .y = 0 });
    
    // Show QR modules
    int moduleSize = 4;  // 4x4 pixels per module
    int startX = 50;     // Center QR code
    int startY = 60;
    
    for (int y = 0; y < QR_SIZE; y++) {
        for (int x = 0; x < QR_SIZE; x++) {
            int moduleIndex = y * QR_SIZE + x;
            bool isBlack = qr_getModule(&state->qrCode, x, y);
            
            if (isBlack) {
                ffx_sceneNode_setPosition(state->qrModules[moduleIndex], (FfxPoint){
                    .x = startX + x * moduleSize,
                    .y = startY + y * moduleSize
                });
            } else {
                // Hide white modules  
                ffx_sceneNode_setPosition(state->qrModules[moduleIndex], (FfxPoint){ .x = -300, .y = 0 });
            }
        }
    }
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
        esp_fill_random(state->privateKey, FFX_PRIVKEY_LENGTH);
        
        // Compute public key
        if (!ffx_pk_computePubkeySecp256k1(state->privateKey, state->publicKey)) {
            return;
        }
        
        // Compute address
        ffx_eth_computeAddress(state->publicKey, state->address);
        
        // Get checksum address string
        ffx_eth_checksumAddress(state->address, state->addressStr);
        
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
            ffx_sceneLabel_setText(state->nodeAddress1, "QR Code Display");
            ffx_sceneLabel_setText(state->nodeAddress2, state->addressStr);
            ffx_sceneLabel_setText(state->nodeInstructions, "Cancel=New  North=Back  Ok=Exit");
        } else {
            // Toggle back to address view
            state->showingQR = false;
            updateAddressDisplay(state);
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
    
    // Temporarily disable QR modules to prevent crashes
    // TODO: Re-implement with simpler approach
    for (int i = 0; i < QR_SIZE * QR_SIZE; i++) {
        state->qrModules[i] = NULL;  // Don't create visual modules for now
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