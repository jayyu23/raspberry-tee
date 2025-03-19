#include "i2c.h"

// Write data to a device with a specific word address
int i2c_write_with_addr(uint8_t dev_addr, uint8_t word_addr, uint8_t data[], unsigned nbytes) {
    uint32_t status;
    
    // Check if the bus is active
    status = GET32(I2C_S);
    if (status & I2C_S_TA) {
        printk("I2C bus is still active\n");
        return -1;
    }
    
    // Clear FIFO
    PUT32(I2C_C, GET32(I2C_C) | I2C_C_CLEAR);
    dev_barrier();
    
    // Clear status flags
    PUT32(I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);
    dev_barrier();
    
    // Set slave address
    PUT32(I2C_A, dev_addr);
    dev_barrier();
    
    // Set data length (word_addr + actual data)
    PUT32(I2C_DLEN, nbytes + 1);
    dev_barrier();
    
    // Send word address first
    PUT32(I2C_FIFO, word_addr);
    
    // Then send the actual data
    for (unsigned i = 0; i < nbytes; i++) {
        PUT32(I2C_FIFO, data[i]);
    }
    
    // Start write transfer
    PUT32(I2C_C, GET32(I2C_C) | I2C_C_ST);
    dev_barrier();
    
    // Wait for transfer to complete
    while (1) {
        status = GET32(I2C_S);
        if (status & I2C_S_DONE)
            break;
        
        if (status & (I2C_S_ERR | I2C_S_CLKT)) {
            printk("I2C error during write with addr: %x\n", status);
            return -1;
        }
    }
    
    // Check for success
    if (status & (I2C_S_ERR | I2C_S_CLKT)) {
        printk("I2C error after write with addr: %x\n", status);
        return -1;
    }
    
    return nbytes + 1;
}

void i2c_init(void) {
    // Configure GPIO pins for I2C function

    gpio_set_function(I2C_SDA, GPIO_FUNC_ALT0);
    gpio_set_function(I2C_SCL, GPIO_FUNC_ALT0);

    // Set pullups
    gpio_set_pullup(I2C_SDA);
    gpio_set_pullup(I2C_SCL);
    
    // Set up I2C with 100kHz clock
    // Reset I2C
    PUT32(I2C_C, 0);
    dev_barrier();
    
    // Clear status flags
    PUT32(I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);
    dev_barrier();
    
    // Set clock divider for 100kHz (assuming 150MHz core clock)
    PUT32(I2C_DIV, 1500);
    dev_barrier();
    
    // Enable I2C
    PUT32(I2C_C, I2C_C_I2CEN);
    dev_barrier();

    // Clock stretching
    PUT32(I2C_CLKT, 0xFFFF);  // Maximum timeout value
    dev_barrier();
    
    printk("I2C initialized\n");
}

int i2c_write(unsigned addr, uint8_t data[], unsigned nbytes) {
    uint32_t status;
    
    // Clear FIFO first
    PUT32(I2C_C, GET32(I2C_C) | I2C_C_CLEAR);
    dev_barrier();
    
    // Clear status flags
    PUT32(I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);
    dev_barrier();
    
    // Set slave address
    PUT32(I2C_A, addr);
    dev_barrier();
    
    // Set data length
    PUT32(I2C_DLEN, nbytes);
    dev_barrier();
    
    // Fill FIFO with data (be careful not to overflow)
    for (unsigned i = 0; i < nbytes && i < 16; i++) {  // 16 is FIFO size
        PUT32(I2C_FIFO, data[i]);
    }
    
    // Start write transfer and set I2C_C_READ bit to 0 (write mode)
    PUT32(I2C_C, (GET32(I2C_C) & ~I2C_C_READ) | I2C_C_ST | I2C_C_I2CEN);
    dev_barrier();
    
    // Wait for transfer to complete or for more FIFO space
    unsigned bytes_sent = (nbytes < 16) ? nbytes : 16;
    
    while (bytes_sent < nbytes || !(GET32(I2C_S) & I2C_S_DONE)) {
        status = GET32(I2C_S);
        
        // Check for errors
        if (status & (I2C_S_ERR | I2C_S_CLKT)) {
            printk("I2C error during write: %x\n", status);
            return -1;
        }
        
        // If FIFO can accept more data and we have more to send
        if ((status & I2C_S_TXD) && bytes_sent < nbytes) {
            PUT32(I2C_FIFO, data[bytes_sent++]);
        }
    }
    
    // Wait for DONE flag
    while (!(GET32(I2C_S) & I2C_S_DONE)) {
        // Just wait
    }
    
    // Check for success
    status = GET32(I2C_S);
    if (status & (I2C_S_ERR | I2C_S_CLKT)) {
        printk("I2C error after write: %x\n", status);
        return -1;
    }
    
    return nbytes;
}

