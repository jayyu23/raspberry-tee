#include "rpi.h"
#include "i2c.h"

// ATECC device address
#define ATECC_ADDR 0x6C

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
    printk("Attempting to wake ATECC608A device...\n");
    
    // 1. Reset I2C and clear status
    PUT32(I2C_C, 0);
    dev_barrier();
    PUT32(I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);
    dev_barrier();
    
    // 2. Set up for wake sequence - 100kHz (slow enough for tWLO)
    PUT32(I2C_DIV, 2500);  // Set clock divider for 100kHz
    dev_barrier();
    PUT32(I2C_C, I2C_C_I2CEN);
    dev_barrier();
    
    // 3. Send wake token (general call with 0x00 data)
    PUT32(I2C_A, 0x00);  // General call address (0x00)
    dev_barrier();
    PUT32(I2C_DLEN, 1);  // Send one byte
    dev_barrier();
    PUT32(I2C_FIFO, 0x00);  // Data byte 0x00
    dev_barrier();
    PUT32(I2C_C, I2C_C_I2CEN | I2C_C_ST);  // Start transfer
    dev_barrier();
    
    // Wait for completion
    unsigned timeout = 50000;
    while (!(GET32(I2C_S) & I2C_S_DONE) && timeout > 0) {
        timeout--;
    }
    
    uint32_t wake_status = GET32(I2C_S);
    printk("Wake pulse status: %x\n", wake_status);
    
    // Clear status flags
    PUT32(I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);
    dev_barrier();
    
    // 4. Wait tWHI (1.5ms minimum)
    delay_ms(2);  // Use 2ms to be safe
    
    // 5. First check if the device ACKs its address
    PUT32(I2C_A, ATECC_ADDR);  // Device address 0x6C
    dev_barrier();
    PUT32(I2C_DLEN, 1);  // Just checking for ACK, not actually reading data yet
    dev_barrier();
    PUT32(I2C_C, I2C_C_I2CEN | I2C_C_ST | I2C_C_READ);  // Start read
    dev_barrier();
    
    // Wait with timeout
    timeout = 10000;
    while (!(GET32(I2C_S) & I2C_S_DONE) && timeout > 0) {
        timeout--;
    }
    
    uint32_t ack_status = GET32(I2C_S);
    printk("Address ACK check status: %x\n", ack_status);
    
    // Check if there was an error (meaning no ACK)
    if (ack_status & I2C_S_ERR) {
        printk("Device did not ACK its address - wake failed\n");
        
        // Clear status
        PUT32(I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);
        dev_barrier();
        
        return 0;  // Wake failed
    }
    
    // If we got here, the device ACKed its address
    printk("Device ACKed its address - might be awake\n");
    
    // Clear status before actual read
    PUT32(I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);
    dev_barrier();
    
    // 6. Now try to read the actual wake response
    PUT32(I2C_A, ATECC_ADDR);  // Device address 0x6C
    dev_barrier();
    PUT32(I2C_DLEN, 4);  // Expect 4 bytes
    dev_barrier();
    PUT32(I2C_C, I2C_C_I2CEN | I2C_C_ST | I2C_C_READ);  // Start read
    dev_barrier();
    
    // Wait with timeout
    timeout = 10000;
    while (!(GET32(I2C_S) & I2C_S_DONE) && timeout > 0) {
        timeout--;
    }
    
    uint32_t read_status = GET32(I2C_S);
    printk("Read status: %x\n", read_status);
    
    // Read response bytes
    uint8_t response[4] = {0, 0, 0, 0};
    int bytes_read = 0;
    
    while ((GET32(I2C_S) & I2C_S_RXD) && bytes_read < 4) {
        response[bytes_read] = GET32(I2C_FIFO) & 0xFF;
        bytes_read++;
    }
    
    printk("Bytes read: %d, Response: %x %x %x %x\n", 
           bytes_read, response[0], response[1], response[2], response[3]);
    
    // Success = received the 0x11 wake response
    int success = (bytes_read > 0 && response[0] == 0x11);
    
    // Clear status
    PUT32(I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);
    dev_barrier();
    
    if (success) {
        printk("ATECC608A successfully woken up!\n");
    } else {
        printk("Wake failed. Did not receive 0x11 response.\n");
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