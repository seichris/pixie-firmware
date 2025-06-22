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
    // Create a more realistic QR pattern with format info and data modules
    uint32_t hash = 0;
    const char *p = data;
    while (*p) {
        hash = hash * 31 + *p++;
    }
    
    // Fill format information areas (like real QR codes)
    // Top-left format info (around top-left finder)
    for (int i = 0; i < 9; i++) {
        if (i != 6) { // Skip timing pattern
            setModule(qr, 8, i, (hash >> i) & 1);
            setModule(qr, i, 8, (hash >> (i + 1)) & 1);
        }
    }
    
    // Top-right format info
    for (int i = 0; i < 7; i++) {
        setModule(qr, qr->size - 1 - i, 8, (hash >> (i + 2)) & 1);
    }
    
    // Bottom-left format info
    for (int i = 0; i < 7; i++) {
        setModule(qr, 8, qr->size - 7 + i, (hash >> (i + 3)) & 1);
    }
    
    // Dark module (required)
    setModule(qr, 8, qr->size - 8, true);
    
    // Fill data area with a more structured pattern
    for (int y = 0; y < qr->size; y++) {
        for (int x = 0; x < qr->size; x++) {
            // Skip already filled areas
            if ((x < 9 && y < 9) ||                    // Top-left finder + format
                (x >= qr->size - 8 && y < 9) ||       // Top-right finder + format
                (x < 9 && y >= qr->size - 8) ||       // Bottom-left finder + format
                (x == 6 || y == 6) ||                 // Timing patterns
                (x == 8 && y < qr->size - 7) ||       // Format info column
                (y == 8 && x < qr->size - 7)) {       // Format info row
                continue;
            }
            
            // Create data pattern based on position and hash
            // Use both coordinates to create more variation
            uint32_t posHash = hash;
            posHash ^= (x * 7) ^ (y * 11);
            posHash ^= ((x + y) * 3);
            
            // Create alternating regions for better visual structure
            int region = ((x / 4) + (y / 4)) % 4;
            posHash ^= region * 17;
            
            // ~50% fill with some spatial correlation
            bool value = (posHash % 7) < 3;  // 3/7 ≈ 43% fill rate
            
            setModule(qr, x, y, value);
        }
    }
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