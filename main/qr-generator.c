#include "qr-generator.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

// QR Code galois field operations for error correction
static uint8_t gf_mul(uint8_t a, uint8_t b) {
    if (a == 0 || b == 0) return 0;
    static const uint8_t gf_log[256] = {
        0, 0, 1, 25, 2, 50, 26, 198, 3, 223, 51, 238, 27, 104, 199, 75,
        4, 100, 224, 14, 52, 141, 239, 129, 28, 193, 105, 248, 200, 8, 76, 113,
        5, 138, 101, 47, 225, 36, 15, 33, 53, 147, 142, 218, 240, 18, 130, 69,
        29, 181, 194, 125, 106, 39, 249, 185, 201, 154, 9, 120, 77, 228, 114, 166,
        6, 191, 139, 98, 102, 221, 48, 253, 226, 152, 37, 179, 16, 145, 34, 136,
        54, 208, 148, 206, 143, 150, 219, 189, 241, 210, 19, 92, 131, 56, 70, 64,
        30, 66, 182, 163, 195, 72, 126, 110, 107, 58, 40, 84, 250, 133, 186, 61,
        202, 94, 155, 159, 10, 21, 121, 43, 78, 212, 229, 172, 115, 243, 167, 87,
        7, 112, 192, 247, 140, 128, 99, 13, 103, 74, 222, 237, 49, 197, 254, 24,
        227, 165, 153, 119, 38, 184, 180, 124, 17, 68, 146, 217, 35, 32, 137, 46,
        55, 63, 209, 91, 149, 188, 207, 205, 144, 135, 151, 178, 220, 252, 190, 97,
        242, 86, 211, 171, 20, 42, 93, 158, 132, 60, 57, 83, 71, 109, 65, 162,
        31, 45, 67, 216, 183, 123, 164, 118, 196, 23, 73, 236, 127, 12, 111, 246,
        108, 161, 59, 82, 41, 157, 85, 170, 251, 96, 134, 177, 187, 204, 62, 90,
        203, 89, 95, 176, 156, 169, 160, 81, 11, 245, 22, 235, 122, 117, 44, 215,
        79, 174, 213, 233, 230, 231, 173, 232, 116, 214, 244, 234, 168, 80, 88, 175
    };
    static const uint8_t gf_exp[256] = {
        1, 2, 4, 8, 16, 32, 64, 128, 29, 58, 116, 232, 205, 135, 19, 38,
        76, 152, 45, 90, 180, 117, 234, 201, 143, 3, 6, 12, 24, 48, 96, 192,
        157, 39, 78, 156, 37, 74, 148, 53, 106, 212, 181, 119, 238, 193, 159, 35,
        70, 140, 5, 10, 20, 40, 80, 160, 93, 186, 105, 210, 185, 111, 222, 161,
        95, 190, 97, 194, 153, 47, 94, 188, 101, 202, 137, 15, 30, 60, 120, 240,
        253, 231, 211, 187, 107, 214, 177, 127, 254, 225, 223, 163, 91, 182, 113, 226,
        217, 175, 67, 134, 17, 34, 68, 136, 13, 26, 52, 104, 208, 189, 103, 206,
        129, 31, 62, 124, 248, 237, 199, 147, 59, 118, 236, 197, 151, 51, 102, 204,
        133, 23, 46, 92, 184, 109, 218, 169, 79, 158, 33, 66, 132, 21, 42, 84,
        168, 77, 154, 41, 82, 164, 85, 170, 73, 146, 57, 114, 228, 213, 183, 115,
        230, 209, 191, 99, 198, 145, 63, 126, 252, 229, 215, 179, 123, 246, 241, 255,
        227, 219, 171, 75, 150, 49, 98, 196, 149, 55, 110, 220, 165, 87, 174, 65,
        130, 25, 50, 100, 200, 141, 7, 14, 28, 56, 112, 224, 221, 167, 83, 166,
        81, 162, 89, 178, 121, 242, 249, 239, 195, 155, 43, 86, 172, 69, 138, 9,
        18, 36, 72, 144, 61, 122, 244, 245, 247, 243, 251, 235, 203, 139, 11, 22,
        44, 88, 176, 125, 250, 233, 207, 131, 27, 54, 108, 216, 173, 71, 142, 1
    };
    return gf_exp[(gf_log[a] + gf_log[b]) % 255];
}

