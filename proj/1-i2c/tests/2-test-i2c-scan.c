#include "rpi.h"
#include "i2c.h"

void my_i2c_init(void) {
    // Configure GPIO pins for I2C function

    gpio_set_function(I2C_SDA, GPIO_FUNC_ALT0);
    gpio_set_function(I2C_SCL, GPIO_FUNC_ALT0);

    gpio_set_pullup(I2C_SDA);
    gpio_set_pullup(I2C_SCL);
    
    // Set up I2C with 100kHz clock
    // Reset I2C
    PUT32(I2C_C, 0);
    dev_barrier();
    
    // Clear status flags
    PUT32(I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);
    dev_barrier();
    
    // Set clock divider for 100kHz (assuming 250MHz core clock)
    PUT32(I2C_DIV, 2500);
    dev_barrier();
    
    // Enable I2C
    PUT32(I2C_C, I2C_C_I2CEN);
    dev_barrier();
    
    printk("I2C initialized\n");
}


void notmain(void) {
    uart_init();
    printk("I2C Bus Scanner\n");
    
    // Initialize I2C
    delay_ms(100);
    my_i2c_init();
    printk("I2C initialized, scanning for devices...\n");


    
    delay_ms(100);
    
    int found = 0;
    
    // Scan all possible 7-bit I2C addresses (0-127)
    for (uint8_t addr = 1; addr < 128; addr++) {
        // First, clear any pending errors
        PUT32(I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);
        dev_barrier();
        
        // Set slave address
        PUT32(I2C_A, addr);
        dev_barrier();
        
        // Set data length (0 - just checking for ACK)
        PUT32(I2C_DLEN, 0);
        dev_barrier();
        
        // Start a write transfer (no data, just the address)
        // Use only the necessary flags and avoid READ flag for address probing
        PUT32(I2C_C, I2C_C_ST | I2C_C_CLEAR); // Clear FIFO, start transfer
        dev_barrier();
        
        // Wait for transfer to complete with timeout
        unsigned timeout = 10000; // Reasonable timeout value
        while (!(GET32(I2C_S) & I2C_S_DONE) && timeout > 0) {
            timeout--;
            // Very short delay to prevent tight loop
            for (volatile int i = 0; i < 50; i++);
        }
        
        // Check if we timed out
        if (timeout == 0) {
            printk("Timeout on address 0x%x\n", addr);
            // Reset the I2C controller
            my_i2c_init();
            continue;
        }
        
        // Check status register
        uint32_t status = GET32(I2C_S);
        
        // If DONE is set and ERR is not set, the device acknowledged
        if ((status & I2C_S_DONE) && !(status & I2C_S_ERR)) {
            printk("Device found at address: 0x%02x\n", addr);
            found++;
        }
        
        // Clear the DONE bit
        PUT32(I2C_S, I2C_S_DONE);
        dev_barrier();
        delay_ms(5);
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