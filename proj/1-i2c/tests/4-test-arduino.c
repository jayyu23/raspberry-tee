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

    // Send a RANDOM command to the ATECC608A
   uint8_t random_cmd[] = {0x03, 0x1B, 0x00, 0x00};
   i2c_write(ATECC608A_ADDR, random_cmd, sizeof(random_cmd));
   printk("Random command sent\n");
   delay_ms(300);
   uint8_t random_data[32];
   i2c_read(ATECC608A_ADDR, random_data, sizeof(random_data));
   printk("Random data received: ");
   for (int i = 0; i < 32; i++) {
       printk("%x ", random_data[i]);
   }
   printk("\n");
   clean_reboot();
}   