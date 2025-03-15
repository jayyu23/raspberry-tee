#ifndef __I2C_H__
#define __I2C_H__

#include "rpi.h"

// I2C pins - BCM numbering
#define I2C_SDA 2
#define I2C_SCL 3

// I2C Base address
#define I2C_BASE 0x20804000

// I2C Registers
#define I2C_C          (I2C_BASE + 0x00)  // Control
#define I2C_S          (I2C_BASE + 0x04)  // Status
#define I2C_DLEN       (I2C_BASE + 0x08)  // Data Length
#define I2C_A          (I2C_BASE + 0x0C)  // Slave Address
#define I2C_FIFO       (I2C_BASE + 0x10)  // Data FIFO
#define I2C_DIV        (I2C_BASE + 0x14)  // Clock Divider
#define I2C_DEL        (I2C_BASE + 0x18)  // Data Delay
#define I2C_CLKT       (I2C_BASE + 0x1C)  // Clock Stretch Timeout

// I2C Control Register Bits
#define I2C_C_I2CEN    (1 << 15)  // I2C Enable
#define I2C_C_INTR     (1 << 10)  // Interrupt on RX
#define I2C_C_INTT     (1 << 9)   // Interrupt on TX
#define I2C_C_INTD     (1 << 8)   // Interrupt on DONE
#define I2C_C_ST       (1 << 7)   // Start transfer
#define I2C_C_CLEAR    (1 << 5)   // Clear FIFO
#define I2C_C_READ     (1 << 0)   // Read transfer

// I2C Status Register Bits
#define I2C_S_CLKT     (1 << 9)   // Clock stretch timeout
#define I2C_S_ERR      (1 << 8)   // Error
#define I2C_S_RXF      (1 << 7)   // RX FIFO full
#define I2C_S_TXE      (1 << 6)   // TX FIFO empty
#define I2C_S_RXD      (1 << 5)   // RX FIFO contains data
#define I2C_S_TXD      (1 << 4)   // TX FIFO can accept data
#define I2C_S_RXR      (1 << 3)   // RX FIFO needs reading
#define I2C_S_TXW      (1 << 2)   // TX FIFO needs writing
#define I2C_S_DONE     (1 << 1)   // Transfer done
#define I2C_S_TA       (1 << 0)   // Transfer active

// Initialize I2C
void i2c_init(void);

// Write data to I2C device
int i2c_write(uint8_t addr, const uint8_t *data, unsigned len);

// Read data from I2C device
int i2c_read(uint8_t addr, uint8_t *data, unsigned len);

// Write then read data (common I2C operation)
int i2c_write_read(uint8_t addr, const uint8_t *wdata, unsigned wlen, uint8_t *rdata, unsigned rlen);

#endif