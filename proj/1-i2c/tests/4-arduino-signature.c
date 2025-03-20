#include "rpi.h"
#include "i2c.h"
#include "atecc608a.h"

// Write a command to the ATECC608A and read the response   
int i2c_write_cmd(uint8_t *cmd, size_t cmd_size, 
                  uint8_t *response, size_t response_size,
                  unsigned delay_ms_time) {
    i2c_write(ATECC608A_ADDR, cmd, cmd_size);
    delay_ms(delay_ms_time);
    i2c_read(ATECC608A_ADDR, response, response_size);
    return 0;
}

void notmain(void) {
    uart_init();
    printk("ATECC608A Detection Test for %x\n", ATECC608A_ADDR);
    
    i2c_init();
    dev_barrier();
    printk("I2C initialized\n");

    // Send a INFO command to the ATECC608A
    uint8_t info_cmd[] = {0x03, 0x30, 0x00, 0x00};
    uint8_t info_data[4];
    i2c_write_cmd(info_cmd, sizeof(info_cmd), info_data, sizeof(info_data), 300);
    printk("INFO command sent\n");
    // delay_ms(300);
    // uint8_t info_data[4];
    // i2c_read(ATECC608A_ADDR, info_data, sizeof(info_data));
    printk("INFO data received: ");
    for (int i = 0; i < 4; i++) {
        printk("%x ", info_data[i]);
    }
    printk("\n");

    // Send a GENKEY command to the ATECC608A
    printk("Sending GENKEY command...\n");
    uint8_t slot = 0; // Use key slot 0
    uint8_t genkey_cmd[] = {0x07, 0x40, slot, 0x00, 0x00, 0x00, 0x00, 0x00};

    uint8_t genkey_data[64];
    // Keygen takes around 3.7 seconds
    i2c_write_cmd(genkey_cmd, sizeof(genkey_cmd), genkey_data, sizeof(genkey_data), 5000);

    // i2c_write(ATECC608A_ADDR, genkey_cmd, sizeof(genkey_cmd));
    printk("GENKEY command sent to slot %d\n", slot);

    // Read the public key
    uint8_t pubkey_cmd_x[] = {0x03, 0x23, 0x00, 0x00};
    uint8_t pubkey_x[32];
    i2c_write_cmd(pubkey_cmd_x, sizeof(pubkey_cmd_x), pubkey_x, sizeof(pubkey_x), 500);
    printk("X coordinate: ");
    for (int i = 0; i < 32; i++) {
        printk("%x ", pubkey_x[i]);
    }
    printk("\n");

    uint8_t pubkey_cmd_y[] = {0x03, 0x23, 0x00, 0x01}; // In our Arduino code, we read the Y coordinate from the same address as the X coordinate
    uint8_t pubkey_y[32];
    i2c_write_cmd(pubkey_cmd_y, sizeof(pubkey_cmd_y), pubkey_y, sizeof(pubkey_y), 500);
    printk("Y coordinate: ");
    for (int i = 0; i < 32; i++) {
        printk("%x ", pubkey_y[i]);
    }
    printk("\n");

    // Sign a message
    uint8_t message[] = "Hello, world!"; // For now we assume < 30 bytes
    uint8_t signature[32]; // Get the first 32 bytes of the signature
    uint8_t signature_cmd[] = {0x03, 0x41};
    // append the message to the signature command
    uint8_t signature_payload[sizeof(signature_cmd) + sizeof(message)];
    memcpy(signature_payload, signature_cmd, sizeof(signature_cmd));
    memcpy(signature_payload + sizeof(signature_cmd), message, sizeof(message));
    printk("SIGN command sent\n");

    i2c_write_cmd(signature_payload, sizeof(signature_payload), signature, sizeof(signature), 5000);
    printk("Signature: ");
    for (int i = 0; i < 32; i++) {
        printk("%x ", signature[i]);
    }
    printk("\n");

   clean_reboot();
}   