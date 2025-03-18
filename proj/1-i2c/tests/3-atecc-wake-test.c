#include "rpi.h"
#include "i2c.h"

// ATECC608A constants from datasheet
#define ATECC_ADDR      0x36    // Default I2C address (pg. 8)
#define ATECC_WAKE_RESP 0x11    // Expected response after wake

// ATECC608A word address values (section 7.2.1)
#define ATECC_RESET     0x00    // Reset the address counter
#define ATECC_SLEEP     0x01    // Put device in low-power sleep mode
#define ATECC_IDLE      0x02    // Put device in idle mode
#define ATECC_COMMAND   0x03    // Write bytes to input command buffer

// Status/error codes (section 4.3)
#define ATECC_SUCCESS           0x00
#define ATECC_CHECKMAC_FAIL     0x01
#define ATECC_PARSE_ERROR       0x03
#define ATECC_ECC_FAULT         0x05
#define ATECC_SELFTEST_ERROR    0x07
#define ATECC_HEALTH_TEST_ERROR 0x08
#define ATECC_EXECUTION_ERROR   0x0F
#define ATECC_WAKE_SUCCESS      0x11
#define ATECC_WATCHDOG_EXPIRE   0xEE
#define ATECC_COMM_ERROR        0xFF

// Debug print status/error code
void print_status_code(uint8_t code) {
    switch(code) {
        case ATECC_SUCCESS:           printk("SUCCESS: Command executed successfully\n"); break;
        case ATECC_CHECKMAC_FAIL:     printk("ERROR: CheckMac or Verify mismatch\n"); break;
        case ATECC_PARSE_ERROR:       printk("ERROR: Parse error - illegal parameter\n"); break;
        case ATECC_ECC_FAULT:         printk("ERROR: ECC fault during processing\n"); break;
        case ATECC_SELFTEST_ERROR:    printk("ERROR: Self-test error\n"); break;
        case ATECC_HEALTH_TEST_ERROR: printk("ERROR: Health test error (RNG)\n"); break;
        case ATECC_EXECUTION_ERROR:   printk("ERROR: Execution error - invalid state\n"); break;
        case ATECC_WAKE_SUCCESS:      printk("SUCCESS: Device woken up\n"); break;
        case ATECC_WATCHDOG_EXPIRE:   printk("ERROR: Watchdog about to expire\n"); break;
        case ATECC_COMM_ERROR:        printk("ERROR: Communication error\n"); break;
        default:                      printk("UNKNOWN status code: %x\n", code);
    }
}

// Print I2C status register
void print_i2c_status(void) {
    uint32_t status = GET32(I2C_S);
    
    printk("I2C Status Register: %x\n", status);
    if (status & I2C_S_TA)   printk("  - Transfer Active\n");
    if (status & I2C_S_DONE) printk("  - Transfer Done\n");
    if (status & I2C_S_TXW)  printk("  - FIFO Needs Writing\n");
    if (status & I2C_S_RXR)  printk("  - FIFO Needs Reading\n");
    if (status & I2C_S_TXD)  printk("  - FIFO Can Accept Data\n");
    if (status & I2C_S_RXD)  printk("  - FIFO Contains Data\n");
    if (status & I2C_S_TXE)  printk("  - FIFO Empty\n");
    if (status & I2C_S_RXF)  printk("  - FIFO Full\n");
    if (status & I2C_S_ERR)  printk("  - ERROR: No ACK received\n");
    if (status & I2C_S_CLKT) printk("  - ERROR: Clock stretch timeout\n");
}