// Calculate penalty score for mask pattern (simplified version)
static int calculateMaskPenalty(QRCode *qr, int mask) {
    int penalty = 0;
    
    // Rule 1: Adjacent modules in row/column with same color
    for (int row = 0; row < qr->size; row++) {
        int count = 1;
        bool lastColor = qr_getModule(qr, 0, row);
        for (int col = 1; col < qr->size; col++) {
            bool color = qr_getModule(qr, col, row);
            if (color == lastColor) {
                count++;
            } else {
                if (count >= 5) penalty += (count - 2);
                count = 1;
                lastColor = color;
            }
        }
        if (count >= 5) penalty += (count - 2);
    }
    
    // Rule 2: Block of modules in same color (simplified)
    for (int row = 0; row < qr->size - 1; row++) {
        for (int col = 0; col < qr->size - 1; col++) {
            bool color = qr_getModule(qr, col, row);
            if (qr_getModule(qr, col + 1, row) == color &&
                qr_getModule(qr, col, row + 1) == color &&
                qr_getModule(qr, col + 1, row + 1) == color) {
                penalty += 3;
            }
        }
    }
    
    return penalty;
}

// Apply mask pattern to data modules
static bool applyMask(int mask, int row, int col) {
    switch (mask) {
        case 0: return (row + col) % 2 == 0;
        case 1: return row % 2 == 0;
        case 2: return col % 3 == 0;
        case 3: return (row + col) % 3 == 0;
        case 4: return (row / 2 + col / 3) % 2 == 0;
        case 5: return ((row * col) % 2) + ((row * col) % 3) == 0;
        case 6: return (((row * col) % 2) + ((row * col) % 3)) % 2 == 0;
        case 7: return (((row + col) % 2) + ((row * col) % 3)) % 2 == 0;
        default: return false;
    }
}

// Generate Reed-Solomon error correction codewords for QR Version 2-L
static void generateErrorCorrection(uint8_t *data, int dataLen, uint8_t *ecc, int eccLen) {
    // Generator polynomial for 10 error correction codewords (Version 2-L)
    uint8_t generator[] = {196, 35, 39, 119, 235, 215, 231, 226, 93, 23, 0}; // Coefficients for 10 ECC
    
    // Initialize ECC array
    memset(ecc, 0, eccLen);
    
    // Polynomial division
    for (int i = 0; i < dataLen; i++) {
        uint8_t feedback = data[i] ^ ecc[0];
        
        // Shift ECC array
        for (int j = 0; j < eccLen - 1; j++) {
            ecc[j] = ecc[j + 1] ^ (feedback ? gf_mul(generator[j], feedback) : 0);
        }
        ecc[eccLen - 1] = feedback ? gf_mul(generator[eccLen - 1], feedback) : 0;
    }
}

static void setModule(QRCode *qr, int x, int y, bool value) {
    if (x >= 0 && x < qr->size && y >= 0 && y < qr->size) {
        qr->modules[y * qr->size + x] = value ? 1 : 0;
    }
}

// Generate format info for QR Version 2 with given mask pattern
static uint16_t generateFormatInfo(int mask) {
    // Error correction level L (01) + mask pattern (3 bits)
    uint16_t data = (0x01 << 3) | (mask & 0x07);  // 5 bits total
    
    // BCH(15,5) error correction for format info
    uint16_t g = 0x537;  // Generator polynomial: x^10 + x^8 + x^5 + x^4 + x^2 + x + 1
    
    uint16_t remainder = data << 10;  // Shift data to high bits
    
    for (int i = 4; i >= 0; i--) {
        if (remainder & (1 << (i + 10))) {
            remainder ^= (g << i);
        }
    }
    
    uint16_t formatInfo = (data << 10) | remainder;
    
    // Apply mask pattern: 101010000010010
    formatInfo ^= 0x5412;
    
    return formatInfo;
}

