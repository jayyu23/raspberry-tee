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

// See datasheet pg.56, follows polynomial 0x8005
uint16_t calculate_crc16(size_t length, const uint8_t *data)
{
    // printk("Calculating CRC-16 for %d bytes\n", length);
    // printk("Data: ");
    // for (int i = 0; i < length; i++) {
    //     printk("%x ", data[i]);
    // }
    // printk("\n");
    size_t counter;
    uint16_t crc_register = 0;
    uint16_t polynom = 0x8005;
    uint8_t shift_register;
    uint8_t data_bit, crc_bit;

    for (counter = 0; counter < length; counter++)
    {
        for (shift_register = 0x01; shift_register > 0x00; shift_register <<= 1)
        {
            data_bit = (data[counter] & shift_register) ? 1 : 0;
            crc_bit = crc_register >> 15;
            crc_register <<= 1;
            if (data_bit != crc_bit)
                crc_register ^= polynom;
        }
    }
    printk("CRC-16: %x\n", crc_register);
    return crc_register;
}

static void print_packet(uint8_t *packet, uint8_t count) {
    printk("Command packet: ");
    for (int i = 0; i < count; i++) {
        printk("%x ", packet[i]);
    }
    printk("\n");
}

static int atecc608a_send_command(uint8_t cmd, uint8_t p1, uint16_t p2, 
                                 const uint8_t *data, uint8_t data_len,
                                 uint8_t *response, uint8_t *response_len, int delay_time_ms) {
    // Packet structure:
    // [word_addr] [count][cmd][param1][param2L][param2H][data...][CRC16L][CRC16H]
    uint8_t packet[128];
    uint8_t count = 7 + data_len;  // count includes count byte + 7 bytes overhead + data
    
    // printk("Building command packet: cmd=0x%x, p1=0x%x, p2=0x%x, data_len=%d\n", 
    //        cmd, p1, p2, data_len);
    
    // Build packet
    packet[0] = 0x03; // Word address
    packet[1] = count; // Packet length
    packet[2] = cmd; // Command
    packet[3] = p1; // Param1
    packet[4] = p2 & 0xFF; // Param2 LSB
    packet[5] = (p2 >> 8) & 0xFF; // Param2 MSB
    
    // Copy data if present
    if (data && data_len > 0) {
        for (int i = 0; i < data_len; i++) {
            packet[i + 6] = data[i];
        }
    }
    
    // Calculate CRC-16 over the entire packet (excluding CRC bytes)
    uint16_t crc = calculate_crc16(count - 2, packet + 1); // Exclude word_addr and final 2 CRC bytes
    packet[count - 1] = crc & 0xFF;        // CRC LSB (For info is 0x03)
    packet[count] = (crc >> 8) & 0xFF; // CRC MSB (For info is 0x5d)
    
    print_packet(packet, count + 1);
    
    // Send command
    if (i2c_write(ATECC608A_ADDR, (uint8_t *) &packet, count + 1) != count + 1) {
        printk("Failed to send command to ATECC608A\n");
        return -1;
    }
    
    // Wait for processing
    printk("Waiting %d ms for command execution...\n", delay_time_ms);
    delay_ms(delay_time_ms);

    // Try polling for command completion
    printk("Polling for command completion...\n");
    int tries = 0;
    int max_tries = 10;
    while (tries < max_tries) {
        // Reset word address (optional, may help with some I2C implementations)
        uint8_t reset_addr = 0x00;
        i2c_write(ATECC608A_ADDR, &reset_addr, 1);
        delay_ms(1);
        
        // Read response length
        uint8_t temp_resp[64];
        int resp_len = i2c_read(ATECC608A_ADDR, temp_resp, 1);
        
        if (resp_len != 1) {
            printk("Polling: No response yet (try %d/%d)\n", tries+1, max_tries);
            tries++;
            delay_ms(5);  // Wait a bit longer
            continue;
        }
        
        // Got a response
        resp_len = temp_resp[0];
        printk("Response length: %d bytes\n", resp_len);
        
        // Read the rest
        if (resp_len > 1) {
            int read_bytes = i2c_read(ATECC608A_ADDR, temp_resp + 1, resp_len - 1);
            if (read_bytes != resp_len - 1) {
                printk("Failed to read complete response\n");
                tries++;
                delay_ms(5);
                continue;
            }
            
            // Print response
            printk("Full response on poll %d: ", tries+1);
            for (int i = 0; i < resp_len; i++) {
                printk("%x ", temp_resp[i]);
            }
            printk("\n");
            // Copy to response buffer
            for (int i = 0; i < resp_len; i++) {
                response[i] = temp_resp[i];
            }
            *response_len = resp_len;
            return 0;  // Success
        }
        
        tries++;
        delay_ms(5);
    }
    return -1;  // Failure if max tries exceeded
}

