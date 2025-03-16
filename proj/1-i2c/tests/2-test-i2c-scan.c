#include "rpi.h"
#include "i2c.h"

void notmain(void) {
    uart_init();
    printk("I2C Bus Scanner Test\n");
    
    // Initialize I2C
    i2c_init();
    printk("I2C initialized, scanning for devices...\n");
    
    int found = 0;
    
    // Scan all possible 7-bit I2C addresses (0-127)
    for (uint8_t addr = 1; addr < 128; addr++) {
        // Try to write 0 bytes to the device (just checking ACK)
        // This is a standard way to detect I2C devices
        uint8_t data = 0;
        if (i2c_write(addr, &data, 0) >= 0) {
            printk("Device found at address: 0x%02x\n", addr);
            found++;
        }
        
        // Short delay between scans
        delay_ms(1);
    }
    
    if (found > 0) {
        printk("Scan complete. Found %d devices.\n", found);
        printk("SUCCESS: I2C scan found devices\n");
    } else {
        printk("Scan complete. No devices found.\n");
        printk("WARNING: No I2C devices detected. Check connections.\n");
    }
    
    clean_reboot();
}