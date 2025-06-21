#ifndef __QR_GENERATOR_H__
#define __QR_GENERATOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define QR_SIZE 13  // Smaller QR code to prevent memory issues  
#define QR_MODULES (QR_SIZE * QR_SIZE)

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

#ifdef __cplusplus
}
#endif

#endif /* __QR_GENERATOR_H__ */