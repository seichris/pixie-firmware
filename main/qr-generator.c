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
    // Simple data encoding - create pattern based on data hash
    uint32_t hash = 0;
    const char *p = data;
    while (*p) {
        hash = hash * 31 + *p++;
    }
    
    // Fill data area with pattern based on hash
    for (int y = 9; y < qr->size - 8; y++) {
        for (int x = 9; x < qr->size - 8; x++) {
            // Skip timing patterns
            if (x == 6 || y == 6) continue;
            
            // Create pattern based on position and hash
            uint32_t pattern = (hash ^ (x * 17) ^ (y * 23)) & 0xFF;
            setModule(qr, x, y, (pattern & 1) == 1);
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