// Choose optimal mask pattern by evaluating penalty scores
static int chooseBestMask(QRCode *qr, const uint8_t *fullStream) {
    int bestMask = 0;
    int bestPenalty = INT_MAX;
    
    printf("[qr] Evaluating mask patterns...\n");
    
    for (int mask = 0; mask < 8; mask++) {
        // Create a copy of the QR code for testing this mask
        QRCode testQr;
        memcpy(&testQr, qr, sizeof(QRCode));
        
        // Apply data with this mask pattern
        int byteIndex = 0;
        int bitIndex = 0;
        bool upward = true;
        
        // Place data with this mask pattern
        for (int colPair = qr->size - 1; colPair > 0; colPair -= 2) {
            if (colPair == 6) colPair--;
            
            for (int i = 0; i < qr->size; i++) {
                int row = upward ? (qr->size - 1 - i) : i;
                
                for (int colOffset = 0; colOffset >= -1; colOffset--) {
                    int col = colPair + colOffset;
                    
                    // Skip function patterns
                    if ((col < 9 && row < 9) ||
                        (col >= qr->size - 8 && row < 9) ||
                        (col < 9 && row >= qr->size - 8) ||
                        (col == 6 || row == 6) ||
                        (col == 8) || (row == 8)) {
                        continue;
                    }
                    
                    // Get data bit
                    bool dataBit = false;
                    if (byteIndex < 44) {
                        dataBit = (fullStream[byteIndex] >> (7 - bitIndex)) & 1;
                        bitIndex++;
                        if (bitIndex >= 8) {
                            bitIndex = 0;
                            byteIndex++;
                        }
                    }
                    
                    // Apply this mask pattern
                    bool maskBit = applyMask(mask, row, col);
                    bool finalBit = dataBit ^ maskBit;
                    
                    setModule(&testQr, col, row, finalBit);
                }
            }
            upward = !upward;
        }
        
        // Calculate penalty for this mask
        int penalty = calculateMaskPenalty(&testQr, mask);
        printf("[qr] Mask %d penalty: %d\n", mask, penalty);
        
        if (penalty < bestPenalty) {
            bestPenalty = penalty;
            bestMask = mask;
        }
        
        // Reset for next iteration
        byteIndex = 0;
        bitIndex = 0;
        upward = true;
    }
    
    printf("[qr] Selected mask %d with penalty %d\n", bestMask, bestPenalty);
    return bestMask;
}

// Large QR code pattern generation for full-screen display
// Creates a QR-like pattern that encodes ETH wallet addresses
// Optimized for 240x240 display with text overlay