int i2c_read(unsigned addr, uint8_t data[], unsigned nbytes) {
    uint32_t status;
    
    // Check if the bus is active
    status = GET32(I2C_S);
    if (status & I2C_S_TA) {
        printk("I2C bus is still active\n");
        return -1;
    }
    
    // Clear FIFO
    PUT32(I2C_C, GET32(I2C_C) | I2C_C_CLEAR);
    dev_barrier();
    
    // Clear status flags
    PUT32(I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);
    dev_barrier();
    
    // Set slave address
    PUT32(I2C_A, addr);
    dev_barrier();
    
    // Set data length
    PUT32(I2C_DLEN, nbytes);
    dev_barrier();
    
    // Start read transfer
    PUT32(I2C_C, GET32(I2C_C) | I2C_C_ST | I2C_C_READ);
    dev_barrier();
    
    // Read data from FIFO
    for (unsigned i = 0; i < nbytes; i++) {
        printk("Reading data: %d\n", i);
        // Wait for data
        while (!(GET32(I2C_S) & I2C_S_RXD)) {
            status = GET32(I2C_S);
            printk("Status: %x\n", status);
            if (status & (I2C_S_ERR | I2C_S_CLKT)) {
                printk("I2C error during read: %x\n", status);
                return -1;
            }
        }
        
        data[i] = GET32(I2C_FIFO) & 0xFF;
        printk("Read data: %x\n", data[i]);
    }
    
    // Wait for transfer to complete
    while (1) {
        status = GET32(I2C_S);
        if (status & I2C_S_DONE)
            break;
        
        if (status & (I2C_S_ERR | I2C_S_CLKT)) {
            printk("I2C error waiting for completion: %x\n", status);
            return -1;
        }
    }
    
    // Check for success
    if (status & (I2C_S_ERR | I2C_S_CLKT)) {
        printk("I2C error after read: %x\n", status);
        return -1;
    }
    
    return nbytes;
}

// Adding new functions required by the header

void i2c_init_clk_div(unsigned clk_div) {
    // Reset I2C
    PUT32(I2C_C, 0);
    dev_barrier();
    
    // Clear status flags
    PUT32(I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);
    dev_barrier();
    
    // Set clock divider to the provided value
    PUT32(I2C_DIV, clk_div);
    dev_barrier();
    
    // Enable I2C
    PUT32(I2C_C, I2C_C_I2CEN);
    dev_barrier();
    
    printk("I2C initialized with clock divider: %u\n", clk_div);
}

// Track if I2C has been initialized
static int i2c_initialized = 0;

void i2c_init_once(void) {
    if (!i2c_initialized) {
        i2c_init();
        i2c_initialized = 1;
    }
}

// Remove or uncomment in header if you want to keep this function
#if 0
int i2c_write_read(uint8_t addr, const uint8_t *wdata, unsigned wlen, uint8_t *rdata, unsigned rlen) {
    if (i2c_write(addr, wdata, wlen) != wlen) {
        return -1;
    }
    
    // Small delay between write and read
    delay_us(100);
    
    return i2c_read(addr, rdata, rlen);
}
#endif