#include "rpi.h"
#include "i2c.h"
#include "atecc608a.h"

void notmain(void) {
    uart_init();
    printk("ATECC608A Detection Test for %x\n", ATECC608A_ADDR);
    
    i2c_init();
    printk("I2C initialized\n");

    // Send a RANDOM command to the ATECC608A
   uint8_t random_cmd[] = {0x03, 0x00, 0x1B, 0x00, 0x00};
   i2c_write(ATECC608A_ADDR, random_cmd, sizeof(random_cmd));


   // uint8_t random_data[32];
   uint8_t random_result;
   // int random_result = atecc608a_random(random_data);
   // printk("Random command result: %d\n", random_result);

    // i2c_write(ATECC608A_ADDR, &random_cmd, 1);
    
    
    clean_reboot();
}   