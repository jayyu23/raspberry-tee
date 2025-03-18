#include "rpi.h"
#include "i2c.h"

void notmain(void) {
    uart_init();
    printk("Hello, world!\n");
}