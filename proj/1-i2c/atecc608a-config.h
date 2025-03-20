// Note this is a WIP.

// SlotConfig bit definitions (per slot)
#define SLOT_CONFIG_READ_KEY_SIGN_EXT       (1 << 0)  // External signatures enabled
#define SLOT_CONFIG_READ_KEY_SIGN_INT       (1 << 1)  // Internal signatures enabled
#define SLOT_CONFIG_READ_KEY_ECDH           (1 << 2)  // ECDH operation permitted
#define SLOT_CONFIG_READ_KEY_ECDH_SECRET    (1 << 3)  // ECDH master secret written to N|1 if set

#define SLOT_CONFIG_NO_MAC                  (1 << 4)  // Key can't be used by MAC command
#define SLOT_CONFIG_LIMITED_USE             (1 << 5)  // Usage controlled by Counter0
#define SLOT_CONFIG_ENCRYPT_READ            (1 << 6)  // Reads encrypted using ReadKey
#define SLOT_CONFIG_IS_SECRET               (1 << 7)  // Contents are secret

// Write key occupies bits 8-11
#define SLOT_CONFIG_WRITE_KEY(key)          ((key & 0x0F) << 8)

// Write config occupies bits 12-15
#define SLOT_CONFIG_WRITE_CONFIG(config)    ((config & 0x0F) << 12)

// Common write config values (to use with SLOT_CONFIG_WRITE_CONFIG)
#define WRITE_CONFIG_ALWAYS                 0
#define WRITE_CONFIG_PUB_INVALID            1
#define WRITE_CONFIG_NEVER                  2
#define WRITE_CONFIG_ENCRYPT                3
#define WRITE_CONFIG_DERIVEKEY              4
#define WRITE_CONFIG_ENCRYPT_MAC            5
#define WRITE_CONFIG_PUB_DERIVE_KEY         6
#define WRITE_CONFIG_PUB_DERIVE_KEY_MAC     7
#define WRITE_CONFIG_KEY4_13_15             8
#define WRITE_CONFIG_KEY8_13_15             9
#define WRITE_CONFIG_KEY15_13_15            10
// Values 11-15 would be included similarly

/*
 * Example ATECC608A Configuration (128 bytes).
 * All values shown in hex. Comments on the right show field name in parentheses.
 */
