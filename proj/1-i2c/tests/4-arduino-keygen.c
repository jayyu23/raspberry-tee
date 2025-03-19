#include "rpi.h"
#include "i2c.h"
#include "atecc608a.h"

void notmain(void) {
    uart_init();
    printk("ATECC608A Detection Test for %x\n", ATECC608A_ADDR);
    
    i2c_init();
    // PUT32(I2C_BASE + 0x1C, 0xFFFF);  // Maximum timeout value
    dev_barrier();
    printk("I2C initialized\n");

    // Send a INFO command to the ATECC608A
    uint8_t info_cmd[] = {0x03, 0x30, 0x00, 0x00};
    i2c_write(ATECC608A_ADDR, info_cmd, sizeof(info_cmd));
    printk("INFO command sent\n");
    delay_ms(300);
    uint8_t info_data[4];
    i2c_read(ATECC608A_ADDR, info_data, sizeof(info_data));
    printk("INFO data received: ");
    for (int i = 0; i < 4; i++) {
        printk("%x ", info_data[i]);
    }
    printk("\n");

    // Send a GENKEY command to the ATECC608A
    printk("Sending GENKEY command...\n");
    uint8_t slot = 0; // Use key slot 0
    uint8_t genkey_cmd[] = {0x07, 0x40, slot, 0x00, 0x00, 0x00, 0x00, 0x00};
    i2c_write(ATECC608A_ADDR, genkey_cmd, sizeof(genkey_cmd));
    printk("GENKEY command sent to slot %d\n", slot);
    
    // Wait for processing - ATECC608A needs time to "generate" the key
    delay_ms(500);
    
    // Read the public key (64 bytes for P256 curve - X and Y coordinates)
    uint8_t pubkey[64];
    i2c_read(ATECC608A_ADDR, pubkey, sizeof(pubkey));
    
    // Display the public key
    printk("Public key received:\n");
    printk("X coordinate: ");
    for (int i = 0; i < 32; i++) {
        printk("%x", pubkey[i]);
        if ((i + 1) % 8 == 0) printk(" ");
    }
    printk("\n");
    
    uint8_t pubkey_cmd_x[] = {0x03, 0x23, 0x00, 0x00};
    i2c_write(ATECC608A_ADDR, pubkey_cmd_x, sizeof(pubkey_cmd_x));
    delay_ms(500);
    uint8_t pubkey_x[32];
    i2c_read(ATECC608A_ADDR, pubkey_x, sizeof(pubkey_x));
    printk("X coordinate: ");
    for (int i = 0; i < 32; i++) {
        printk("%x", pubkey_x[i]);
    }
    printk("\n");

    uint8_t pubkey_cmd_y[] = {0x03, 0x23, 0x00, 0x01}; // In our Arduino code, we read the Y coordinate from the same address as the X coordinate
    i2c_write(ATECC608A_ADDR, pubkey_cmd_y, sizeof(pubkey_cmd_y));
    delay_ms(500);
    uint8_t pubkey_y[32];
    i2c_read(ATECC608A_ADDR, pubkey_y, sizeof(pubkey_y));
    printk("Y coordinate: ");
    for (int i = 0; i < 32; i++) {
        printk("%x", pubkey_y[i]);
    }
    printk("\n");


   clean_reboot();
}   