int atecc608a_get_revision_info(void) {
    printk("Executing get_revision_info...\n");
    
    uint8_t response[7]; // 4 bytes for response + 3 bytes metadata
    uint8_t response_len = 7;
    
    // For the INFO command with Revision mode:
    // Mode (p1) should be 0x00 (Revision mode)
    // Param2 should be 0x0000
    int ret = -1;
    ret = atecc608a_send_command(ATECC_CMD_INFO, 0x00, 0x0000, NULL, 0, response, &response_len, 5);
    int data_len = response_len - 3; // 3 bytes metadata - 1 byte status + 2 bytes CRC
    if (ret == 0) {
        // Check if response status bit is 0x0
        if (response[1] == 0x00) {
            printk("Revision info received successfully: ");
        } else {
            printk("Failed to get revision info: ");
        }
        for (int j = 0; j < data_len; j++) {
            printk("%x ", response[j + 1]);
        }
        printk("\n");
    } else {
        printk("Failed to get revision info, error: %d\n", ret);
    }
    return ret;
}

int atecc608a_init(void) {
    // Initialize I2C
    i2c_init();
    // Wake up the device
    atecc608a_wakeup();
    // Get revision info
    atecc608a_get_revision_info();
    return 0;
}

int atecc608a_random(uint8_t *rand_out) {
    
    // Wake up the device
    atecc608a_wakeup();
    // Random command parameters
    uint8_t mode = 0x00;  // Default mode
    uint8_t response[32];
    uint8_t response_len = sizeof(response);
    
    int ret = atecc608a_send_command(ATECC_CMD_RANDOM, mode, 0, NULL, 0, response, &response_len, 50);
    
    if (ret < 0)
        return -1;
    
    // Copy random bytes to output buffer
    for (int i = 0; i < 32; i++) {
        rand_out[i] = response[i + 1]; // Skip the first byte (status)
    }
    
    return 0;
}

// Get the public key
int atecc608a_pubkey(uint8_t key_id, uint8_t *pubkey) {
    // Wake up the device first
    atecc608a_wakeup();
    
    uint8_t response[70]; // Large enough for the signature response
    uint8_t response_len = sizeof(response);
    
    // Send GENKEY command with mode 0 (get public key from private key)
    // Mode 0x00: compute public key from existing private key
    printk("Retrieving public key for key_id %d...\n", key_id);
    
    // The key_id is provided in param2
    uint16_t param2 = key_id;
    
    int ret = atecc608a_send_command(ATECC_CMD_GENKEY, 0x00, param2, 
                                    NULL, 0, response, &response_len, 50);
    
    if (ret != 0) {
        printk("Failed to execute GENKEY command\n");
        return -1;
    }
        
    // The public key is 64 bytes (X and Y coordinates, 32 bytes each)
    // It starts after count byte in the response
    printk("Public key retrieved successfully\n");
    
    // Copy the public key to the output buffer
    for (int i = 0; i < 64; i++) {
        pubkey[i] = response[i + 1]; // Skip count byte
    }
    
    // Put the device to sleep to save power
    atecc608a_sleep();
    
    return 0;
}


int atecc608a_sign(uint8_t key_id, const uint8_t *msg, uint8_t *signature) {
    // Wake up the device first
    atecc608a_wakeup();
    
    uint8_t response[70]; // Large enough for the signature response
    uint8_t response_len = sizeof(response);
    int ret;
    
    // Step 1: Load the message digest into TempKey using NONCE command
    // For NONCE command in Pass-through mode (mode 3)
    // This loads the 32-byte message/digest directly into TempKey
    printk("Loading message digest into TempKey...\n");
    ret = atecc608a_send_command(ATECC_CMD_NONCE, 0x03, 0x0000, 
                                msg, 32, response, &response_len, 10);
    
    if (ret != 0) {
        printk("Failed to execute NONCE command\n");
        return -1;
    }
    
    // Check if NONCE command was successful
    if (response[1] != 0x00) {
        printk("NONCE command failed with error: %x\n", response[1]);
        return -1;
    }
    
    // Step 2: Sign the digest using SIGN command
    // Mode 0x80: use TempKey as the source of the digest
    printk("Signing digest with key %d...\n", key_id);
    response_len = sizeof(response);
    
    // The key_id parameter is provided in param2
    uint16_t param2 = key_id;
    
    ret = atecc608a_send_command(ATECC_CMD_SIGN, 0x80, param2, 
                                NULL, 0, response, &response_len, 100);
    
    if (ret != 0) {
        printk("Failed to execute SIGN command\n");
        return -1;
    }
    
    // Check if SIGN command was successful
    // if (response[1] != 0x00) {
    //     printk("SIGN command failed with error: %x\n", response[1]);
    //     return -1;
    // }
    
    // Copy the signature from the response
    // The signature is 64 bytes (r and s components of ECDSA signature)
    // It starts after the status byte (response[1])
    printk("Signature generated successfully\n");
    
    // Copy the signature to the output buffer
    for (int i = 0; i < 64; i++) {
        signature[i] = response[i + 1];
    }
    
    // Put the device to sleep to save power
    atecc608a_sleep();
    
    return 0;
}

