# RaspberryLedger
Bare-metal implementation on Raspberry Pi (BCM2835) with ATECC608A trusted hardware chip to sign blockchain transactions securely

Jay Yu and Rishi Padmanabhan

CS 140E: Embedded Operating Systems, Winter 2025 Final Project

![Raspberry TEE](./img/image.png)


## Key Features

- Baremetal implementation of I2C protocol (Raspberry Pi BCM2835 SoC as I2C master) in `i2c.c`
- Baremetal implementation of ATECC608A commands with default configuration in `atecc608a.c`
- Private key securely stored in ATECC608A, public key available upon request
- Sign arbitrary messages with ECDSA-P256 private key
- Verify signatures against the corresponding ECDSA-P256 public key
- Test suite in `proj/1-i2c/tests`
- Supplemental Arduino ATECC608A I2C slave simulator for testing I2C communication in `arduino-atecc608a`

## Setup

1. Assumes working `pi-install` tool from CS140E repo.
2. Please set `CS140E_2025_PATH_FINAL` to the path of this project repository. 
3. Make `libpi`, `cd libpi && make`
4. For each subdirectory in `proj`, run `make run`

## Usage

- Run `make run` in each subdirectory to execute the test programs
- Run `make clean` to clean up the build directory

## Notes

- ATECC608A is configured to use I2C address 0x60
- ATECC608A is configured to use default configuration from Sparkfun ATECC608A breakout board
- ATECC608A is configured to use slot 0 for private key
