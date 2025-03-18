#include "atecc608a.h"


// Check if ATECC608A is awake
int atecc608a_is_awake(void) {
    // Try to read from the device
    uint8_t response[4];
    int read_result = i2c_read(ATECC608A_ADDR, response, 4);
    
    // If we can read 4 bytes and the first byte is 0x04 (count),
    // and the second byte is 0x11 (wake status), the device is awake
    if (read_result == 4 && response[0] == 0x04 && response[1] == 0x11) {
        return 1;  // Device is awake
    }
    
    // Check for execution status (device is awake but busy)
    if (read_result == 4 && response[0] == 0x04 && 
        (response[1] != 0xFF && response[1] != 0x01)) {
        return 1;  // Device is awake but may be executing a command
    }
    
    // If read fails or returns unexpected values, assume device is asleep
    return 0;
}

// Wake up the ATECC608A
int atecc608a_wakeup(void) {
    // Create a wake pulse by sending a "null" address (0x00)
    // The datasheet recommends sending 0x00 at 100kHz to generate the wake pulse
    uint8_t dummy = 0x00;
    
    // Temporarily slow down I2C clock for wake pulse
    uint32_t saved_div = GET32(I2C_DIV);
    PUT32(I2C_DIV, 2500);  // Approx 100kHz assuming 250MHz core
    
    // Send 0x00 to address 0x00 (no ACK expected, just generating wake pulse)
    i2c_write(0x00, &dummy, 1);
    
    // Restore original clock speed
    PUT32(I2C_DIV, saved_div);
    
    // Wait tWHI (wake high delay) - at least 1500μs per datasheet
    delay_ms(2);  // Safe value above minimum
    
    // Now check if device is awake by reading the wake response
    uint8_t response[4];
    if (i2c_read(ATECC608A_ADDR, response, 4) != 4) {
        printk("Failed to read wake response\n");
        return -1;
    }
    
    // Check for expected wake response (0x04, 0x11, ...)
    if (response[0] != 0x04 || response[1] != 0x11) {
        printk("Unexpected wake response: %02x %02x %02x %02x\n", 
               response[0], response[1], response[2], response[3]);
        return -1;
    }
    
    return 0;
}

// Put ATECC608A to sleep
int atecc608a_sleep(void) {
    uint8_t sleep_cmd = 0x01;  // Sleep opcode
    return i2c_write(ATECC608A_ADDR, &sleep_cmd, 1);
}

// Send a command to ATECC608A and get response
static int atecc608a_send_command(uint8_t cmd, uint8_t p1, uint16_t p2, 
                                 const uint8_t *data, uint8_t data_len,
                                 uint8_t *response, uint8_t *response_len) {
    // Packet structure:
    // [count][cmd][param1][param2L][param2H][data...][CRC16L][CRC16H]
    uint8_t packet[64];
    uint8_t count = 8 + data_len;  // count includes count byte + 7 bytes overhead + data
    
    // Build packet
    packet[0] = count;
    packet[1] = cmd;
    packet[2] = p1;
    packet[3] = p2 & 0xFF;
    packet[4] = (p2 >> 8) & 0xFF;
    
    // Copy data if present
    if (data && data_len > 0) {
        for (int i = 0; i < data_len; i++) {
            packet[5 + i] = data[i];
        }
    }
    
    // CRC calculation would go here (simplified for now)
    packet[count - 2] = 0xAA;  // Placeholder for CRC
    packet[count - 1] = 0xBB;  // Placeholder for CRC
    
    // Send command
    if (i2c_write(ATECC608A_ADDR, packet, count) != count) {
        printk("Failed to send command to ATECC608A\n");
        return -1;
    }
    
    // Wait for processing
    delay_ms(5);  // Adjust based on command
    
    // Read response
    uint8_t temp_resp[64];
    int resp_len = i2c_read(ATECC608A_ADDR, temp_resp, 1);
    if (resp_len != 1) {
        printk("Failed to read response length\n");
        return -1;
    }
    
    // Read the rest of the response
    resp_len = temp_resp[0];
    if (resp_len > 1) {
        if (i2c_read(ATECC608A_ADDR, temp_resp + 1, resp_len - 1) != resp_len - 1) {
            printk("Failed to read complete response\n");
            return -1;
        }
    }
    
    // Copy response if buffer provided
    if (response && response_len) {
        // Skip count byte and status byte
        int copy_len = resp_len - 3;  // -3 for count, status, and CRC
        if (copy_len > *response_len)
            copy_len = *response_len;
        
        for (int i = 0; i < copy_len; i++) {
            response[i] = temp_resp[i + 2];  // Skip count and status
        }
        
        *response_len = copy_len;
    }
    
    // Check status
    if (temp_resp[1] != 0x00) {
        printk("ATECC608A command failed with status: %02x\n", temp_resp[1]);
        return -1;
    }
    
    return 0;
}

int atecc608a_init(void) {
    // Initialize I2C
    i2c_init();
    
    // Wake up the device
    if (atecc608a_wakeup() < 0) {
        printk("Failed to wake ATECC608A\n");
        return -1;
    }
    
    // Read device info to verify communication
    uint8_t info_param = 0x00;
    uint8_t response[4];
    uint8_t response_len = sizeof(response);
    
    if (atecc608a_send_command(ATECC_CMD_INFO, 0, 0, &info_param, 1, response, &response_len) < 0) {
        printk("Failed to get ATECC608A info\n");
        return -1;
    }
    
    printk("ATECC608A initialized successfully\n");
    
    // Put device to sleep to save power
    atecc608a_sleep();
    
    return 0;
}

int atecc608a_random(uint8_t *rand_out) {
    // Wake up the device
    if (atecc608a_wakeup() < 0)
        return -1;
    
    // Random command parameters
    uint8_t mode = 0x00;  // Default mode
    uint8_t response[32];
    uint8_t response_len = sizeof(response);
    
    int ret = atecc608a_send_command(ATECC_CMD_RANDOM, mode, 0, NULL, 0, response, &response_len);
    
    // Put device to sleep
    atecc608a_sleep();
    
    if (ret < 0)
        return -1;
    
    // Copy random bytes to output buffer
    for (int i = 0; i < 32; i++) {
        rand_out[i] = response[i];
    }
    
    return 0;
}

int atecc608a_sign(uint8_t key_id, const uint8_t *msg, uint8_t *signature) {
    // Implementation would require several steps:
    // 1. Load the message digest into TempKey using NONCE command
    // 2. Sign the digest using SIGN command
    // For brevity, this is simplified
    
    printk("Signing with key %d (not fully implemented)\n", key_id);
    return -1;  // Not fully implemented in this scaffold
}

int atecc608a_verify(uint8_t key_id, const uint8_t *msg, const uint8_t *signature) {
    // Implementation would require:
    // 1. Load the message digest using NONCE command
    // 2. Verify the signature using VERIFY command
    // For brevity, this is simplified
    
    printk("Verifying with key %d (not fully implemented)\n", key_id);
    return -1;  // Not fully implemented in this scaffold
}