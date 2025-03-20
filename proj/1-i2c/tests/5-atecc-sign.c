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
        printk("%x ", pubkey[i]);
    }
    printk("\n");
    
    uint8_t msg[] = "Hello, world!";
    uint8_t signature[64];
    atecc608a_sign(0, msg, signature);
    printk("Signature: ");
    for (int i = 0; i < 64; i++) {
        printk("%x ", signature[i]);
    }
    printk("\n");
    clean_reboot();
}