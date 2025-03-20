#include "atecc608a.h"
#include "i2c.h"
#include <string.h>

//---------------------------------------------------------------------
// Default Configuration for basic signing/verification operation
//
// This configuration is 128 bytes long and is arranged as follows:
//   Bytes  0- 3:  Serial Number (dummy values; in production these are set by manufacturer)
//   Bytes  4- 7:  Revision Number (dummy)
//   Bytes  8-12:  More Serial Number bytes (dummy)
//   Byte     13:  AES_Enable (not used here, so set to 0)
//   Byte     14:  I2C_Enable (bit0=1 to enable I2C)
//   Byte     15:  Reserved (0)
//   Byte     16:  I2C_Address (0xC0 for 8-bit; corresponds to 0x60 in 7-bit)
//   Byte     17:  Reserved (0)
//   Byte     18:  CountMatch (0)
//   Byte     19:  ChipMode (0; use I2C address from byte 16)
//   Bytes 20-51:  SlotConfig for 16 slots (2 bytes per slot)
//                For slot 0, we set a sample value (0x80,0x00) indicating an ECC private key that is non‑readable.
//                The other slots are left as default (0x00,0x00).
//   Bytes 52-59:  Counter[0] (8 bytes, all 0)
//   Bytes 60-67:  Counter[1] (8 bytes, all 0)
//   Byte     68:  UseLock (0)
//   Byte     69:  VolatileKey Permission (0)
//   Bytes 70-71:  SecureBoot (0,0)
//   Byte     72:  KdfIvLoc (0)
//   Bytes 73-74:  KdfIvStr (0,0)
//   Bytes 75-83:  Reserved (9 bytes, 0)
//   Byte     84:  UserExtra (0)
//   Byte     85:  UserExtraAdd (0)
//   Byte     86:  LockValue (0x55 means unlocked)
//   Byte     87:  LockConfig (0x55 means unlocked)
//   Bytes 88-89:  SlotLocked (2 bytes, 0)
//   Bytes 90-91:  ChipOptions (2 bytes, 0)
//   Bytes 92-95:  X509format (4 bytes, 0)
//   Bytes 96-127: KeyConfig for 16 slots (2 bytes per slot)
//                For slot 0, set to 0x80,0x00 (to indicate an ECC private key used for signing)
//                The remaining slots are left as 0.
 //---------------------------------------------------------------------

static const uint8_t default_config[128] = {
    // Bytes 0-3: Serial Number (dummy)
    0x01, 0x23, 0x45, 0x67,
    // Bytes 4-7: Revision Number (dummy)
    0x00, 0x00, 0x00, 0x00,
    // Bytes 8-12: Serial Number (dummy)
    0x89, 0xAB, 0xCD, 0xEF, 0x01,
    // Byte 13: AES_Enable
    0x00,
    // Byte 14: I2C_Enable (enable I2C)
    0x01,
    // Byte 15: Reserved
    0x00,
    // Byte 16: I2C_Address (0xC0 for 8-bit, corresponds to 0x60 in 7-bit)
    0xC0,
    // Byte 17: Reserved
    0x00,
    // Byte 18: CountMatch
    0x00,
    // Byte 19: ChipMode
    0x00,
    // Bytes 20-51: SlotConfig (16 slots x 2 bytes)
    // For slot 0: set to 0x80,0x00 (ECC private key, non-readable, sign allowed)
    0x80, 0x00, // slot 0
    // Slots 1 to 15: default (all 0)
    0x00, 0x00, // slot 1
    0x00, 0x00, // slot 2
    0x00, 0x00, // slot 3
    0x00, 0x00, // slot 4
    0x00, 0x00, // slot 5
    0x00, 0x00, // slot 6
    0x00, 0x00, // slot 7
    0x00, 0x00, // slot 8
    0x00, 0x00, // slot 9
    0x00, 0x00, // slot 10
    0x00, 0x00, // slot 11
    0x00, 0x00, // slot 12
    0x00, 0x00, // slot 13
    0x00, 0x00, // slot 14
    0x00, 0x00, // slot 15
    // Bytes 52-59: Counter[0] (8 bytes)
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    // Bytes 60-67: Counter[1] (8 bytes)
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    // Byte 68: UseLock
    0x00,
    // Byte 69: VolatileKey Permission
    0x00,
    // Bytes 70-71: SecureBoot
    0x00, 0x00,
    // Byte 72: KdfIvLoc
    0x00,
    // Bytes 73-74: KdfIvStr
    0x00, 0x00,
    // Bytes 75-83: Reserved (9 bytes)
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    // Byte 84: UserExtra
    0x00,
    // Byte 85: UserExtraAdd
    0x00,
    // Byte 86: LockValue (0x55 means unlocked)
    0x55,
    // Byte 87: LockConfig (0x55 means unlocked)
    0x55,
    // Bytes 88-89: SlotLocked (2 bytes)
    0x00,0x00,
    // Bytes 90-91: ChipOptions (2 bytes)
    0x00,0x00,
    // Bytes 92-95: X509format (4 bytes)
    0x00,0x00,0x00,0x00,
    // Bytes 96-127: KeyConfig (16 slots x 2 bytes)
    // For slot 0: set to 0x80,0x00 (ECC private key with sign enabled, non-readable)
    0x80, 0x00, // slot 0
    // Slots 1 to 15: default
    0x00, 0x00, // slot 1
    0x00, 0x00, // slot 2
    0x00, 0x00, // slot 3
    0x00, 0x00, // slot 4
    0x00, 0x00, // slot 5
    0x00, 0x00, // slot 6
    0x00, 0x00, // slot 7
    0x00, 0x00, // slot 8
    0x00, 0x00, // slot 9
    0x00, 0x00, // slot 10
    0x00, 0x00, // slot 11
    0x00, 0x00, // slot 12
    0x00, 0x00, // slot 13
    0x00, 0x00, // slot 14
    0x00, 0x00  // slot 15
};