// Reset the I2C controller
void i2c_reset(void) {
    // Disable I2C completely
    PUT32(I2C_C, 0);
    dev_barrier();
    
    // Force GPIO control of pins
    gpio_set_function(I2C_SDA, GPIO_FUNC_OUTPUT);
    gpio_set_function(I2C_SCL, GPIO_FUNC_OUTPUT);
    
    // Generate a clean bus state with a STOP condition
    gpio_write(I2C_SDA, 0);
    gpio_write(I2C_SCL, 1);
    delay_us(10);
    gpio_write(I2C_SDA, 1);
    delay_us(10);
    
    // Return control to I2C peripheral with pullups
    gpio_set_function(I2C_SDA, GPIO_FUNC_ALT0);
    gpio_set_function(I2C_SCL, GPIO_FUNC_ALT0);
    gpio_set_pullup(I2C_SDA);
    gpio_set_pullup(I2C_SCL);
    delay_ms(1);
    
    // Reinitialize I2C with longer timeouts and slower clock
    PUT32(I2C_C, 0);
    PUT32(I2C_S, 0xFFFFFFFF);  // Clear all status flags
    PUT32(I2C_DIV, 5000);      // 50kHz (assuming 250MHz core)
    PUT32(I2C_CLKT, 0xFFFF);   // Maximum timeout
    PUT32(I2C_C, I2C_C_I2CEN); // Enable I2C
    dev_barrier();
    
    delay_ms(10);  // Allow controller to stabilize
}

// Method 1: Wake the device using a manual SDA low pulse
// According to section 7.1.1 of the datasheet
int manual_wake(void) {
    printk("Using manual wake method (GPIO control of SDA)...\n");
    
    // Reset I2C controller
    i2c_reset();
    
    // Switch SDA to GPIO output mode
    gpio_set_function(I2C_SDA, GPIO_FUNC_OUTPUT);
    
    // Create wake pulse - drive SDA low for > 60µs (tWLO)
    gpio_write(I2C_SDA, 0);  // SDA low
    printk("SDA pulled low for wake pulse\n");
    delay_us(100);           // Hold for 100µs (exceeding 60µs minimum)
    
    // Release SDA high
    gpio_write(I2C_SDA, 1);  // SDA high
    printk("SDA released high\n");
    
    // Wait tWHI (wake high delay) - at least 1500µs per datasheet
    delay_ms(2);  // 2ms is safely above 1.5ms minimum
    
    // Switch back to I2C function
    gpio_set_function(I2C_SDA, GPIO_FUNC_ALT0);
    gpio_set_function(I2C_SCL, GPIO_FUNC_ALT0);
    printk("GPIO pins switched back to I2C mode\n");
    
    // Re-initialize I2C
    // i2c_init();
    
    // Send a read request to get the wake response
    uint8_t response[4] = {0};
    printk("Reading wake response...\n");
    int result = i2c_read(ATECC_ADDR, response, 4);
    
    if (result < 0) {
        printk("ERROR: Failed to read wake response (I2C error)\n");
        print_i2c_status();
        return 0;
    }
    
    printk("Wake response: %x %x %x %x\n", 
           response[0], response[1], response[2], response[3]);
    
    // Per datasheet section 4.3, successful wake returns 0x11
    if (response[0] == ATECC_WAKE_SUCCESS) {
        print_status_code(response[0]);
        return 1;
    } else {
        print_status_code(response[0]);
        return 0;
    }
}

// Method 2: Wake the device using slow I2C transmission of 0x00
// As suggested in the "Tip" on page 77 of the datasheet
int slow_clock_wake(void) {
    printk("Using slow clock wake method (0x00 at 100kHz)...\n");
    
    // Reset I2C controller
    i2c_reset();
    
    // Save current baudrate for restoration later
    uint32_t saved_baudrate = GET32(I2C_DIV);
    
    // Set clock to 100kHz (or lower) for wake sequence
    // Formula: baudrate = core_clock / (16 + 2*CDIV*4^FEDL)
    PUT32(I2C_DIV, 0x9C4);  // Approx 100kHz with 250MHz core clock
    dev_barrier();
    
    // Send a byte of 0x00 to any address (will act as wake pulse)
    uint8_t dummy = 0x00;
    i2c_write(0x00, &dummy, 1);  // Address is arbitrary, not expecting ACK
    printk("Sent 0x00 byte at 100kHz\n");
    
    // Wait tWHI (wake high delay) - at least 1500µs
    delay_ms(2);
    
    // Restore original baudrate
    PUT32(I2C_DIV, saved_baudrate);
    dev_barrier();
    
    // Read the wake response
    uint8_t response[4] = {0};
    printk("Reading wake response...\n");
    int result = i2c_read(ATECC_ADDR, response, 4);
    
    if (result < 0) {
        printk("ERROR: Failed to read wake response (I2C error)\n");
        print_i2c_status();
        return 0;
    }
    
    printk("Wake response: %x %x %x %x\n", 
           response[0], response[1], response[2], response[3]);
    
    // Check for successful wake response (0x11)
    if (response[0] == ATECC_WAKE_SUCCESS) {
        print_status_code(response[0]);
        return 1;
    } else {
        print_status_code(response[0]);
        return 0;
    }
}

