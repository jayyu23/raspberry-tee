#include "rpi.h"
#include "i2c.h"

// ATECC device address
#define ATECC_ADDR 0xC0

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

// Improved wake function
int atecc_wake(void) {
    printk("Attempting to wake ATECC device...\n");
    
    // Reset I2C first
    PUT32(I2C_C, 0);
    dev_barrier();
    PUT32(I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);
    dev_barrier();
    PUT32(I2C_C, I2C_C_I2CEN);
    dev_barrier();
    
    // First, force SDA low for wake pulse by bit-banging
    gpio_func_t i2c_func = GPIO_FUNC_ALT0;
    
    // Configure as outputs
    gpio_set_output(I2C_SDA);
    gpio_set_output(I2C_SCL);
    
    // Generate wake pulse - hold SDA low for exactly 60us (timing is critical)
    gpio_write(I2C_SDA, 0);  // SDA low
    delay_us(60);            // Hold low for 60us (exact timing from datasheet)
    
    // Release the bus
    gpio_write(I2C_SDA, 1);  // SDA high
    gpio_write(I2C_SCL, 1);  // SCL high
    delay_us(1);             // Short delay
    
    // Switch back to I2C function
    gpio_set_function(I2C_SDA, i2c_func);
    gpio_set_function(I2C_SCL, i2c_func);
    
    // Wait for device to wake up (must be at least 2ms)
    delay_ms(2);  // ATECC needs ~2ms to wake up
    
    // For wake response, use address 0x03 (special wake address)
    // Clear status
    PUT32(I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);
    dev_barrier();
    
    // Set device address for wake response (0x03)
    PUT32(I2C_A, 0x03);
    dev_barrier();
    
    // Set data length for read (4 bytes)
    PUT32(I2C_DLEN, 4);
    dev_barrier();
    
    // Start a read transfer (DON'T clear FIFO before reading)
    PUT32(I2C_C, I2C_C_I2CEN | I2C_C_ST | I2C_C_READ);
    dev_barrier();
    
    // Read the response with better error handling
    uint8_t response[4];
    int success = 0;
    
    // Wait for data in FIFO with timeout
    unsigned timeout = 50000;  // Increased timeout for clock stretching
    while (!(GET32(I2C_S) & I2C_S_RXD) && timeout > 0) {
        timeout--;
        if (GET32(I2C_S) & I2C_S_ERR) {
            printk("I2C error detected: 0x%x\n", GET32(I2C_S));
            break;
        }
    }
    
    if (timeout == 0) {
        printk("Timeout waiting for wake response\n");
        return 0;
    }
    
    // Read the byte
    response[0] = GET32(I2C_FIFO) & 0xFF;
    response[1] = GET32(I2C_FIFO) & 0xFF;
    response[2] = GET32(I2C_FIFO) & 0xFF;
    response[3] = GET32(I2C_FIFO) & 0xFF;
    
    // Wait for transfer to complete
    timeout = 10000;
    while (!(GET32(I2C_S) & I2C_S_DONE) && timeout > 0) {
        timeout--;
        for (volatile int i = 0; i < 50; i++);
    }
    
    // Clear DONE bit
    PUT32(I2C_S, I2C_S_DONE);
    dev_barrier();
    
    // Check if response indicates successful wake (typically 0x04, 0x11, 0x33, 0x43)
    if (response[0] == 0x04 && response[1] == 0x11 && 
        response[2] == 0x33 && response[3] == 0x43) {
        printk("ATECC device successfully woken up!\n");
        printk("Wake response: %x %x %x %x\n", 
               response[0], response[1], response[2], response[3]);
        success = 1;
    } else {
        printk("Received unexpected wake response: %x %x %x %x\n", 
               response[0], response[1], response[2], response[3]);
        success = 0;
    }
    
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
    
    // Use better detection
    if (detect_device(ATECC_ADDR)) {
        printk("Device detected at address 0x%x - could be ATECC\n", ATECC_ADDR);
    } else {
        printk("No device detected at address 0x%x\n", ATECC_ADDR);
        printk("This is normal if the device is asleep - trying wake sequence\n");
    }
    
    // Try to wake the device regardless
    if (atecc_wake()) {
        printk("SUCCESS: ATECC device woken up successfully!\n");
    } else {
        printk("ERROR: Failed to wake ATECC device\n");
        printk("Please check connections and verify device address\n");
    }
    
    clean_reboot();
} 