static void drawFinderPattern(QRCode *qr, int x, int y) {
    // Draw standard 7x7 finder pattern
    for (int dy = -3; dy <= 3; dy++) {
        for (int dx = -3; dx <= 3; dx++) {
            int px = x + dx;
            int py = y + dy;
            
            // Outer border (7x7)
            if (abs(dx) == 3 || abs(dy) == 3) {
                setModule(qr, px, py, true);
            }
            // Inner square (3x3)
            else if (abs(dx) <= 1 && abs(dy) <= 1) {
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
    // Standard timing patterns at row/col 6
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
    int dataLen = strlen(data);
    printf("[qr] Encoding data: %s (length=%d)\n", data, dataLen);
    
    // Create QR data stream for Version 2-L (34 data + 10 error correction = 44 total)
    uint8_t dataBytes[34];
    uint8_t eccBytes[10];
    uint8_t fullStream[44];
    
    memset(dataBytes, 0, sizeof(dataBytes));
    
    // Create proper QR bit stream using explicit bit placement
    // Mode indicator: 0100 (4 bits for Byte mode)  
    // Character count: 42 = 00101010 (8 bits)
    // Total header: 12 bits
    
    int bitPos = 0;
    
    // Write mode indicator: 0100 (Byte mode)
    for (int i = 3; i >= 0; i--) {
        bool bit = (0x4 >> i) & 1;
        int byteIndex = bitPos / 8;
        int bitIndex = 7 - (bitPos % 8);
        
        if (bit) {
            dataBytes[byteIndex] |= (1 << bitIndex);
        }
        bitPos++;
    }
    
    // Write character count: 42 = 00101010 (8 bits)
    for (int i = 7; i >= 0; i--) {
        bool bit = (dataLen >> i) & 1;
        int byteIndex = bitPos / 8;
        int bitIndex = 7 - (bitPos % 8);
        
        if (bit) {
            dataBytes[byteIndex] |= (1 << bitIndex);
        }
        bitPos++;
    }
    
    // Data: Add ETH address characters byte by byte
    for (int i = 0; i < dataLen; i++) {
        uint8_t dataByte = (uint8_t)data[i];
        
        // Write each bit of the data byte
        for (int j = 7; j >= 0; j--) {
            bool bit = (dataByte >> j) & 1;
            int byteIndex = bitPos / 8;
            int bitIndex = 7 - (bitPos % 8);
            
            if (byteIndex < 34 && bit) {
                dataBytes[byteIndex] |= (1 << bitIndex);
            }
            bitPos++;
        }
    }
    
    // Add terminator 0000 (4 bits) if we have space
    int currentByteIndex = bitPos / 8;
    if (currentByteIndex < 34) {
        bitPos += 4; // Skip 4 bits for terminator (already 0 from memset)
    }
    
    // Pad to byte boundary if needed
    int bitsToNextByte = (8 - (bitPos % 8)) % 8;
    bitPos += bitsToNextByte;
    
    // Pad remaining space with 11101100 00010001 pattern
    int padByteIndex = bitPos / 8;
    uint8_t padPattern[] = {0xEC, 0x11};
    int padIndex = 0;
    while (padByteIndex < 34) {
        dataBytes[padByteIndex++] = padPattern[padIndex % 2];
        padIndex++;
    }
    
    // Generate error correction codewords
    generateErrorCorrection(dataBytes, 34, eccBytes, 10);
    
    // Combine data and error correction
    memcpy(fullStream, dataBytes, 34);
    memcpy(fullStream + 34, eccBytes, 10);
    
    printf("[qr] Data encoding: Byte0=0x%02X Byte1=0x%02X Byte2=0x%02X\n", 
           dataBytes[0], dataBytes[1], dataBytes[2]);
    printf("[qr] Expected: Mode=0100 Count=00101010 -> Byte0=0x42 Byte1=0xA0\n");
    
    // Debug: Print first few bytes of address data to verify encoding
    printf("[qr] Address bytes: ");
    for (int i = 0; i < 8 && i < dataLen; i++) {
        printf("0x%02X('%c') ", (uint8_t)data[i], data[i]);
    }
    printf("...\n");
    
    // Debug: Show bit stream structure
    printf("[qr] Bit structure: Mode(4)=0x%X Count(8)=0x%02X FirstChar(8)=0x%02X\n", 
           0x4, dataLen, (uint8_t)data[0]);
    printf("[qr] Address data: %.*s...\n", dataLen > 10 ? 10 : dataLen, data);
    printf("[qr] Generated error correction, total stream: 44 bytes\n");
    
    // Choose optimal mask pattern by evaluating all 8 patterns
    int selectedMask = chooseBestMask(qr, fullStream);
    
    // Generate format info for selected mask pattern
    uint16_t formatInfo = generateFormatInfo(selectedMask);
    printf("[qr] Using mask %d, format info: 0x%04x\n", selectedMask, formatInfo);
    
    // Place format information around finder patterns
    // Top-left format bits
    for (int i = 0; i < 6; i++) {
        bool bit = (formatInfo >> i) & 1;
        setModule(qr, 8, i, bit);
        setModule(qr, i, 8, bit);
    }
    setModule(qr, 8, 7, (formatInfo >> 6) & 1);
    setModule(qr, 8, 8, (formatInfo >> 7) & 1);
    setModule(qr, 7, 8, (formatInfo >> 8) & 1);
    
    // Top-right format bits
    for (int i = 0; i < 8; i++) {
        bool bit = (formatInfo >> (14 - i)) & 1;
        setModule(qr, qr->size - 1 - i, 8, bit);
    }
    
    // Bottom-left format bits
    for (int i = 0; i < 7; i++) {
        bool bit = (formatInfo >> (6 - i)) & 1;
        setModule(qr, 8, qr->size - 7 + i, bit);
    }
    
    // Dark module at (4*version + 9, 8) = (17, 8) for version 2
    setModule(qr, 8, qr->size - 8, true);
    
    // Place data in proper QR zigzag pattern starting from bottom-right
    int byteIndex = 0;
    int bitIndex = 0;
    bool upward = true; // Direction flag for zigzag
    
    // Start from rightmost column pair and work leftward
    for (int colPair = qr->size - 1; colPair > 0; colPair -= 2) {
        // Skip timing column (column 6)
        if (colPair == 6) {
            colPair--;
        }
        
        // Process this column pair (right column first, then left)
        for (int i = 0; i < qr->size; i++) {
            // Calculate row based on direction
            int row = upward ? (qr->size - 1 - i) : i;
            
            // Process right column then left column of the pair
            for (int colOffset = 0; colOffset >= -1; colOffset--) {
                int col = colPair + colOffset;
                
                // Skip if this is a function pattern area
                if ((col < 9 && row < 9) ||                    // Top-left finder + format
                    (col >= qr->size - 8 && row < 9) ||       // Top-right finder + format
                    (col < 9 && row >= qr->size - 8) ||       // Bottom-left finder + format
                    (col == 6 || row == 6) ||                 // Timing patterns
                    (col == 8) || (row == 8)) {               // Format info
                    continue;
                }
                
                // Get data bit
                bool dataBit = false;
                if (byteIndex < 44) {
                    dataBit = (fullStream[byteIndex] >> (7 - bitIndex)) & 1;
                    bitIndex++;
                    if (bitIndex >= 8) {
                        bitIndex = 0;
                        byteIndex++;
                    }
                }
                
                // Apply selected mask pattern
                bool mask = applyMask(selectedMask, row, col);
                bool finalBit = dataBit ^ mask;
                
                setModule(qr, col, row, finalBit);
            }
        }
        
        // Flip direction for next column pair
        upward = !upward;
    }
    
    printf("[qr] Placed data with error correction and mask pattern %d, used %d bytes\n", selectedMask, byteIndex);
    printf("[qr] QR Version 2 (25x25) generated successfully\n");
    
    // Debug: Verify QR structure
    printf("[qr] QR Structure: Size=%d, Finder patterns at (3,3), (%d,3), (3,%d)\n", 
           qr->size, qr->size-4, qr->size-4);
    printf("[qr] Timing patterns at row/col 6, Format info around finders\n");
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

// Simple 8x8 font for displaying ETH address
static const uint8_t font8x8[96][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ' '
    {0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00}, // '!'
    {0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // '"'
    {0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00}, // '#'
    {0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00}, // '$'
    {0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00}, // '%'
    {0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00}, // '&'
    {0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}, // '
    {0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00}, // '()
    {0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00}, // ')'
    {0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00}, // '*'
    {0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00}, // '+'
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x06, 0x00}, // ','
    {0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00}, // '-'
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00}, // '.'
    {0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00}, // '/'
    {0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00}, // '0'
    {0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00}, // '1'
    {0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00}, // '2'
    {0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00}, // '3'
    {0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00}, // '4'
    {0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00}, // '5'
    {0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00}, // '6'
    {0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00}, // '7'
    {0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00}, // '8'
    {0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00}, // '9'
    {0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00}, // ':'
    {0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x06, 0x00}, // ';'
    {0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00}, // '<'
    {0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00}, // '='
    {0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00}, // '>'
    {0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00}, // '?'
    {0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00}, // '@'
    {0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00}, // 'A'
    {0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00}, // 'B'
    {0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00}, // 'C'
    {0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00}, // 'D'
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00}, // 'E'
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00}, // 'F'
};

void qr_renderToDisplay(uint8_t *buffer, uint32_t y0, const char *ethAddress, const QRCode *qr) {
    const uint32_t DISPLAY_WIDTH = 240;
    const uint32_t FRAGMENT_HEIGHT = 24;
    
    // Defensive check to prevent corruption
    if (!buffer || !ethAddress || !qr) {
        return;
    }
    
    // Clear buffer with white background (RGB565: 0xFFFF)
    uint16_t *pixelBuffer = (uint16_t*)buffer;
    for (int i = 0; i < DISPLAY_WIDTH * FRAGMENT_HEIGHT; i++) {
        pixelBuffer[i] = 0xFFFF; // White in RGB565
    }
    
    // Display ETH address text at top (first 30 pixels)
    if (y0 < 30) {
        // Safely copy address to prevent corruption
        char safe_address[50];
        strncpy(safe_address, ethAddress, 42);
        safe_address[42] = '\0'; // Ensure null termination
        
        // Only print address once per QR generation (not per fragment)
        static char last_address[50] = "";
        if (strcmp(safe_address, last_address) != 0) {
            printf("[qr] Rendering ETH address: %.42s\n", safe_address);
            strncpy(last_address, safe_address, sizeof(last_address) - 1);
            last_address[sizeof(last_address) - 1] = '\0';
        }
        
        // Render address text in two lines
        int line = 0;
        int char_x = DISPLAY_WIDTH - 10 - (21 * 6); // Start from right, account for 21 chars * 6 pixels each
        
        for (int i = 0; i < 42 && line < 2; i++) {
            char c = safe_address[i];
            
            // Handle line wrapping after 21 characters
            if (i == 21) {
                line++;
                char_x = DISPLAY_WIDTH - 10 - (21 * 6); // Reset to right position for second line
            }
            
            if (line >= 2) break;
            
            int char_y = 2 + (line * 12);
            
            // Skip if this character is outside current fragment
            if (char_y < y0 || char_y + 8 > y0 + FRAGMENT_HEIGHT) {
                char_x += 6;
                continue;
            }
            
            // Get font data for character
            uint8_t font_index = (c >= 32 && c <= 127) ? (c - 32) : 0;
            if (font_index >= 96) font_index = 0;
            
            // Draw character (fix mirroring by reading bits left-to-right)
            for (int py = 0; py < 8; py++) {
                if (char_y + py < y0 || char_y + py >= y0 + FRAGMENT_HEIGHT) continue;
                
                uint8_t row = font8x8[font_index][py];
                for (int px = 0; px < 8; px++) {
                    if (char_x + px >= DISPLAY_WIDTH) break;
                    
                    // Fix bit order - read from MSB to LSB (left to right)
                    if (row & (0x80 >> px)) {
                        // Mirror X coordinate to fix display orientation
                        int mirrored_x = DISPLAY_WIDTH - 1 - (char_x + px);
                        int pixel_pos = (char_y + py - y0) * DISPLAY_WIDTH + mirrored_x;
                        if (pixel_pos >= 0 && pixel_pos < DISPLAY_WIDTH * FRAGMENT_HEIGHT) {
                            pixelBuffer[pixel_pos] = 0x0000; // Black text in RGB565
                        }
                    }
                }
            }
            char_x += 6; // Character spacing
        }
    }
    
    // Render scaled QR code starting below text area
    for (int y = 0; y < FRAGMENT_HEIGHT; y++) {
        int display_y = y0 + y;
        
        // Skip text area
        if (display_y < QR_OFFSET_Y) continue;
        
        int qr_pixel_y = display_y - QR_OFFSET_Y;
        int qr_module_y = qr_pixel_y / QR_SCALE;
        
        // Check if we're within QR code bounds
        if (qr_module_y >= 0 && qr_module_y < qr->size) {
            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                int qr_pixel_x = x - QR_OFFSET_X;
                
                bool is_black = false;
                if (qr_pixel_x >= 0) {
                    int qr_module_x = qr_pixel_x / QR_SCALE;
                    if (qr_module_x >= 0 && qr_module_x < qr->size) {
                        is_black = qr_getModule(qr, qr_module_x, qr_module_y);
                    }
                }
                
                // Mirror X coordinate to fix display orientation
                int mirrored_x = DISPLAY_WIDTH - 1 - x;
                int pixel_pos = y * DISPLAY_WIDTH + mirrored_x;
                if (pixel_pos >= 0 && pixel_pos < DISPLAY_WIDTH * FRAGMENT_HEIGHT) {
                    if (is_black) {
                        pixelBuffer[pixel_pos] = 0x0000; // Black in RGB565
                    }
                    // White pixels already set above
                }
            }
        }
    }
}