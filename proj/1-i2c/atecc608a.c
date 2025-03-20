#include "atecc608a.h"
#include "i2c.h"


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
    gpio_set_function(I2C_SDA, GPIO_FUNC_OUTPUT);

    gpio_write(I2C_SDA, 0);
    delay_us(80);  // 80 us ensures meeting tWLO (min ~60us)

    gpio_write(I2C_SDA, 1);

    // Switch SDA back to I2C function
    gpio_set_function(I2C_SDA, GPIO_FUNC_ALT0);

    // Wait for tWHI (at least 150 us) before communication
    delay_us(150);
    printk("Wake pulse sent\n");
    i2c_init();
    // Now immediately perform I2C read from the ATECC608A
    uint8_t buffer[4];
    if (i2c_read(ATECC608A_ADDR, buffer, sizeof(buffer)) < 0) {
        // printk("I2C error during read: 0xb0000151\n");
    } else {
        if (buffer[0] == 0x04 &&
            buffer[1] == 0x11 &&
            buffer[2] == 0x33 &&
            buffer[3] == 0x43) {
            printk("Wake successful\n");
        } else {
            printk("Unexpected wake response\n");
        }
    }
    return 0;
}

// Put ATECC608A to sleep
int atecc608a_sleep(void) {
    uint8_t sleep_cmd = 0x01;  // Sleep opcode
    return i2c_write(ATECC608A_ADDR, &sleep_cmd, 1);
}

static uint16_t calculate_crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0;
    size_t i, j;
    
    for (i = 0; i < length; i++) {
        crc ^= (data[i] << 8);
        for (j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x8005;
            } else {
                crc = crc << 1;
            }
        }
    }
    
    return crc;
}

// Send a command to ATECC608A and get response
static int atecc608a_send_command(uint8_t cmd, uint8_t p1, uint16_t p2, 
                                 const uint8_t *data, uint8_t data_len,
                                 uint8_t *response, uint8_t *response_len, int delay_time_ms) {
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
    
    // Calculate CRC-16 over the entire packet (excluding CRC bytes)
    uint16_t crc = calculate_crc16(packet, count - 2);
    packet[count - 2] = crc & 0xFF;        // CRC LSB
    packet[count - 1] = (crc >> 8) & 0xFF; // CRC MSB
    
    // Send command
    if (i2c_write(ATECC608A_ADDR, packet, count) != count) {
        printk("Failed to send command to ATECC608A\n");
        return -1;
    }
    
    // Wait for processing
    delay_ms(delay_time_ms);  // Adjust based on command
    
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
        printk("ATECC608A command failed with status: %x\n", temp_resp[1]);
        return -1;
    }
    
    return 0;
}

int atecc608a_get_revision_info(void) {
    uint8_t response[4];
    uint8_t response_len = sizeof(response);
    return atecc608a_send_command(ATECC_CMD_INFO, 0, 0, NULL, 0, response, &response_len, 5);
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
    
    if (atecc608a_send_command(ATECC_CMD_INFO, 0, 0, &info_param, 1, response, &response_len, 5) < 0) {
        printk("Failed to get ATECC608A info\n");
        return -1;
    }
    
    printk("ATECC608A initialized successfully\n");
    
    // Put device to sleep to save power
    atecc608a_sleep();
    
    return 0;
}

int atecc608a_random(uint8_t *rand_out) {
    // // Wake up the device
    // if (atecc608a_wakeup() < 0)
    //     return -1;
    // Random command parameters
    uint8_t mode = 0x00;  // Default mode
    uint8_t response[32];
    uint8_t response_len = sizeof(response);
    
    int ret = atecc608a_send_command(ATECC_CMD_RANDOM, mode, 0, NULL, 0, response, &response_len, 50);
    
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