# QR Code Generator Refactor Plan

## Current Issues with Version 2 Implementation
1. **Scannability Problems**: Current QR codes are not being recognized by scanners
2. **Size Mismatch**: Version 2 (25x25) is too small for 42-character Ethereum addresses
3. **Error Correction**: Only 10 ECC bytes may be insufficient
4. **Bit Encoding**: Potential issues in the data placement zigzag pattern
5. **Format Information**: May not be correctly masked

## Target: QR Version 3 (29x29) - ISO/IEC 18004 Compliant

### Specifications
- **Size**: 29×29 modules
- **Data Capacity**: 53 data codewords + 15 error correction codewords = 68 total
- **Error Correction Level**: L (Low) - ~7% data recovery capability
- **Mode**: Byte mode (0100) for raw ASCII encoding
- **Character Count**: 42 characters (00101010 binary)

### Data Structure
```
Total: 68 codewords (544 bits)
├── Data: 53 codewords (424 bits)
│   ├── Mode Indicator: 4 bits (0100)
│   ├── Character Count: 8 bits (00101010 = 42)
│   ├── Data Payload: 336 bits (42 × 8 bits)
│   ├── Terminator: 4 bits (0000) - only if space available
│   └── Padding: Remaining bits filled with 11101100 00010001 pattern
└── Error Correction: 15 codewords (120 bits)
```

### Implementation Plan

#### Phase 1: Update QR Structure Constants
```c
#define QR_VERSION 3
#define QR_SIZE 29
#define QR_MODULES (QR_SIZE * QR_SIZE)  // 841 modules
#define QR_DATA_CODEWORDS 53
#define QR_ECC_CODEWORDS 15
#define QR_TOTAL_CODEWORDS 68
#define QR_SCALE 6  // 29*6 = 174px fits well in 240px display
```

#### Phase 2: Proper Reed-Solomon Error Correction
- Generator polynomial for 15 ECC codewords (Version 3-L)
- Galois field GF(256) operations with proper primitive polynomial
- Polynomial division for ECC generation

#### Phase 3: Correct Function Pattern Placement
```
Version 3 (29×29) Function Patterns:
├── Finder Patterns: (3,3), (25,3), (3,25) - 7×7 each
├── Separators: White borders around finders
├── Timing Patterns: Row/Column 6 (alternating black/white)
├── Dark Module: (21, 8) - always black
└── Format Information: Around finder patterns
```

#### Phase 4: Proper Zigzag Data Placement
```
Data Placement Pattern (ISO/IEC 18004 Section 7.7.3):
1. Start at bottom-right corner (28,28)
2. Move in 2-column vertical strips
3. Alternate upward/downward direction per strip
4. Skip timing patterns and function patterns
5. Skip format information areas
```

#### Phase 5: Mask Pattern Evaluation
```c
// All 8 mask patterns (ISO/IEC 18004 Section 7.8.2)
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
    }
}
```

#### Phase 6: Format Information Encoding
```c
// Format information encoding (ISO/IEC 18004 Section 7.9)
// Error Correction Level L = 01, Mask Pattern = 000-111
uint16_t formatInfo = encodeFormatInfo(errorLevel, maskPattern);
formatInfo ^= 0x5412;  // Apply mask pattern
```

### Expected Benefits
1. **Larger Capacity**: 53 vs 34 data codewords (56% increase)
2. **Better Error Correction**: 15 vs 10 ECC codewords (50% increase)
3. **Improved Scannability**: Larger modules with proper spacing
4. **Standard Compliance**: Full ISO/IEC 18004 specification adherence
5. **Better Display Fit**: 29×6 = 174px fits well in 240px width

### Testing Strategy
1. **Unit Tests**: Test each component (ECC, masking, data placement)
2. **Reference Comparison**: Compare against known QR generators
3. **Scanner Testing**: Test with multiple QR scanner applications
4. **Edge Cases**: Test with various Ethereum address formats

### Migration Path
1. **Parallel Implementation**: Create new `qr-generator-v3.c` alongside existing
2. **Feature Flag**: Runtime switch between Version 2 and Version 3
3. **Gradual Transition**: Test thoroughly before replacing old implementation
4. **Fallback Support**: Keep Version 2 as backup if Version 3 fails

### File Structure
```
main/
├── qr-generator.h           # Updated constants and API
├── qr-generator-v3.c        # New ISO compliant implementation
├── qr-generator-v2.c        # Legacy implementation (backup)
├── qr-ecc.c                 # Reed-Solomon error correction
├── qr-mask.c                # Mask pattern evaluation
└── qr-placement.c           # Data placement utilities
```

### Performance Considerations
- **Memory Usage**: 29×29 = 841 bytes for modules array
- **Computation Time**: Mask evaluation for 8 patterns
- **Display Rendering**: 6x scaling fits in existing buffer size
- **Error Correction**: More complex but still feasible on ESP32-C3

### Success Criteria
1. ✅ QR code generated follows ISO/IEC 18004 exactly
2. ✅ Scannable by iOS Camera, Android Camera, and QR scanner apps
3. ✅ Returns exact 42-character Ethereum address
4. ✅ No system timeouts or crashes during generation
5. ✅ Proper display orientation and text alignment