#ifndef __QR_GENERATOR_H__
#define __QR_GENERATOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define QR_SIZE 25  // QR code version 2 (25x25) - can handle 42 bytes
#define QR_MODULES (QR_SIZE * QR_SIZE)
#define QR_SCALE 7  // Scale factor for display (25*7 = 175px)
#define QR_OFFSET_X 32  // Center QR code horizontally ((240-175)/2)
#define QR_OFFSET_Y 40   // Leave space at top for address text

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