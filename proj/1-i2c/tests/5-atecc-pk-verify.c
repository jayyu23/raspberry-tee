#include "rpi.h"
#include "i2c.h"
#include "atecc608a.h"

void notmain(void) {
    uart_init();
    printk("ATECC608A Detection Test for %x\n", ATECC608A_ADDR);
    
    i2c_init();
    printk("I2C initialized\n");
    
    // Try to wake up the device
    printk("Attempting to wake ATECC608A...\n");
    
    // Send wake pulse (byte 0x00 at 100kHz)
    // uint8_t wake_cmd = 0x00;
    // int wake_result = i2c_write(0x00, &wake_cmd, 1);
    atecc608a_wakeup();
    // uint8_t recv_buf[4]; // Need at least 4 bytes for status response
    // int result = i2c_read(ATECC608A_ADDR, recv_buf, 4);
    // atecc608a_get_revision_info();

    uint8_t pubkey[64];
    atecc608a_pubkey(0, pubkey);
    printk("Public key: ");
    for (int i = 0; i < 64; i++) {
        printk("%x, ", pubkey[i]);
    }
    printk("\n");
    
    // uint8_t msg[] = "hi";
    // message = "hi"
    // SHA 256 hash of "hi"
    uint8_t msg[] = {
        0x8f, 0x43, 0x43, 0x46, 0x64, 0x8f, 0x6b, 0x96,
        0xdf, 0x89, 0xdd, 0xa9, 0x01, 0xc5, 0x17, 0x6b,
        0x10, 0xa6, 0xd8, 0x39, 0x61, 0xdd, 0x3c, 0x1a,
        0xc8, 0x8b, 0x59, 0xb2, 0xdc, 0x32, 0x7a, 0xa4
    };

    uint8_t signature[64];
    atecc608a_sign(0, msg, signature);
    printk("Signature: ");
    for (int i = 0; i < 64; i++) {
        printk("%x, ", signature[i]);
    }
    printk("\n");

    // Verify the signature using the public key
    printk("--------------------------------\n");
    printk("Verifying signature...\n");
    int result = atecc608a_verify(msg, signature, pubkey);
    if (result == 0) {
        printk("Signature verified successfully!\n");
    } else {
        printk("Signature verification failed!\n");
    }

    // Corrupt signature with a single bit flip
    printk("--------------------------------\n");
    printk("Corrupting signature...\n");
    signature[0] ^= 1;
    result = atecc608a_verify(msg, signature, pubkey);
    if (result == 0) {
        printk("Error: Corrupt signature verified successfully!\n");
    } else {
        printk("Corrupt signature failed as expected with result %d\n", result);
    }

    clean_reboot();
}