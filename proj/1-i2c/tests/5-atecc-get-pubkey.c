#include "rpi.h"
#include "i2c.h"
#include "atecc608a.h"

void notmain(void) {
    uart_init();
    printk("ATECC608A Get Public Key Test for %x\n", ATECC608A_ADDR);
    
    i2c_init();
    printk("I2C initialized\n");
    atecc608a_wakeup();

    // Get the public key
    uint8_t pubkey[64];
    atecc608a_pubkey(0, pubkey);
    printk("Public key: ");
    for (int i = 0; i < 64; i++) {
        printk("%x, ", pubkey[i]);
    }
    printk("\n");
    clean_reboot();
}