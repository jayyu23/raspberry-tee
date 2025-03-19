#include <Wire.h>

// Configuration
#define ATECC_ADDR 0x60       // Default I2C address for ATECC devices
#define SERIAL_BAUD 115200
#define SDA_PIN A4            // Standard Arduino SDA pin
#define SCL_PIN A5            // Standard Arduino SCL pin

// ATECC command opcodes
#define ATECC_CMD_RANDOM 0x1B
#define ATECC_CMD_INFO 0x30
#define ATECC_CMD_PUBKEY 0x23 // Define this to read the public key
#define ATECC_CMD_GENKEY 0x40
#define MAX_BUF_LEN 32

// Simple buffer for received data
byte rxBuffer[MAX_BUF_LEN];
byte rxLength = 0;

byte responseBuffer[MAX_BUF_LEN];
int responseLen = 0;

// Predefined responses for different commands
const byte deviceInfoResponse[] = {0x01, 0x23, 0x45, 0x67};

// Hardcoded key pair (just for demonstration - in a real device these would be securely generated)
// Private key - typically never exposed directly in a real ATECC device
const byte privateKey[] = {
  0x73, 0x77, 0x6d, 0x20, 0xf2, 0xc6, 0xca, 0xe0,
  0x06, 0x13, 0xc5, 0x3d, 0x0e, 0x5f, 0xb8, 0xdb,
  0x65, 0x30, 0x58, 0x11, 0x9d, 0x8e, 0x9b, 0xd5,
  0x18, 0x4d, 0x00, 0x93, 0xb1, 0x43, 0x2e, 0x44
};

// Public key - would be generated from private key in a real device
// For P256 curve, public key is typically 64 bytes (x,y coordinates)
const byte publicKey[] = {
  // X coordinate
  0x25, 0x19, 0x0f, 0x95, 0x53, 0x7d, 0xa6, 0x8b,
  0x89, 0x91, 0x17, 0x16, 0x2f, 0x8c, 0x47, 0x95,
  0x56, 0x38, 0x83, 0x31, 0x01, 0x8e, 0xe5, 0x4c,
  0xa6, 0x4d, 0x05, 0x9a, 0x9d, 0xe3, 0x61, 0x2f,
  // Y coordinate
  0x3c, 0x09, 0x26, 0x35, 0x06, 0x7c, 0x87, 0x4b,
  0x46, 0xf1, 0xa8, 0xab, 0x69, 0x6f, 0xd1, 0x68,
  0x34, 0x0f, 0x5e, 0xed, 0x0e, 0xb3, 0x1a, 0x83,
  0x2c, 0x83, 0xd9, 0x98, 0x4e, 0xc2, 0xfd, 0xab
};

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println("ATECC Device Simulator Starting");
  
  // Initialize I2C as slave
  Wire.begin(ATECC_ADDR);
  
  // Register event handlers
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  Wire.setClock(100000);
  
  // Set up pins for wake detection
  pinMode(SDA_PIN, INPUT_PULLUP);
  pinMode(SCL_PIN, INPUT_PULLUP);
  
  Serial.println("Listening on I2C address 0x60 (96 decimal)");
}

void loop() {
}

// Called when RPi sends data to this device
void receiveEvent(int numBytes) {
  Serial.print("Received ");
  Serial.print(numBytes);
  Serial.println(" bytes:");
  
  rxLength = 0;
  while (Wire.available() && rxLength < 32) {
    rxBuffer[rxLength] = Wire.read();
    Serial.print("0x");
    if (rxBuffer[rxLength] < 16) Serial.print("0");
    Serial.print(rxBuffer[rxLength], HEX);
    Serial.print(" ");
    rxLength++;
  }
  Serial.println();
  
  // Handle the command
  if (rxLength > 0) {
    byte command = rxBuffer[1];
    interpretCommand(command);
  }
}

// Called when RPi requests data from this device
void requestEvent() {
  // Serial.println("I2C request received!");
  Wire.write(responseBuffer, responseLen);
  memset(responseBuffer, 0, MAX_BUF_LEN);
  responseLen = 0;
}


void interpretCommand(byte command) {
  Serial.print("Command interpreted as: ");
  
  switch (command) {
    case ATECC_CMD_RANDOM:
      Serial.println("RANDOM");
      for (int i = 0; i < 32; i++) {
        responseBuffer[i] = random(0, 256); // Random byte (0-255)
      }
      responseLen = 32;
      break;
      
    case ATECC_CMD_INFO:
      Serial.println("INFO");
      memcpy(responseBuffer, deviceInfoResponse, sizeof(deviceInfoResponse));
      responseLen = sizeof(deviceInfoResponse);
      break;

    case ATECC_CMD_GENKEY:
      Serial.println("GENKEY");
      int keySlot;
      if (rxLength >= 3) {
        keySlot = rxBuffer[2]; // Get slot number from command
      }
      // For simulation, we just return the X coordinate of the hardcoded public key
      memcpy(responseBuffer, publicKey, 32);
      responseLen = 32;
      Serial.print("Generated key for slot ");
      Serial.println(keySlot);
      break;
    

    case ATECC_CMD_PUBKEY:
      Serial.println("PUBKEY");
      if (rxLength >= 3) {
        keySlot = rxBuffer[2]; // Get slot number from command
      }
      
      // Check if this is a request for X or Y part
      if (rxLength >= 4 && rxBuffer[3] == 0x01) {
        // Request for Y coordinate
        memcpy(responseBuffer, &publicKey[32], 32);
        Serial.println("Sending Y coordinate");
      } else {
        // Default: send X coordinate
        memcpy(responseBuffer, publicKey, 32);
        Serial.println("Sending X coordinate");
      }
      
      responseLen = 32;
      break;

      
    default:
      Serial.print("UNKNOWN (0x");
      Serial.print(command, HEX);
      Serial.println(")");
      break;
  }
}