// Try to detect the device (used both before and after wake)
int detect_device(uint8_t addr) {
    printk("Checking for device at address %x...\n", addr);
    
    // Try to read from the device (should return 4 bytes if awake)
    uint8_t buffer[4] = {0};
    int read_result = i2c_read(addr, buffer, 4);
    
    if (read_result >= 0) {
        printk("SUCCESS: Device detected (read operation)\n");
        printk("Response: %x %x %x %x\n", 
               buffer[0], buffer[1], buffer[2], buffer[3]);
               
        if (buffer[0] == ATECC_WAKE_SUCCESS) {
            printk("Device is awake!\n");
        }
        return 1;
    }
    
    // If read fails, try a write with a valid word address
    // This could reset the device address pointer
    uint8_t cmd[1] = {ATECC_RESET};
    int write_result = i2c_write(addr, cmd, 1);
    
    if (write_result >= 0) {
        printk("SUCCESS: Device detected (write operation)\n");
        return 1;
    }
    
    printk("FAILED: No device detected at address %x\n", addr);
    print_i2c_status();
    return 0;
}

// Send device to sleep mode
int sleep_device(void) {
    printk("Sending device to sleep mode...\n");
    
    uint8_t sleep_cmd = ATECC_SLEEP;
    int result = i2c_write(ATECC_ADDR, &sleep_cmd, 1);
    
    if (result < 0) {
        printk("ERROR: Failed to send sleep command\n");
        print_i2c_status();
        return 0;
    }
    
    printk("Sleep command sent successfully\n");
    return 1;
}

// Send device to idle mode
int idle_device(void) {
    printk("Sending device to idle mode...\n");
    
    uint8_t idle_cmd = ATECC_IDLE;
    int result = i2c_write(ATECC_ADDR, &idle_cmd, 1);
    
    if (result < 0) {
        printk("ERROR: Failed to send idle command\n");
        print_i2c_status();
        return 0;
    }
    
    printk("Idle command sent successfully\n");
    return 1;
}

// Perform I2C synchronization recovery as specified in Section 7.2.2
void i2c_synchronization_recovery(void) {
    printk("Performing I2C synchronization recovery...\n");
    
    // Reset I2C controller
    i2c_reset();
    
    // Set SDA and SCL to GPIO outputs
    gpio_set_function(I2C_SDA, GPIO_FUNC_OUTPUT);
    gpio_set_function(I2C_SCL, GPIO_FUNC_OUTPUT);
    
    // Set both high
    gpio_write(I2C_SDA, 1);
    gpio_write(I2C_SCL, 1);
    delay_us(10);
    
    // Generate 9 clock pulses with SDA high
    for (int i = 0; i < 9; i++) {
        gpio_write(I2C_SCL, 0);
        delay_us(10);
        gpio_write(I2C_SCL, 1);
        delay_us(10);
    }
    
    // Generate START condition
    gpio_write(I2C_SDA, 1);
    delay_us(10);
    gpio_write(I2C_SDA, 0);
    delay_us(10);
    
    // Generate STOP condition
    gpio_write(I2C_SCL, 0);
    delay_us(10);
    gpio_write(I2C_SDA, 0);
    delay_us(10);
    gpio_write(I2C_SCL, 1);
    delay_us(10);
    gpio_write(I2C_SDA, 1);
    delay_us(10);
    
    // Return pins to I2C function
    gpio_set_function(I2C_SDA, GPIO_FUNC_ALT0);
    gpio_set_function(I2C_SCL, GPIO_FUNC_ALT0);
    
    // Re-initialize I2C
    i2c_init();
    
    printk("Synchronization recovery complete\n");
}

