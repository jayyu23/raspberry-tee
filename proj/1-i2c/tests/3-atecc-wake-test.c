#include "rpi.h"
#include "i2c.h"

// ATECC device address
#define ATECC_ADDR 0x00

// ATECC commands
#define ATECC_WAKE 0x00
#define ATECC_SLEEP 0x01

// Function to initialize I2C
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

// Better device detection
int detect_device(uint8_t addr) {
    // Clear any pending errors
    PUT32(I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);
    dev_barrier();
    
    // Set slave address
    PUT32(I2C_A, addr);
    dev_barrier();
    
    // Set data length (1 byte for a more reliable check)
    PUT32(I2C_DLEN, 1);
    dev_barrier();
    
    // Put a dummy byte in FIFO
    PUT32(I2C_FIFO, 0);
    dev_barrier();
    
    // Start a write transfer
    PUT32(I2C_C, I2C_C_I2CEN | I2C_C_ST | I2C_C_CLEAR);
    dev_barrier();
    
    // Wait for transfer to complete with timeout
    unsigned timeout = 10000;
    while (!(GET32(I2C_S) & I2C_S_DONE) && timeout > 0) {
        timeout--;
        if (GET32(I2C_S) & I2C_S_ERR) break;  // Exit early on error
    }
    
    // Get status and check for both DONE and no ERR
    uint32_t status = GET32(I2C_S);
    printk("I2C Status: %x\n", status);
    
    // Clear status flags
    PUT32(I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);
    dev_barrier();
    
    return ((status & I2C_S_DONE) && !(status & I2C_S_ERR));
}

int atecc_wake(void) {
    printk("Attempting to wake ATECC device...\n");
    
    // Reset I2C first
    PUT32(I2C_C, 0);
    dev_barrier();
    PUT32(I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);
    dev_barrier();
    PUT32(I2C_C, I2C_C_I2CEN);
    dev_barrier();
    
    // Save current GPIO function settings
    gpio_func_t i2c_func = GPIO_FUNC_ALT0;
    
    // Configure SDA and SCL as outputs
    gpio_set_output(I2C_SDA);
    gpio_set_output(I2C_SCL);
    
    // Generate wake pulse - hold SDA low for tWLO (60us minimum)
    gpio_write(I2C_SDA, 0);  // SDA low
    gpio_write(I2C_SCL, 1);  // SCL high during wake pulse
    delay_ms(5);            // Hold low for 70us (slightly more than required 60us)
    gpio_write(I2C_SDA, 1);  // SDA high (release)
    
    // Critical: Wait for tWHI BEFORE switching back to I2C function
    delay_ms(2);             // tWHI is 1.5ms, use 2ms to be safe
    
    // Switch back to I2C function
    gpio_set_function(I2C_SDA, i2c_func);
    gpio_set_function(I2C_SCL, i2c_func);
    gpio_set_pullup(I2C_SDA); // Ensure pullups are enabled
    gpio_set_pullup(I2C_SCL);
    
    // Short delay to ensure I2C is stabilized
    delay_ms(10);
    
    // Clear status
    PUT32(I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);
    dev_barrier();
    
    // Use ATECC address (0x00) for wake response
    PUT32(I2C_A, 0x00);
    dev_barrier();
    
    // Set data length for read (4 bytes)
    PUT32(I2C_DLEN, 4);
    dev_barrier();
    
    // Start a read transfer
    PUT32(I2C_C, I2C_C_I2CEN | I2C_C_ST | I2C_C_READ);
    dev_barrier();
    
    // Read the response with better error handling
    uint8_t response[4] = {0, 0, 0, 0};
    int success = 0;
    
    // Wait for transfer to complete with a generous timeout
    unsigned timeout = 100000;  // Very long timeout for possible clock stretching
    while (!(GET32(I2C_S) & I2C_S_DONE) && timeout > 0) {
        timeout--;
        if (GET32(I2C_S) & I2C_S_ERR) {
            printk("I2C error during wake: %x\n", GET32(I2C_S));
            break;
        }
    }
    
    if (timeout == 0) {
        printk("Timeout waiting for wake response\n");
        return 0;
    }
    
    // Check if there's data in the FIFO
    if (GET32(I2C_S) & I2C_S_RXD) {
        // Read all four bytes
        for (int i = 0; i < 4; i++) {
            response[i] = GET32(I2C_FIFO) & 0xFF;
        }
        
        // Log the response
        printk("Wake response: %x %x %x %x\n", 
               response[0], response[1], response[2], response[3]);
               
        // Check if response indicates successful wake (typically 0x04, 0x11, 0x33, 0x43)
        if (response[0] == 0x04 && response[1] == 0x11 && 
            response[2] == 0x33 && response[3] == 0x43) {
            printk("ATECC device successfully woken up!\n");
            success = 1;
        } else {
            printk("Unexpected wake response\n");
            success = 0;
        }
    } else {
        printk("No data received in FIFO\n");
        success = 0;
    }
    
    // Clear status
    PUT32(I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);
    dev_barrier();
    
    return success;
}

void notmain(void) {
    uart_init();
    printk("ATECC Wake-up Test\n");
    
    // Initialize I2C
    delay_ms(100);
    my_i2c_init();
    printk("I2C initialized\n");
    
    delay_ms(100);
    
    // Try to wake the device
    for (int attempt = 1; attempt <= 3; attempt++) {
        printk("\nAttempt %d to wake ATECC device...\n", attempt);
        
        if (atecc_wake()) {
            printk("SUCCESS: ATECC device woken up successfully!\n");
            
            // Now try to detect the device at its normal address
            if (detect_device(ATECC_ADDR)) {
                printk("Device detected at address %x\n", ATECC_ADDR);
            } else {
                printk("WARNING: Device woke up but not detected at address %x\n", ATECC_ADDR);
            }
            
            break;
        } else {
            printk("Failed to wake ATECC device on attempt %d\n", attempt);
            
            if (attempt < 3) {
                printk("Waiting before next attempt...\n");
                delay_ms(100);
            } else {
                printk("All wake attempts failed.\n");
            }
        }
    }
    
    clean_reboot();
}