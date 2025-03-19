# Makefile for I2C driver

# Set the path to CS140E project
ifndef CS140E_2025_PATH_FINAL
$(error CS140E_2025_PATH_FINAL is not set)
endif

# Tests/examples to build
# PROGS += tests/0-hello-world.c
# PROGS := tests/1-test-i2c-init.c
# PROGS += tests/2-test-i2c-scan.c
# PROGS += tests/3-atecc-wake-test.c
# PROGS += tests/4-test-atecc608a-random.c
# PROGS += tests/4-test-arduino.c
PROGS += tests/4-arduino-keygen.c
# Common source files
COMMON_SRC += ./i2c.c
COMMON_SRC += ./atecc608a.c

# STAFF_OBJS += ./staff-i2c.o

# Include directories

# Optional bootloader
TTYUSB = "/dev/cu.usbserial-1110"
BOOTLOADER = pi-install

# Set to 1 to automatically run after build
RUN = 0

# Grep string for filtering output
GREP_STR := 'TRACE:\|HASH:\|ERROR:\|PANIC:\|SUCCESS:'

# Include the robust makefile template
include $(CS140E_2025_PATH_FINAL)/libpi/mk/Makefile.robust-v2

# Custom clean
clean::
	rm -f *.bin *.elf *.list *.o src/*.o tests/*.o
	make -C tests clean

# Directory creation
directories:
	@mkdir -p inc src tests

.PHONY: directories