// Get ATECC608A info using the Info command (0x30 mode 0x00)
int get_device_info(void) {
    printk("Requesting device info...\n");
    
    // Construct Info command packet:
    // Count (1) + Command (1) + Param1 (1) + Param2 (2) + CRC (2)
    uint8_t packet[7] = {
        0x07,       // Count (including count byte and CRC)
        0x30,       // Info command opcode
        0x00,       // Mode = Revision
        0x00, 0x00, // Param2 (2 bytes)
        0x03, 0xD4  // CRC (calculated for the above bytes)
    };
    
    // Send command packet
    uint8_t addr_with_cmd = ATECC_COMMAND;  // Word address 0x03
    i2c_write_with_addr(ATECC_ADDR, addr_with_cmd, packet, 7);
    
    // Device needs time to process the command
    delay_ms(2);
    
    // Read the response (4 bytes for revision info)
    uint8_t response[7] = {0};
    int result = i2c_read(ATECC_ADDR, response, 7);
    
    if (result < 0) {
        printk("ERROR: Failed to read device info\n");
        print_i2c_status();
        return 0;
    }
    
    printk("Info response: ");
    for (int i = 0; i < 7; i++) {
        printk("%x ", response[i]);
    }
    printk("\n");
    
    if (response[0] == 4 && response[1] == 0) {
        printk("Device revision: %x %x %x %x\n", 
               response[1], response[2], response[3], response[4]);
        return 1;
    } else {
        print_status_code(response[1]);
        return 0;
    }
}

void notmain(void) {
    uart_init();
    printk("\n\nATECC608A-TFLXTLS Driver Test\n");
    printk("----------------------------\n");
    
    i2c_init();
    printk("I2C interface initialized\n");
    delay_ms(100);
    
    // Check initial device state
    printk("\n1. CHECKING INITIAL DEVICE STATE\n");
    if (detect_device(ATECC_ADDR)) {
        printk("NOTE: Device is already awake or responsive\n");
    } else {
        printk("Device not responding, needs to be woken up\n");
    }
    
    // Try manual wake method first
    printk("\n2. ATTEMPTING MANUAL WAKE\n");
    int wake_success = manual_wake();
    
    // If manual wake fails, try the slow clock method
    if (!wake_success) {
        printk("\n3. MANUAL WAKE FAILED, TRYING SLOW CLOCK WAKE\n");
        wake_success = slow_clock_wake();
    }
    
    // If both wake methods fail, try synchronization recovery
    if (!wake_success) {
        printk("\n4. BOTH WAKE METHODS FAILED, TRYING SYNCHRONIZATION RECOVERY\n");
        i2c_synchronization_recovery();
        
        // Try manual wake again after synchronization recovery
        printk("\n5. ATTEMPTING MANUAL WAKE AFTER SYNCHRONIZATION\n");
        wake_success = manual_wake();
    }
    
    if (wake_success) {
        printk("\nSUCCESS: DEVICE IS AWAKE\n");
        
        // Get device info
        printk("\n6. READING DEVICE INFO\n");
        get_device_info();
        
        // Send to sleep when done
        printk("\n7. SENDING DEVICE TO SLEEP\n");
        sleep_device();
    } else {
        printk("\nFAILED: COULD NOT WAKE DEVICE\n");
        printk("Please check:\n");
        printk("1. Hardware connections (SDA/SCL/VCC/GND)\n");
        printk("2. Pull-up resistors on SDA/SCL (2.2K-10K ohms)\n");
        printk("3. Power supply to ATECC608A (2.0-5.5V)\n");
        printk("4. Device address (default is 0x6C, check datasheet)\n");
        printk("5. Bus capacitance (<400pF)\n");
    }
    
    clean_reboot();
}