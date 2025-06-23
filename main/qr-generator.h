#ifndef __QR_GENERATOR_H__
#define __QR_GENERATOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define QR_VERSION 3
#define QR_SIZE 29  // QR code version 3 (29x29) - optimal for 42-character addresses
#define QR_MODULES (QR_SIZE * QR_SIZE)  // 841 modules
#define QR_DATA_CODEWORDS 53   // Data capacity for Version 3-L
#define QR_ECC_CODEWORDS 15    // Error correction codewords for Version 3-L
#define QR_TOTAL_CODEWORDS (QR_DATA_CODEWORDS + QR_ECC_CODEWORDS)  // 68 total
#define QR_SCALE 6  // Scale factor for display (29*6 = 174px)  
#define QR_OFFSET_X ((240 - QR_SIZE * QR_SCALE) / 2)  // Center QR code horizontally
#define QR_OFFSET_Y ((240 - QR_SIZE * QR_SCALE) / 2)  // Center QR code vertically

// Simple QR code structure
typedef struct {
    uint8_t modules[QR_MODULES];  // 1 = black, 0 = white
    int size;
} QRCode;

/**
 * Generate a simple QR code for text data
 * @param qr Pointer to QRCode structure to fill
 * @param data Text data to encode
 * @return true if successful, false if failed
 */
bool qr_generate(QRCode *qr, const char *data);

/**
 * Get module value at specific coordinates
 * @param qr QR code structure
 * @param x X coordinate (0 to size-1)
 * @param y Y coordinate (0 to size-1)
 * @return true if module is black, false if white
 */
bool qr_getModule(const QRCode *qr, int x, int y);

/**
 * Render QR code to display buffer with ETH address text
 * @param buffer Display buffer (RGB565 format)
 * @param y0 Starting y coordinate for this fragment
 * @param ethAddress ETH wallet address string
 * @param qr QR code structure
 */
void qr_renderToDisplay(uint8_t *buffer, uint32_t y0, const char *ethAddress, const QRCode *qr);

#ifdef __cplusplus
}
#endif

#endif /* __QR_GENERATOR_H__ */