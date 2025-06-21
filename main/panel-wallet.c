#include <stdio.h>
#include <string.h>
#include <esp_random.h>

#include "firefly-scene.h"
#include "firefly-crypto.h"
#include "firefly-address.h"

#include "panel.h"
#include "panel-wallet.h"

typedef struct WalletState {
    FfxScene scene;
    FfxNode nodeAddress1;
    FfxNode nodeAddress2;
    FfxNode nodeBackground;
    FfxNode nodeInstructions;
    uint8_t privateKey[FFX_PRIVKEY_LENGTH];
    uint8_t publicKey[FFX_PUBKEY_LENGTH];
    uint8_t address[FFX_ADDRESS_LENGTH];
    char addressStr[FFX_ADDRESS_STRING_LENGTH];
    char addressLine1[25];
    char addressLine2[25];
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

static void keyChanged(EventPayload event, void *_state) {
    WalletState *state = _state;
    
    switch(event.props.keys.down) {
        case KeyWest:
            panel_pop();
            return;
        case KeyOk:
            if (state->showingQR) {
                // Toggle back to address view
                state->showingQR = false;
                updateAddressDisplay(state);
                ffx_sceneLabel_setText(state->nodeInstructions, "OK=New  East=QR  West=Exit");
            } else {
                // Generate new wallet
                esp_fill_random(state->privateKey, FFX_PRIVKEY_LENGTH);
                
                // Compute public key
                if (!ffx_pk_computePubkeySecp256k1(state->privateKey, state->publicKey)) {
                    printf("[wallet] Failed to compute public key\n");
                    return;
                }
                
                // Compute address
                ffx_eth_computeAddress(state->publicKey, state->address);
                
                // Get checksum address string
                ffx_eth_checksumAddress(state->address, state->addressStr);
                
                // Update display
                updateAddressDisplay(state);
                
                printf("[wallet] New address: %s\n", state->addressStr);
            }
            return;
        case KeyEast:
            if (!state->showingQR) {
                // Show QR code placeholder
                state->showingQR = true;
                ffx_sceneLabel_setText(state->nodeAddress1, "QR Code");
                ffx_sceneLabel_setText(state->nodeAddress2, "(Placeholder)");
                ffx_sceneLabel_setText(state->nodeInstructions, "OK=Back  West=Exit");
            }
            return;
        default:
            return;
    }
}

static int init(FfxScene scene, FfxNode node, void* _state, void* arg) {
    WalletState *state = _state;
    state->scene = scene;
    
    // Create title
    FfxNode nodeTitle = ffx_scene_createLabel(scene, FfxFontLarge, "ETH Wallet");
    ffx_sceneGroup_appendChild(node, nodeTitle);
    ffx_sceneNode_setPosition(nodeTitle, (FfxPoint){ .x = 120, .y = 30 });
    
    // Create background for address text
    state->nodeBackground = ffx_scene_createBox(scene, ffx_size(200, 80));
    ffx_sceneBox_setColor(state->nodeBackground, ffx_color_rgba(0, 0, 0, 200));
    ffx_sceneGroup_appendChild(node, state->nodeBackground);
    ffx_sceneNode_setPosition(state->nodeBackground, (FfxPoint){ .x = 20, .y = 75 });
    
    // Create address labels (split into two lines)
    state->nodeAddress1 = ffx_scene_createLabel(scene, FfxFontMedium, "Press OK to");
    ffx_sceneGroup_appendChild(node, state->nodeAddress1);
    ffx_sceneNode_setPosition(state->nodeAddress1, (FfxPoint){ .x = 25, .y = 85 });
    
    state->nodeAddress2 = ffx_scene_createLabel(scene, FfxFontMedium, "generate wallet");
    ffx_sceneGroup_appendChild(node, state->nodeAddress2);
    ffx_sceneNode_setPosition(state->nodeAddress2, (FfxPoint){ .x = 25, .y = 115 });
    
    // Create instructions
    state->nodeInstructions = ffx_scene_createLabel(scene, FfxFontSmall, "OK=New  East=QR  West=Exit");
    ffx_sceneGroup_appendChild(node, state->nodeInstructions);
    ffx_sceneNode_setPosition(state->nodeInstructions, (FfxPoint){ .x = 40, .y = 200 });
    
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
    
    // Register for key events
    panel_onEvent(EventNameKeysChanged | KeyWest | KeyOk | KeyEast, keyChanged, state);
    
    return 0;
}

void pushPanelWallet(void* arg) {
    panel_push(init, sizeof(WalletState), PanelStyleSlideLeft, arg);
}