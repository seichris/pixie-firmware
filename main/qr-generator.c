#include "qr-generator.h"
#include <string.h>
#include <stdio.h>

// Simple QR code pattern generation
// This is a minimal implementation that creates a QR-like pattern
// For production use, consider integrating a proper QR library

static void setModule(QRCode *qr, int x, int y, bool value) {
    if (x >= 0 && x < qr->size && y >= 0 && y < qr->size) {
        qr->modules[y * qr->size + x] = value ? 1 : 0;
    }
}

static void drawFinderPattern(QRCode *qr, int x, int y) {
    // Draw 7x7 finder pattern (corner squares)
    for (int dy = -3; dy <= 3; dy++) {
        for (int dx = -3; dx <= 3; dx++) {
            int px = x + dx;
            int py = y + dy;
            
            // Outer border
            if (dx == -3 || dx == 3 || dy == -3 || dy == 3) {
                setModule(qr, px, py, true);
            }
            // Inner square
            else if (dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1) {
                setModule(qr, px, py, true);
            }
            // White space between
            else {
                setModule(qr, px, py, false);
            }
        }
    }
}

static void drawTimingPattern(QRCode *qr) {
    // Horizontal timing pattern
    for (int x = 8; x < qr->size - 8; x++) {
        setModule(qr, x, 6, (x % 2) == 0);
    }
    
    // Vertical timing pattern  
    for (int y = 8; y < qr->size - 8; y++) {
        setModule(qr, 6, y, (y % 2) == 0);
    }
}

static void encodeData(QRCode *qr, const char *data) {
    // Create a simple text-based QR pattern that embeds the actual address
    int dataLen = strlen(data);
    printf("[qr] Encoding data: %s (length=%d)\n", data, dataLen);
    
    // For debugging: log first few characters
    if (dataLen > 0) {
        printf("[qr] First chars: 0x%02x 0x%02x 0x%02x\n", 
               (uint8_t)data[0], 
               dataLen > 1 ? (uint8_t)data[1] : 0,
               dataLen > 2 ? (uint8_t)data[2] : 0);
    }
    
    // Create proper QR format information for Version 1 (21x21), Error Correction Level L
    uint16_t formatInfo = 0x548C; // Format bits for ECL=L, Mask=0
    printf("[qr] Using format info: 0x%04x\n", formatInfo);
    
    // Top-left format info
    for (int i = 0; i < 9; i++) {
        if (i != 6) { // Skip timing pattern position
            bool bit = (formatInfo >> i) & 1;
            setModule(qr, 8, i, bit);
            setModule(qr, i, 8, bit);
        }
    }
    
    // Top-right format info  
    for (int i = 0; i < 8; i++) {
        bool bit = (formatInfo >> (14 - i)) & 1;
        setModule(qr, qr->size - 1 - i, 8, bit);
    }
    
    // Bottom-left format info
    for (int i = 0; i < 7; i++) {
        bool bit = (formatInfo >> (6 - i)) & 1;
        setModule(qr, 8, qr->size - 7 + i, bit);
    }
    
    // Dark module (always dark at position (4*version + 9, 8))
    setModule(qr, 8, qr->size - 8, true);
    
    // Fill data area with actual address bits
    int dataByteIndex = 0;
    int dataBitIndex = 0;
    int modulesPlaced = 0;
    
    // QR code data is filled in a specific zigzag pattern, but we'll use a simpler approach
    // for better readability while still encoding the actual address
    for (int y = 0; y < qr->size; y++) {
        for (int x = 0; x < qr->size; x++) {
            // Skip function patterns
            if ((x < 9 && y < 9) ||                    // Top-left finder + format
                (x >= qr->size - 8 && y < 9) ||       // Top-right finder + format  
                (x < 9 && y >= qr->size - 8) ||       // Bottom-left finder + format
                (x == 6 || y == 6) ||                 // Timing patterns
                (x == 8 && y < qr->size - 7) ||       // Format info column
                (y == 8 && x < qr->size - 7)) {       // Format info row
                continue;
            }
            
            bool value = false;
            
            // Use actual data bits when available
            if (dataByteIndex < dataLen) {
                uint8_t currentByte = (uint8_t)data[dataByteIndex];
                value = (currentByte >> (7 - dataBitIndex)) & 1;
                
                dataBitIndex++;
                if (dataBitIndex >= 8) {
                    dataBitIndex = 0;
                    dataByteIndex++;
                }
            } else {
                // Padding pattern when we run out of data
                value = (modulesPlaced % 17) < 8; // Create a recognizable pattern
            }
            
            setModule(qr, x, y, value);
            modulesPlaced++;
        }
    }
    
    printf("[qr] Encoded %d data modules, used %d bytes of address data\n", modulesPlaced, dataByteIndex);
}

bool qr_generate(QRCode *qr, const char *data) {
    if (!qr || !data) {
        return false;
    }
    
    qr->size = QR_SIZE;
    
    // Clear all modules
    memset(qr->modules, 0, QR_MODULES);
    
    // Draw finder patterns (corners)
    drawFinderPattern(qr, 3, 3);                    // Top-left
    drawFinderPattern(qr, qr->size - 4, 3);        // Top-right  
    drawFinderPattern(qr, 3, qr->size - 4);        // Bottom-left
    
    // Draw timing patterns
    drawTimingPattern(qr);
    
    // Encode data
    encodeData(qr, data);
    
    return true;
}

bool qr_getModule(const QRCode *qr, int x, int y) {
    if (!qr || x < 0 || x >= qr->size || y < 0 || y >= qr->size) {
        return false;
    }
    
    return qr->modules[y * qr->size + x] == 1;
}