// First, implement the missing verify_signature function
int atecc608a_verify_signature(const uint8_t *signature, const uint8_t *public_key) {
    // Ensure the device is awake
    if (!atecc608a_is_awake()) {
        atecc608a_wakeup();
    }
    
    uint8_t response[7]; // Count + status + CRC bytes
    uint8_t response_len = sizeof(response);
    
    // Prepare the combined data buffer for the VERIFY command
    uint8_t verify_data[128];
    
    // Copy signature (64 bytes) to the first part of verify_data
    for (int i = 0; i < 64; i++) {
        verify_data[i] = signature[i];
    }
    
    // Copy public key (64 bytes) to the second part of verify_data
    for (int i = 0; i < 64; i++) {
        verify_data[i + 64] = public_key[i];
    }
    
    printk("Verifying signature with external public key...\n");
    
    // Send VERIFY command in External mode (0x02)
    // Param2 = 0x0004 specifies P256 NIST ECC curve
    int ret = atecc608a_send_command(ATECC_CMD_VERIFY, 0x02, 0x0004, 
                                    verify_data, sizeof(verify_data), 
                                    response, &response_len, 70);
    
    if (ret != 0) {
        printk("Failed to execute VERIFY command\n");
        return -1;
    }
    
    // Check verification result
    if (response[1] == 0x00) {
        printk("Signature verified successfully\n");
        return 0; // Success
    } else if (response[1] == 0x01) {
        printk("Signature verification failed - invalid signature\n");
        return 1; // Invalid signature
    } else {
        printk("Verification command returned error: 0x%02x\n", response[1]);
        return -1; // Other error
    }
}
int atecc608a_load_tempkey(const uint8_t *data) {
    // Wake up the device if needed
    if (!atecc608a_is_awake()) {
        atecc608a_wakeup();
    }
    
    uint8_t response[7]; // Count + status + 2 CRC bytes + buffer
    uint8_t response_len = sizeof(response);
    
    printk("Loading data into TempKey using NONCE command...\n");
    
    // NONCE command with mode 0x03 (Pass-through mode)
    // This directly loads the 32-byte input into TempKey without hashing
    int ret = atecc608a_send_command(ATECC_CMD_NONCE, 0x03, 0x0000, 
                                    data, 32, response, &response_len, 10);
    
    if (ret != 0) {
        printk("Failed to execute NONCE command\n");
        return -1;
    }
    
    // Check if NONCE command was successful (status byte should be 0x00)
    if (response[1] != 0x00) {
        printk("NONCE command failed with error code: 0x%02x\n", response[1]);
        return -1;
    }
    
    printk("Data successfully loaded into TempKey\n");
    return 0;
}

int atecc608a_verify(const uint8_t *msg, const uint8_t *signature, const uint8_t *public_key) {
    // Step 1: Load the message digest into TempKey
    int ret = atecc608a_load_tempkey(msg);
    if (ret != 0) {
        printk("Failed to load message into TempKey\n");
        return -1;
    }
    
    // Step 2: Verify the signature against the public key
    ret = atecc608a_verify_signature(signature, public_key);
    
    // Put the device to sleep to save power
    atecc608a_sleep();
    
    return ret;
}

// Now create a complete test function
int test_sign_verify(void) {
    printk("Starting sign-verify test...\n");
    
    // Initialize the device
    atecc608a_init();
    
    // Test message (this could be a hash of your actual data)
    uint8_t message[32] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20
    };
    
    // Buffers for signature and public key
    uint8_t signature[64] = {0};
    uint8_t public_key[64] = {0};
    
    // Slot ID to use for signing - choose a slot configured with a private ECC key
    uint8_t key_id = 0; // Adjust this to match your configuration
    
    // Step 1: Get the public key for the slot
    printk("Retrieving public key from slot %d...\n", key_id);
    if (atecc608a_pubkey(key_id, public_key) != 0) {
        printk("Failed to retrieve public key\n");
        return -1;
    }
    
    // Print the public key (hex format for better readability)
    printk("Public key: ");
    for (int i = 0; i < 64; i++) {
        printk("%x", public_key[i]);
        if ((i + 1) % 16 == 0) printk("\n           ");
    }
    printk("\n");
    
    // Step 2: Sign the message with the private key
    printk("Signing message with private key in slot %d...\n", key_id);
    if (atecc608a_sign(key_id, message, signature) != 0) {
        printk("Failed to sign message\n");
        return -1;
    }
    
    // Print the signature (hex format)
    printk("Signature: ");
    for (int i = 0; i < 64; i++) {
        printk("%x", signature[i]);
        if ((i + 1) % 16 == 0) printk("\n           ");
    }
    printk("\n");
    
    // Step 3: Verify the signature using the public key
    printk("Verifying signature...\n");
    message[3] = 0x09;
    int result = atecc608a_verify(message, signature, public_key);
    
    if (result == 0) {
        printk("VERIFICATION SUCCESSFUL: Signature is valid!\n");
        return 0;
    } else if (result == 1) {
        printk("VERIFICATION FAILED: Signature is invalid\n");
        return 1;
    } else {
        printk("VERIFICATION ERROR: Process failed\n");
        return -1;
    }
}