#ifndef __ATECC608A_H__
#define __ATECC608A_H__

#include "i2c.h"

// ATECC608A I2C address (7-bit)
#define ATECC608A_ADDR 0x60

// ATECC608A commands
#define ATECC_CMD_CHECKMAC    0x28
#define ATECC_CMD_COUNTER     0x24
#define ATECC_CMD_DERIVEKEY   0x1C
#define ATECC_CMD_ECDH        0x43
#define ATECC_CMD_GENDIG      0x15
#define ATECC_CMD_GENKEY      0x40
#define ATECC_CMD_INFO        0x30
#define ATECC_CMD_LOCK        0x17
#define ATECC_CMD_MAC         0x08
#define ATECC_CMD_NONCE       0x16
#define ATECC_CMD_PRIVWRITE   0x46
#define ATECC_CMD_RANDOM      0x1B
#define ATECC_CMD_READ        0x02
#define ATECC_CMD_SIGN        0x41
#define ATECC_CMD_SHA         0x47
#define ATECC_CMD_VERIFY      0x45
#define ATECC_CMD_WRITE       0x12

// Initialize the ATECC608A
int atecc608a_init(void);

// Generate a random number
int atecc608a_random(uint8_t *rand_out);

// Sign a message digest using a private key
int atecc608a_sign(uint8_t key_id, const uint8_t *msg, uint8_t *signature);

// Verify a signature
int atecc608a_verify(uint8_t key_id, const uint8_t *msg, const uint8_t *signature);

#endif