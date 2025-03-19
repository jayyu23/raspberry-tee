#include <Wire.h>
#include "uECC.h"

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

byte privateKey[32] = {0};
byte publicKey[64] = {0};

// Generate using the secp256k1 curve that Bitcoin, Ethereum and other cryptocurrencies use
const struct uECC_Curve_t * curve = uECC_secp256k1(); 

void generateKeyPair() {
  Serial.println("Generating secp256k1 key pair...");

  unsigned long startTime = millis();
  int ret = uECC_make_key(publicKey, privateKey, curve);
  unsigned long endTime = millis();

  if (ret) {
    Serial.println("Key pair generated successfully");
    Serial.print("Generation time (ms): ");
    Serial.println(endTime - startTime);
  } else {
    Serial.println("Failed to generate key pair!");
  }
  printKeyPair();
}

static int rng_function(uint8_t *dest, unsigned size) {
  for (unsigned i = 0; i < size; i++) {
    dest[i] = (uint8_t)random(256);
  }
  return 1;
}

void printKeyPair() {
  // Serial.println("Private key:");
  // for (int i = 0; i < 32; i++) {
  //   Serial.print(privateKey[i], HEX);
  //   Serial.print(" ");
  // }
  // Serial.println();
  Serial.println("Public key:");
  for (int i = 0; i < 64; i++) {
    Serial.print(publicKey[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

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
  
  uECC_set_rng(rng_function);
  Serial.println("Listening on I2C address 0x60 (96 decimal)");
  // generateKeyPair();
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
      Serial.println("Generating key pair...");
      generateKeyPair();
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