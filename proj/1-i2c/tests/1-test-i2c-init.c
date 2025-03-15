#include "rpi.h"
#include "i2c.h"

void notmain(void) {
    uart_init();
    printk("I2C Initialization Test\n");
    
    i2c_init();
    
    printk("I2C initialization complete\n");
    
    printk("SUCCESS: I2C init test completed\n");
    clean_reboot();
}