static const uint8_t config[128] = 
{
    // Block 0 (Bytes 0..31)
    0x00, 0x00, 0x00, 0x00,   // SN[0..3]; SN[2..3] are typically unique from factory. Never read.
    0x00, 0x00, 0x00, 0x00,   // RevNum (Bytes 4..7) => Example only, often {0x00,0x00,0x60,0x02}
    0x00, 0x00, 0x00, 0x00,   // SN[4..7]; “EE EE EE EE” is placeholder. Typically unique from factory
    0x00,                     // AES_Enable=0 (Byte 13)
    0x00,                     // I2C_Enable=1 => uses I2C (Byte 14)
    0x00,                     // Reserved (Byte 15)
    0xC0,                     // I2C_Address=0xC0 => 7-bit address=0x60 (Byte 16)
    0x00,                     // Reserved (Byte 17)
    0x00,                     // CountMatch=0 => disabled (Byte 18)
    0x00,                     // ChipMode=0 => Normal (Byte 19)

    // SlotConfig[0..7], each slot 2 bytes => Bytes 20..35
    // Slot 0: ECC Private Key for P256, sign external
    0x83, 0x02,   // SlotConfig[0]: = 0x0283 (little-endian)
                  //   readKey=0x3 (bits0..3 => 0b0011 => signExt=1, signInt=1, ECDH=0)
                  //   isSecret=1 (bit7)
                  //   writeConfig=0b0010=2 => “Never” for normal writes
                  //   (Private keys use GenKey/PrivWrite, so normal writes are blocked.)

    0xFF, 0xFF,   // SlotConfig[1]
    0xFF, 0xFF,   // SlotConfig[2]
    0xFF, 0xFF,   // SlotConfig[3]
    0xFF, 0xFF,   // SlotConfig[4]
    0xFF, 0xFF,   // SlotConfig[5]
    0xFF, 0xFF,   // SlotConfig[6]
    0xFF, 0xFF,   // SlotConfig[7]  => all set to 0xFFFF => “Never read, never write,” etc.

    // Bytes 36..51 => SlotConfig[8..15]
    0xFF, 0xFF,   // SlotConfig[8] 
    0xFF, 0xFF,   // SlotConfig[9]
    0xFF, 0xFF,   // SlotConfig[10]
    0xFF, 0xFF,   // SlotConfig[11]
    0xFF, 0xFF,   // SlotConfig[12]
    0xFF, 0xFF,   // SlotConfig[13]
    0xFF, 0xFF,   // SlotConfig[14]
    0xFF, 0xFF,   // SlotConfig[15] => all “FF FF” for a default lock-down

    // Bytes 52..83
    0x00, 0x00, 0x00, 0x00,   // Counter[0..1], 8 bytes total (Bytes 52..59). All zeros initially
    0x00, 0x00, 0x00, 0x00,
    0x00,                     // UseLock=0 (Byte 68)
    0x00,                     // VolatileKeyPermission=0 (Byte 69)
    0x00, 0x00,               // SecureBoot=0 => disabled (Bytes 70..71)
    0xF0,                     // KdfIvLoc=0xF0 => HKDF special IV check disabled (Byte 72)
    0x00, 0x00,               // KdfIvStr=0 (Bytes 73..74)
    0x00, 0x00, 0x00, 0x00, 0x00,  // Reserved (Bytes 75..79)
    0x00, 0x00, 0x00, 0x00, 0x00,  // Reserved (Bytes 80..84)
    0x00, 0x00, 0x00,             // Reserved (Bytes 85..87)

    // LockValue=0x55 => unlocked data/OTP; LockConfig=0x55 => unlocked config
    0x55, 0x55,   // (Bytes 86..87 => 0x55 each => both zones unlocked)

    // SlotLocked bits => 0x0000 => no slots individually locked
    0x00, 0x00,   // (Bytes 88..89)

    // ChipOptions => enable ECDH / KDF outputs if you want. We’ll default to 0
    0x00, 0x00,   // Bytes 90..91 => 0 => no IO protection, no KDF/AES if you like

    // X509format => all zero
    0x00, 0x00, 0x00, 0x00,   // Bytes 92..95 => all zero => no X509 validation format

    // KeyConfig for each slot => 2 bytes per slot => Bytes 96..127
    // Slot 0: Private=1, PubInfo=1, KeyType=P256(=4), Lockable=1, ReqRandom=1
    //         no Auth, no X509 checking => see below
    0x73, 0x00,   // KeyConfig[0] = 0x0073 in little-endian
                  //  bit0=Private=1
                  //  bit1=PubInfo=1
                  //  bits2..4=KeyType=4 => P256
                  //  bit5=Lockable=1
                  //  bit6=ReqRandom=1
                  //  bit7=ReqAuth=0
                  //  bits8..11=AuthKey=0
                  //  bit12=PersistentDisable=0
                  //  bit13=RFU=0
                  //  bits14..15=X509id=0

    0xFF, 0xFF,   // KeyConfig[1]
    0xFF, 0xFF,   // KeyConfig[2]
    0xFF, 0xFF,   // KeyConfig[3]
    0xFF, 0xFF,   // KeyConfig[4]
    0xFF, 0xFF,   // KeyConfig[5]
    0xFF, 0xFF,   // KeyConfig[6]
    0xFF, 0xFF,   // KeyConfig[7]
    0xFF, 0xFF,   // KeyConfig[8]
    0xFF, 0xFF,   // KeyConfig[9]
    0xFF, 0xFF,   // KeyConfig[10] e.g. store a public key if you wish
    0xFF, 0xFF,   // KeyConfig[11]
    0xFF, 0xFF,   // KeyConfig[12]
    0xFF, 0xFF,   // KeyConfig[13]
    0xFF, 0xFF,   // KeyConfig[14]
    0xFF, 0xFF    // KeyConfig[15]
};