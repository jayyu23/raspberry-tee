#include "rpi.h"
#include "i2c.h"
#include "atecc608a.h"

void notmain(void) {
    uart_init();
    printk("ATECC608A Detection Test for %x\n", ATECC608A_ADDR);
    
    i2c_init();
    printk("I2C initialized\n");
    
    // Try to wake up the device
    printk("Attempting to wake ATECC608A...\n");
    
    // Send wake pulse (byte 0x00 at 100kHz)
    // uint8_t wake_cmd = 0x00;
    // int wake_result = i2c_write(0x00, &wake_cmd, 1);

    atecc608a_wakeup();
    
    // Wait for device to wake
    
    // // Read response
    // uint8_t response[4];
    // int read_result = i2c_read(ATECC608A_ADDR, response, 4);
    
    // if (read_result == 4 && response[0] == 0x04 && response[1] == 0x11) {
    //     printk("ATECC608A detected and awake!\n");
        
    //     // Put device to sleep
    //     printk("Putting device to sleep...\n");
    //     uint8_t sleep_cmd = 0x01;
    //     i2c_write_with_addr(ATECC608A_ADDR, 0x01, NULL, 0);
        
    //     printk("SUCCESS: ATECC608A detected and successfully communicated\n");
    // } else if (read_result < 0) {
    //     printk("FAILURE: No ACK received from ATECC608A address\n");
    //     printk("Device not present or not responding\n");
    // } else {
    //     printk("FAILURE: Unexpected response: [%02x %02x %02x %02x]\n", 
    //            response[0], response[1], response[2], response[3]);
    // }
    
    clean_reboot();
}